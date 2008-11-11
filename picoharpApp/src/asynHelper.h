#ifndef __ASYN_HELPER_H__
#define __ASYN_HELPER_H__

#include <asynDriver.h>
#include <asynDrvUser.h>

#define DECLARE_INTERFACE(self, typ, impl) \
  self->typ.interfaceType = asyn ## typ ## Type; \
  self->typ.pinterface = &impl; \
  self->typ.drvPvt = self;

#define ASYNMUSTSUCCEED(e, m) \
{ \
 asynStatus status = e; \
 if(status != asynSuccess) { \
   errlogPrintf(m); \
   return -1; \
 } \
}

typedef struct COMMANDPAIR
{
  char *name;
  int value;
} commandPair;

/* common */

void common_report (void *drvPvt, FILE * fp, int details);
asynStatus common_connect (void *drvPvt, asynUser * pasynUser);
asynStatus common_disconnect (void *drvPvt, asynUser * pasynUser);

/* DrvUser */

asynStatus drvuser_create (void *drvPvt, asynUser * pasynUser,
			   const char *drvInfo,
			   const char **pptypeName, size_t * psize);

asynStatus drvuser_get_type (void *drvPvt, asynUser * pasynUser,
			     const char **pptypeName, size_t * psize);

asynStatus drvuser_destroy (void *drvPvt, asynUser * pasynUser);

#endif
