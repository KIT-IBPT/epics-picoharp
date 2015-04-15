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

#define EXPORT_RANGE(name, extra...) \
    EXPORT_PICO(name##_fast, extra), \
    EXPORT_PICO(name##_5, extra), \
    EXPORT_PICO(name##_60, extra), \
    EXPORT_PICO(name##_180, extra), \
    EXPORT_PICO(name##_all, extra)

static struct struct_info picoStructInfo[] = {
    /* Readout values */

    EXPORT_RANGE(samples,       .alarmed = true),
    EXPORT_RANGE(raw_buckets,   .alarmed = true),
    EXPORT_RANGE(fixup,         .alarmed = true),
    EXPORT_RANGE(max_fixup,     .alarmed = true),
    EXPORT_RANGE(buckets,       .alarmed = true),
    EXPORT_RANGE(socs,          .alarmed = true),
    EXPORT_RANGE(turns,         .alarmed = true),

    EXPORT_RANGE(profile),
    EXPORT_RANGE(peak),
    EXPORT_RANGE(flux),
    EXPORT_RANGE(total_count),

    EXPORT_PICO(max_bin,        .alarmed = true),
    EXPORT_PICO(count_rate_0),
    EXPORT_PICO(count_rate_1),
    EXPORT_PICO(resolution),

    EXPORT_PICO(error),
    EXPORT_PICO(reset_time),

    /* Controllable parameters. */

    EXPORT_PICO(offset,         .notify = true),
    EXPORT_PICO(cfdzerox0,      .notify = true),
    EXPORT_PICO(cfdzerox1,      .notify = true),
    EXPORT_PICO(cfdlevel0,      .notify = true),
    EXPORT_PICO(cfdlevel1,      .notify = true),
    EXPORT_PICO(syncdiv,        .notify = true),
    EXPORT_PICO(range,          .notify = true),

    EXPORT_PICO(time),
    EXPORT_PICO(shift),
    EXPORT_PICO(sample_width),
    EXPORT_PICO(deadtime),
    EXPORT_PICO(reset_accum),

    /* Environmental readbacks. */

    EXPORT_PICO(dcct_alarm),
    EXPORT_PICO(current),


    EXPORT_ARRAY_END
};

struct pico_pvt
{
    struct struct_info *info;

    epicsMutexId lock;
    epicsEventId started;

    char *port;
    char *serial;
    int event_fast;     // Event for single shot update
    int event_5s;       // Event for 5 second update
    int alarm;

    asynInterface Common;
    asynInterface DrvUser;
    asynInterface Float64Array;
    asynInterface Float64;
    asynInterface Octet;

    struct pico_data data;
};



static const struct struct_info *lookup_info(void *drvPvt, asynUser *pasynUser)
{
    struct pico_pvt *pico = drvPvt;
    int field = pasynUser->reason;
    if (field < 0)
        return NULL;
    else
        return &pico->info[field];
}


static asynStatus pico_read_array(
    void *drvPvt, asynUser *pasynUser, epicsFloat64 *value,
    size_t elements, size_t *nIn)
{
    const struct struct_info *info = lookup_info(drvPvt, pasynUser);
    if (info == NULL)
        return asynError;

    struct pico_pvt *pico = drvPvt;
    epicsMutexMustLock(pico->lock);

    double *array = *(double **) MEMBER_LOOKUP(&pico->data, info);
    memcpy(value, array, elements * sizeof(epicsFloat64));
    *nIn = elements;
    bool alarm = pico->alarm && info->alarmed;

    epicsMutexUnlock(pico->lock);

    return alarm ? asynError : asynSuccess;
}

static asynStatus pico_read_value(
    void *drvPvt, asynUser *pasynUser, epicsFloat64 *value)
{
    const struct struct_info *info = lookup_info(drvPvt, pasynUser);
    if (info == NULL)
        return asynError;

    struct pico_pvt *pico = drvPvt;
    epicsMutexMustLock(pico->lock);

    *value = *(double *) MEMBER_LOOKUP(&pico->data, info);
    bool alarm = pico->alarm && info->alarmed;

    epicsMutexUnlock(pico->lock);

    return alarm ? asynError : asynSuccess;
}


static asynStatus pico_write_value(
    void *drvPvt, asynUser *pasynUser, epicsFloat64 value)
{
    const struct struct_info *info = lookup_info(drvPvt, pasynUser);
    if (info == NULL)
        return asynError;

    struct pico_pvt *pico = drvPvt;
    epicsMutexMustLock(pico->lock);

    *(double *) MEMBER_LOOKUP(&pico->data, info) = value;

    epicsMutexUnlock(pico->lock);
    return asynSuccess;
}

static asynStatus oct_read(
    void *drvPvt, asynUser *pasynUser, char *value,
    size_t numchars, size_t *nbytesTransferred, int *eomReason)
{
    const struct struct_info *info = lookup_info(drvPvt, pasynUser);
    if (info == NULL)
        return asynError;

    struct pico_pvt *pico = drvPvt;
    epicsMutexMustLock(pico->lock);

    snprintf(value, numchars, "%s", MEMBER_LOOKUP(&pico->data, info));
    *nbytesTransferred = numchars;
    *eomReason = 0;

    epicsMutexUnlock(pico->lock);
    return asynSuccess;
}

static asynFloat64Array asynFloat64ArrayImpl = {
    .read = pico_read_array
};
static asynFloat64 asynFloat64Impl = {
    .write = pico_write_value,
    .read = pico_read_value
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


/* Checks whether 5 seconds have passed since the last time things were
 * processed and updates *last_tick accordingly. */
static bool check_5s(time_t *last_tick)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    bool process = now.tv_sec >= *last_tick + 5;
    if (process)
        *last_tick = 5 * (now.tv_sec / 5);
    return process;
}


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
    time_t last_tick = 0;
    while (true)
    {
        /* acquire the data (usually 5s) */
        epicsMutexMustLock(pico->lock);
        int pico_time = (int) pico->data.time;
        epicsMutexUnlock(pico->lock);

        /* clear error and measure */
        snprintf(pico->data.error, ERRBUF, "%s", PICO_NO_ERROR);
        pico_measure(&pico->data, pico_time);

        /* Process the data. */

        epicsMutexMustLock(pico->lock);

        /* check DCCT alarm state */
        if(pico->data.dcct_alarm)
            snprintf(pico->data.error, ERRBUF, "%s", PICO_DCCT_ERROR);

        /* check for PicoHarp errors */
        if (strcmp(pico->data.error, PICO_NO_ERROR) == 0)
            pico->alarm = 0;
        else
            pico->alarm = 1;

        bool process_5s = check_5s(&last_tick);

        pico_process_fast(&pico->data);
        if (process_5s)
            pico_process_5s(&pico->data);

        epicsMutexUnlock(pico->lock);

        if (first)
        {
            epicsEventSignal(pico->started);
            first = false;
        }

        /* events are trivial, I/O interrupts are tedious */
        post_event(pico->event_fast);
        if (process_5s)
            post_event(pico->event_5s);

        if(pico->alarm)
            /* backoff in case of failure */
            epicsThreadSleep(1.0);
    }
}

/* port creation */

static int initPicoAsyn(
    const char *port, const char *serial, int event_fast, int event_5s,
    int bucket_count, int bin_size, int valid_samples,
    int samples_per_bucket, double turns_per_sec)
{
    printf("initPicoAsyn('%s')\n", port);

    struct pico_pvt *pico = callocMustSucceed(1, sizeof(*pico), "PicoAsyn");

    pico->info = picoStructInfo;
    pico->port = epicsStrDup(port);
    pico->serial = epicsStrDup(serial);
    pico->lock = epicsMutexMustCreate();
    pico->started = epicsEventMustCreate(epicsEventEmpty);
    pico->event_fast = event_fast;
    pico->event_5s = event_5s;

    // Hard-wired defaults to be overwritten by autosave/restore
    pico->data.offset = 0;
    pico->data.cfdlevel0 = 300;
    pico->data.cfdlevel1 = 100;
    pico->data.cfdzerox0 = 10;
    pico->data.cfdzerox1 = 5;
    pico->data.syncdiv = 1;

    /* Control parameters from IOC initialisation. */
    pico->data.range = bin_size;
    pico->data.bucket_count = bucket_count;
    pico->data.valid_samples = valid_samples;
    pico->data.samples_per_bucket = samples_per_bucket;
    pico->data.turns_per_sec = turns_per_sec;

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
    "initPicoAsyn", 9, (const iocshArg *[]) {
        &(iocshArg) { "Port name",          iocshArgString },
        &(iocshArg) { "Serial ID",          iocshArgString },
        &(iocshArg) { "Event Fast",         iocshArgInt },
        &(iocshArg) { "Event 5s",           iocshArgInt },
        &(iocshArg) { "Buckets per turn",   iocshArgInt },
        &(iocshArg) { "Picoharp bin size",  iocshArgInt },
        &(iocshArg) { "Total valid bins",   iocshArgInt },
        &(iocshArg) { "Bins per bucket",    iocshArgInt },
        &(iocshArg) { "Turns per second",   iocshArgDouble },
    }
};

static const iocshFuncDef scanFuncDef = { "scanPicoDevices", 0, NULL };

static void initCallFunc(const iocshArgBuf * args)
{
    initPicoAsyn(
        args[0].sval, args[1].sval, args[2].ival, args[3].ival, args[4].ival,
        args[5].ival, args[6].ival, args[7].ival, args[8].dval);
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
