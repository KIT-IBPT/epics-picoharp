#include <stdbool.h>
#include <string.h>

#include <epicsString.h>

#include "asynHelper.h"


/* drvUser base implementation */

asynStatus drvuser_create(
    void *drvPvt, asynUser *pasynUser,
    const char *drvInfo, const char **pptypeName, size_t *psize)
{
    struct struct_info *map = drvPvt;
    if (drvInfo == 0)
        return asynError;

    for (int n = 0; ; n ++)
    {
        const char *name = map[n].name;
        if (name == NULL)
            return asynError;
        else if (strcmp(name, drvInfo) == 0)
        {
            pasynUser->reason = n;
            if (pptypeName)
                *pptypeName = epicsStrDup(name);
            if (psize)
                *psize = sizeof(pasynUser->reason);
            return asynSuccess;
        }
    }
}

asynStatus drvuser_get_type(
    void *drvPvt, asynUser *pasynUser,
    const char **pptypeName, size_t *psize)
{
    struct struct_info *map = drvPvt;
    int command = pasynUser->reason;
    *pptypeName = NULL;
    *psize = 0;
    if (pptypeName)
        *pptypeName = epicsStrDup(map[command].name);
    if (psize)
        *psize = sizeof(command);
    return asynSuccess;
}

asynStatus drvuser_destroy(void *drvPvt, asynUser *pasynUser)
{
    return asynSuccess;
}

/* common */

void common_report(void *drvPvt, FILE *fp, int details)
{
    fprintf(fp, "report: %d %s\n", __LINE__, __FILE__);
}

asynStatus common_connect(void *drvPvt, asynUser *pasynUser)
{
    pasynManager->exceptionConnect(pasynUser);
    return asynSuccess;
}

asynStatus common_disconnect(void *drvPvt, asynUser *pasynUser)
{
    pasynManager->exceptionDisconnect(pasynUser);
    return asynSuccess;
}
