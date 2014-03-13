/*
  picoharp device support
*/

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

#include <asynDriver.h>
#include <asynFloat64Array.h>
#include <asynFloat64.h>
#include <asynOctet.h>
#include <asynDrvUser.h>

#include <dbScan.h>
#include <epicsThread.h>
#include <errlog.h>
#include <iocsh.h>
#include <epicsExport.h>
#include <epicsString.h>
#include <cantProceed.h>
#include <epicsMutex.h>
#include <epicsEvent.h>

#include "asynHelper.h"
#include "picopeaks.h"

#define PICO_NO_ERROR "OK"
#define PICO_DCCT_ERROR "ERROR_DCCT"

/*
 * mapping for read / write requests
 * structure, type, member name, alarmed
 */

#define EXPORT_PICO(member, extra...) \
    EXPORT_ARRAY(struct pico_data, double, member, extra)
static struct struct_info picoStructInfo[] = {
    EXPORT_PICO(buckets,    .alarmed = true),
    EXPORT_PICO(buckets60,  .alarmed = true),
    EXPORT_PICO(buckets180, .alarmed = true),

    EXPORT_PICO(samples,    .alarmed = true),
    EXPORT_PICO(samples60,  .alarmed = true),
    EXPORT_PICO(samples180, .alarmed = true),

    EXPORT_PICO(profile,    .alarmed = true),
    EXPORT_PICO(peak),
    EXPORT_PICO(flux,       .alarmed = true),
    EXPORT_PICO(nflux,      .alarmed = true),
    EXPORT_PICO(time),
    EXPORT_PICO(max_bin,    .alarmed = true),
    EXPORT_PICO(shift),
    EXPORT_PICO(sample_width),
    EXPORT_PICO(socs_5,     .alarmed = true),
    EXPORT_PICO(socs_60,    .alarmed = true),
    EXPORT_PICO(socs_180,   .alarmed = true),
    EXPORT_PICO(count_rate_0),
    EXPORT_PICO(count_rate_1),
    EXPORT_PICO(freq),
    EXPORT_PICO(dcct_alarm),
    EXPORT_PICO(charge),
    EXPORT_PICO(resolution),

    /* Controllable parameters.  If any of these are written then all parameters
     * will be reloaded. */
    EXPORT_PICO(Offset,     .notify = true),
    EXPORT_PICO(CFDZeroX0,  .notify = true),
    EXPORT_PICO(CFDZeroX1,  .notify = true),
    EXPORT_PICO(CFDLevel0,  .notify = true),
    EXPORT_PICO(CFDLevel1,  .notify = true),
    EXPORT_PICO(SyncDiv,    .notify = true),
    EXPORT_PICO(Range,      .notify = true),

    EXPORT_ARRAY_END
};

struct pico_pvt
{
    struct struct_info *info;

    epicsMutexId lock;
    epicsEventId started;

    char *port;
    char *serial;
    int event;
    int alarm;

    char alarm_string[ERRBUF];

    asynInterface Common;
    asynInterface DrvUser;
    asynInterface Float64Array;
    asynInterface Float64;
    asynInterface Octet;

    struct pico_data data;
};

/* float 64 array */

static asynStatus pico_write(
    void *drvPvt, asynUser *pasynUser, epicsFloat64 *value, size_t elements)
{
    struct pico_pvt *pico = drvPvt;
    struct pico_data *data = &pico->data;

    int field = pasynUser->reason;
    if (field < 0)
        return asynError;
    const struct struct_info *info = &pico->info[field];

    if (elements > info->elements)
        return asynError;

    epicsMutexMustLock(pico->lock);

    memcpy(MEMBER_LOOKUP(data, info), value, elements * sizeof(epicsFloat64));
    if (info->notify)
        data->parameter_updated = true;

    epicsMutexUnlock(pico->lock);

    return asynSuccess;
}

static asynStatus pico_read(
    void *drvPvt, asynUser *pasynUser, epicsFloat64 *value,
    size_t elements, size_t *nIn)
{
    struct pico_pvt *pico = drvPvt;
    struct pico_data *data = &pico->data;

    int field = pasynUser->reason;
    if (field < 0)
        return asynError;
    const struct struct_info *info = &pico->info[field];

    if (elements > info->elements)
        return asynError;

    epicsMutexMustLock(pico->lock);

    memcpy(value, MEMBER_LOOKUP(data, info), elements * sizeof(epicsFloat64));
    *nIn = elements;
    bool alarm = pico->alarm && info->alarmed;

    epicsMutexUnlock(pico->lock);

    return alarm ? asynError : asynSuccess;
}

static asynStatus pico_write_adapter(
    void *drvPvt, asynUser *pasynUser, epicsFloat64 value)
{
    return pico_write(drvPvt, pasynUser, &value, 1);
}

static asynStatus pico_read_adapter(
    void *drvPvt, asynUser *pasynUser, epicsFloat64 *value)
{
    size_t nIn;
    return pico_read(drvPvt, pasynUser, value, 1, &nIn);
}

static asynStatus oct_read(
    void *drvPvt, asynUser *pasynUser, char *data,
    size_t numchars, size_t *nbytesTransferred, int *eomReason)
{
    struct pico_pvt *pico = drvPvt;
    epicsMutexLock(pico->lock);
    snprintf(data, numchars, "%s", pico->alarm_string);
    *nbytesTransferred = numchars;
    *eomReason = 0;
    epicsMutexUnlock(pico->lock);
    return asynSuccess;
}

static asynFloat64Array asynFloat64ArrayImpl = {
    .write = pico_write,
    .read = pico_read
};
static asynFloat64 asynFloat64Impl = {
    .write = pico_write_adapter,
    .read = pico_read_adapter
};
static asynCommon asynCommonImpl = {
    .report = common_report,
    .connect = common_connect,
    .disconnect = common_disconnect
};
static asynDrvUser asynDrvUserImpl = {
    .create = drvuser_create,
    .getType = drvuser_get_type,
    .destroy = drvuser_destroy
};
static asynOctet asynOctetImpl = { .read = oct_read };

/* I/O and calculation thread */

static void picoThreadFunc(void *pvt)
{
    struct pico_pvt *pico = pvt;

    printf("pico_init\n");
    epicsMutexMustLock(pico->lock);
    if (!pico_init(&pico->data, pico->serial))
    {
        epicsMutexUnlock(pico->lock);
        epicsEventSignal(pico->started);
        return;
    }

    epicsThreadSleep(1.0);
    epicsMutexUnlock(pico->lock);

    bool first = true;
    while (true)
    {
        /* acquire the data (usually 5s) */
        epicsMutexMustLock(pico->lock);
        int pico_time = pico->data.time;
        epicsMutexUnlock(pico->lock);

        /* clear error and measure */
        snprintf(pico->data.errstr, ERRBUF, "%s", PICO_NO_ERROR);
        pico_measure(&pico->data, pico_time);

        /* do the averaging */

        epicsMutexMustLock(pico->lock);

        strcpy(pico->alarm_string, pico->data.errstr);

        /* check DCCT alarm state */
        if(pico->data.dcct_alarm)
            snprintf(pico->alarm_string, ERRBUF, "%s", PICO_DCCT_ERROR);

        /* check for PicoHarp errors */
        if (strcmp(pico->alarm_string, PICO_NO_ERROR) == 0)
            pico->alarm = 0;
        else
            pico->alarm = 1;
        pico_average(&pico->data);

        epicsMutexUnlock(pico->lock);

        if (first)
        {
            epicsEventSignal(pico->started);
            first = false;
        }
        /* events are trivial, I/O interrupts are tedious */

        post_event(pico->event);

        if(pico->alarm)
            /* backoff in case of failure */
            epicsThreadSleep(1.0);
    }
}

/* port creation */

static int initPicoAsyn(
    const char *port, const char *serial,
    int event, int Offset, int CFDLevel0, int CFDLevel1,
    int CFDZeroX0, int CFDZeroX1, int SyncDiv, int Range)
{
    printf("initPicoAsyn('%s')\n", port);

    struct pico_pvt *pico = callocMustSucceed(1, sizeof(*pico), "PicoAsyn");

    pico->info = picoStructInfo;
    pico->port = epicsStrDup(port);
    pico->serial = epicsStrDup(serial);
    pico->lock = epicsMutexMustCreate();
    pico->started = epicsEventMustCreate(epicsEventEmpty);
    pico->event = event;

    pico->data.Offset = Offset;
    pico->data.CFDLevel0 = CFDLevel0;
    pico->data.CFDLevel1 = CFDLevel1;
    pico->data.CFDZeroX0 = CFDZeroX0;
    pico->data.CFDZeroX1 = CFDZeroX1;
    pico->data.SyncDiv = SyncDiv;
    pico->data.Range = Range;

    DECLARE_INTERFACE(pico, Common, asynCommonImpl, pico);
    DECLARE_INTERFACE(pico, DrvUser, asynDrvUserImpl, pico->info);
    DECLARE_INTERFACE(pico, Float64Array, asynFloat64ArrayImpl, pico);
    DECLARE_INTERFACE(pico, Float64, asynFloat64Impl, pico);
    DECLARE_INTERFACE(pico, Octet, asynOctetImpl, pico);

    ASYNMUSTSUCCEED(pasynManager->
        registerPort(port, ASYN_MULTIDEVICE, 1, 0, 0),
        "PicoAsyn: Can't register port.\n");

    ASYNMUSTSUCCEED(pasynManager->registerInterface(port, &pico->Common),
        "PicoAsyn: Can't register common.\n");

    ASYNMUSTSUCCEED(pasynManager->registerInterface(port, &pico->DrvUser),
        "PicoAsyn: Can't register DrvUser.\n");

    ASYNMUSTSUCCEED(pasynFloat64ArrayBase->
        initialize(port, &pico->Float64Array),
        "PicoAsyn: Can't register float64Array.\n");

    ASYNMUSTSUCCEED(pasynFloat64Base->initialize(port, &pico->Float64),
        "PicoAsyn: Can't register float64Array.\n");

    ASYNMUSTSUCCEED(pasynOctetBase->initialize(port, &pico->Octet, 0, 0, 0),
        "PicoAsyn: Can't register Octet.\n");
    epicsThreadCreate(port, 0, 1024 * 1024, picoThreadFunc, pico);

    epicsEventMustWait(pico->started);

    return 0;
}


/* IOC shell commands */

static const iocshFuncDef initFuncDef = {
    "initPicoAsyn", 10, (const iocshArg *[]) {
        &(iocshArg) { "Port name",  iocshArgString },
        &(iocshArg) { "Serial ID",  iocshArgString },
        &(iocshArg) { "Event",      iocshArgInt },
        &(iocshArg) { "Offset",     iocshArgInt },
        &(iocshArg) { "CFDLevel0",  iocshArgInt },
        &(iocshArg) { "CFDLevel1",  iocshArgInt },
        &(iocshArg) { "CFDZeroX0",  iocshArgInt },
        &(iocshArg) { "CFDZeroX1",  iocshArgInt },
        &(iocshArg) { "SyncDiv",    iocshArgInt },
        &(iocshArg) { "Range",      iocshArgInt },
    }
};

static const iocshFuncDef scanFuncDef = {
    "scanPicoDevices", 0, NULL };

static void initCallFunc(const iocshArgBuf * args)
{
    initPicoAsyn(
        args[0].sval, args[1].sval, args[2].ival, args[3].ival,
        args[4].ival, args[5].ival, args[6].ival, args[7].ival,
        args[8].ival, args[9].ival);
}

static void scanCallFunc(const iocshArgBuf *args)
{
    scanPicoDevices();
}

static void epicsShareAPI PicoAsynRegistrar(void)
{
    iocshRegister(&initFuncDef, initCallFunc);
    iocshRegister(&scanFuncDef, scanCallFunc);
}

epicsExportRegistrar(PicoAsynRegistrar);
