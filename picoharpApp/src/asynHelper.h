#ifndef __ASYN_HELPER_H__
#define __ASYN_HELPER_H__

#include <asynDriver.h>
#include <asynDrvUser.h>

#define DECLARE_INTERFACE(self, typ, impl, pvt) \
  self->typ.interfaceType = asyn ## typ ## Type; \
  self->typ.pinterface = &impl; \
  self->typ.drvPvt = pvt;

#define ASYNMUSTSUCCEED(e, m) \
{ \
 asynStatus status = e; \
 if(status != asynSuccess) { \
   errlogPrintf(m); \
   return -1; \
 } \
}

#define EXPORT_ARRAY(STRUCT, TYPE, MEMBER) \
{offsetof(STRUCT, MEMBER), #MEMBER, (sizeof((STRUCT *)0)->MEMBER) / sizeof(TYPE)}
#define EXPORT_ARRAY_END {0, 0, 0}

#define MEMBER_LOOKUP(s, info, n) ((char *) s) + info[n].offset

typedef struct STRUCTINFO
{
  int offset;
  char * name;
  int elements;
} struct_info;

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
