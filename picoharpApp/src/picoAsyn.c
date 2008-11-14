/*
  picoharp device support
*/

#include <string.h>
#include <math.h>

#include <asynDriver.h>
#include <asynFloat64Array.h>
#include <asynFloat64.h>
#include <asynOctet.h>
#include <asynDrvUser.h>

#include <dbScan.h>
#include <cadef.h>
#include <errlog.h>
#include <iocsh.h>
#include <epicsExport.h>
#include <epicsString.h>
#include <cantProceed.h>
#include <epicsMutex.h>

#include "asynHelper.h"
#include "picopeaks.h"

/* map commands to values using dispatch table
   only works for doubles */

static struct_info picoStructInfo[] = {
  EXPORT_ARRAY (PicoData, double, buckets),
  EXPORT_ARRAY (PicoData, double, buckets60),
  EXPORT_ARRAY (PicoData, double, buckets180),
  EXPORT_ARRAY (PicoData, double, fill),
  EXPORT_ARRAY (PicoData, double, peak),
  EXPORT_ARRAY (PicoData, double, pk_auto),
  EXPORT_ARRAY (PicoData, double, flux),
  EXPORT_ARRAY (PicoData, double, time),
  EXPORT_ARRAY (PicoData, double, max_bin),
  EXPORT_ARRAY (PicoData, double, shift),
  EXPORT_ARRAY (PicoData, double, counts_fill),
  EXPORT_ARRAY (PicoData, double, counts_5),
  EXPORT_ARRAY (PicoData, double, counts_60),
  EXPORT_ARRAY (PicoData, double, counts_180),
  EXPORT_ARRAY (PicoData, double, freq),
  EXPORT_ARRAY (PicoData, double, current),
  EXPORT_ARRAY_END
};

typedef struct PICOPVT
{
  struct_info *info;

  epicsMutexId lock;

  char *port;
  int event;
  int alarm;

  asynInterface Common;
  asynInterface DrvUser;
  asynInterface Float64Array;
  asynInterface Float64;
  asynInterface Octet;

  PicoData data;

} PicoPvt;

/* float 64 array */

static asynStatus
pico_write (void *drvPvt, asynUser * pasynUser, epicsFloat64 * value,
	    size_t elements)
{
  PicoPvt *pico = (PicoPvt *) drvPvt;

  if (pasynUser->reason < 0)
    return (asynError);

  if (elements > pico->info[pasynUser->reason].elements)
    return (asynError);

  epicsMutexMustLock (pico->lock);

  memcpy (MEMBER_LOOKUP(&pico->data, pico->info, pasynUser->reason),
	  value, elements * sizeof (epicsFloat64));

  epicsMutexUnlock (pico->lock);

  return asynSuccess;
}

static asynStatus
pico_read (void *drvPvt, asynUser * pasynUser, epicsFloat64 * value,
	   size_t elements, size_t * nIn)
{
  PicoPvt *pico = (PicoPvt *) drvPvt;
  int alarm;

  if (pasynUser->reason < 0)
    return (asynError);

#if 0
  printf("reading %s\n", pico->info[pasynUser->reason].name);
#endif

  if (elements > pico->info[pasynUser->reason].elements)
    return (asynError);

  epicsMutexMustLock (pico->lock);

  memcpy (value, MEMBER_LOOKUP(&pico->data, pico->info, pasynUser->reason),
	  elements * sizeof (epicsFloat64));

  *nIn = elements;

  alarm = pico->alarm;

  epicsMutexUnlock (pico->lock);

  if (alarm)
    {
      return asynError;
    }
  else
    {
      return asynSuccess;
    }
}

static asynStatus
pico_write_adapter (void *drvPvt, asynUser * pasynUser, epicsFloat64 value)
{
  return pico_write (drvPvt, pasynUser, &value, 1);
}

static asynStatus
pico_read_adapter (void *drvPvt, asynUser * pasynUser, epicsFloat64 * value)
{
  size_t nIn;
  return pico_read (drvPvt, pasynUser, value, 1, &nIn);
}

static asynStatus
oct_read_raw (void *drvPvt, asynUser * pasynUser, char *data,
	      size_t numchars, size_t * nbytesTransferred, int *eomReason)
{
  PicoPvt *pico = (PicoPvt *) drvPvt;
  epicsMutexLock (pico->lock);
  snprintf (data, numchars, "%s", pico->data.errstr);
  *nbytesTransferred = numchars;
  *eomReason = 0;
  epicsMutexUnlock (pico->lock);
  return asynSuccess;
}

static asynFloat64Array asynFloat64ArrayImpl = { pico_write, pico_read };
static asynFloat64 asynFloat64Impl =
  { pico_write_adapter, pico_read_adapter };
static asynCommon asynCommonImpl = { common_report, common_connect,
  common_disconnect
};
static asynDrvUser asynDrvUserImpl = { drvuser_create, drvuser_get_type,
  drvuser_destroy
};
static asynOctet asynOctetImpl = { NULL, NULL, NULL, oct_read_raw };

/* I/O and calculation thread */

void
picoThreadFunc (void *pvt)
{

  PicoPvt *pico = (PicoPvt *) pvt;
  int time;

  epicsMutexMustLock (pico->lock);
  pico_init (&pico->data);
  epicsMutexUnlock (pico->lock); 

  while (1)
    {

      /* acquire the data (usually 5s) */

      epicsMutexMustLock (pico->lock);
      time = pico->data.time;
      epicsMutexUnlock (pico->lock);

      pico_measure (&pico->data, time);

      /* do the averaging */

      epicsMutexMustLock (pico->lock);

      /* check for PicoHarp errors */
      if (strcmp (pico->data.errstr, "") == 0)
	{
	  pico->alarm = 0;
	}
      else
	{
	  pico->alarm = 1;
	}
#if 0
      /* some debugging info */
      n = 0;
      while (1)
	{
	  if (pico->info[n].name == 0)
	    break;
	  double *value =
	    (double *) (((char *) &pico->data) + pico->info[n].offset);
	  printf ("%s: %f\n", pico->info[n].name, *value);
	  n++;
	}
#endif
      pico_average (&pico->data);

      epicsMutexUnlock (pico->lock);

      /* events are trivial, I/O interrupts are tedious */

      post_event (pico->event);

      epicsThreadSleep (1.0);
    }
}

/* port creation */

int
initPicoAsyn (char *port, int event, int Offset, int CFDLevel0, int CFDLevel1,
	      int CFDZeroX0, int CFDZeroX1, int SyncDiv, int Range)
{

  epicsThreadId thread;

  printf ("initPicoAsyn('%s')\n", port);

  PicoPvt *pico = callocMustSucceed (1, sizeof (*pico), "PicoAsyn");

  pico->info = picoStructInfo;
  pico->port = epicsStrDup (port);
  pico->lock = epicsMutexMustCreate ();
  pico->event = event;

  pico->data.Offset = Offset;
  pico->data.CFDLevel0 = CFDLevel0;
  pico->data.CFDLevel1 = CFDLevel1;
  pico->data.CFDZeroX0 = CFDZeroX0;
  pico->data.CFDZeroX1 = CFDZeroX1;
  pico->data.SyncDiv = SyncDiv;
  pico->data.Range = Range;

  DECLARE_INTERFACE (pico, Common, asynCommonImpl, pico);
  DECLARE_INTERFACE (pico, DrvUser, asynDrvUserImpl, pico->info);
  DECLARE_INTERFACE (pico, Float64Array, asynFloat64ArrayImpl, pico);
  DECLARE_INTERFACE (pico, Float64, asynFloat64Impl, pico);
  DECLARE_INTERFACE (pico, Octet, asynOctetImpl, pico);

  ASYNMUSTSUCCEED (pasynManager->
		   registerPort (port, ASYN_MULTIDEVICE, 1, 0, 0),
		   "PicoAsyn: Can't register port.\n");

  ASYNMUSTSUCCEED (pasynManager->registerInterface (port, &pico->Common),
		   "PicoAsyn: Can't register common.\n");

  ASYNMUSTSUCCEED (pasynManager->registerInterface (port, &pico->DrvUser),
		   "PicoAsyn: Can't register DrvUser.\n");

  ASYNMUSTSUCCEED (pasynFloat64ArrayBase->
		   initialize (port, &pico->Float64Array),
		   "PicoAsyn: Can't register float64Array.\n");

  ASYNMUSTSUCCEED (pasynFloat64Base->initialize (port, &pico->Float64),
		   "PicoAsyn: Can't register float64Array.\n");

  ASYNMUSTSUCCEED (pasynOctetBase->initialize (port, &pico->Octet, 0, 0, 0),
		   "PicoAsyn: Can't register Octet.\n");
  thread =
    epicsThreadCreate (port, 0, 1024 * 1024, picoThreadFunc, (void *) pico);

  return 0;
}

/* IOC shell command */

static const iocshArg initArg0 = { "Port name", iocshArgString };
static const iocshArg initArg1 = { "Event", iocshArgInt };
static const iocshArg initArg2 = { "Offset", iocshArgInt };
static const iocshArg initArg3 = { "CFDLevel0", iocshArgInt };
static const iocshArg initArg4 = { "CFDLevel1", iocshArgInt };
static const iocshArg initArg5 = { "CFDZeroX0", iocshArgInt };
static const iocshArg initArg6 = { "CFDZeroX1", iocshArgInt };
static const iocshArg initArg7 = { "SyncDiv", iocshArgInt };
static const iocshArg initArg8 = { "Range", iocshArgInt };

static const iocshArg *const initArgs[] = {
  &initArg0, &initArg1, &initArg2, &initArg3,
  &initArg4, &initArg5, &initArg6, &initArg7, &initArg8
};

static const iocshFuncDef initFuncDef = { "initPicoAsyn", 9, initArgs };

static void
initCallFunc (const iocshArgBuf * args)
{
  initPicoAsyn (args[0].sval, args[1].ival, args[2].ival, args[3].ival,
		args[4].ival, args[5].ival, args[6].ival, args[7].ival,
		args[8].ival);
}

static void epicsShareAPI
PicoAsynRegistrar (void)
{
  iocshRegister (&initFuncDef, initCallFunc);
}

epicsExportRegistrar (PicoAsynRegistrar);
