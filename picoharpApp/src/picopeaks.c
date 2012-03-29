#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <phlib.h>
#include <phdefin.h>

#include "picopeaks.h"

/* TODO return from function on error */

#define PICO_CHECK(call) \
    { \
        int __status = call; \
        char __errstr[ERRBUF]; \
        if(__status < 0) \
        { \
            PH_GetErrorString(__errstr, __status); \
            sprintf(self->errstr, "%s", __errstr); \
            printf(#call " %s\n", self->errstr); \
            return -1; \
        } \
    }


static double matlab_mod(double x, double y)
{
    /* matlab real modulo */
    double n = floor(x / y);
    return x - n * y;
}

/* return index of first peak in picoharp buffer */
static int pico_detect_peak(struct pico_data *self)
{
    if (self->pk_auto != 1)
        return self->peak;

    /* find first peak */
    double pk = self->samples[0];
    int first_peak_auto = 1;
    for (int i = 1; i < VALID_SAMPLES; ++i)
    {
        if (self->samples[i] > pk)
        {
            pk = self->samples[i];
            first_peak_auto = i + 1;
        }
    }

    /* peak magic */
    int peak =
        round(matlab_mod(first_peak_auto, VALID_SAMPLES * 1.0 / BUCKETS));
    if (peak < 5)
    {
        if (peak < 1)
            peak = 1;
    }
    else if (peak > 5 && peak < 58)
        peak = peak - 5;
    else
        peak = 53;

    return peak;
}

static int pico_peaks(
    struct pico_data *self,
    double *f, double *s, double *total_counts, double *sum_of_squares)
{
    *total_counts = 0;
    *sum_of_squares = 0;

    /* average over samples in each bucket */
    for (int k = 0; k < BUCKETS; ++k)
    {
        double sum = 0;
        int bucket_start = round(VALID_SAMPLES * (k * 1.0 / BUCKETS));
        for (int j = 0; j < SAMPLES_PER_BUCKET; ++j)
            sum += f[bucket_start + j + (int) (self->peak)];
        s[k] = sum;
        *total_counts += sum;
    }

    if (*total_counts <= 0)
        *total_counts = 1;

    double charge = self->charge;
    if (charge < 0)
        charge = 0;

    double perm[BUCKETS];
    for (int k = 0; k < BUCKETS; ++k)
    {
        perm[k] = s[k] / *total_counts * charge;
        *sum_of_squares += perm[k] * perm[k];
    }

    /* circular shift */
    for (int k = 0; k < BUCKETS; ++k)
        s[k] = perm[(k + (int) self->shift) % BUCKETS] + LOG_PLOT_OFFSET;

    return 0;
}

int pico_average(struct pico_data *self)
{
    for (int n = 0; n < HISTCHAN; ++n)
    {
        self->samples[n] = self->countsbuffer[n];
        self->fill[n] = self->samples[n] + LOG_PLOT_OFFSET;
    }

    self->counts_fill = 0.0;
    for (int n = 0; n < HISTCHAN; ++n)
        self->counts_fill += self->samples[n];

    /* all counts > 0 */
    double max_bin = 0.0;
    for (int n = 0; n < HISTCHAN; ++n)
    {
        if (self->samples[n] > max_bin)
            max_bin = self->samples[n];
    }

    self->flux = self->counts_fill / self->time * 1000 / self->freq * BUCKETS;
    self->max_bin = max_bin;
    self->peak = pico_detect_peak(self);

    if (self->charge > 0.01)
        self->nflux = self->flux / self->charge;
    else
        self->nflux = 0;


    /* 60 and 180 second averages */

    memcpy(&self->buffer60[self->index60][0], self->samples,
        HISTCHAN * sizeof(double));

    memcpy(&self->buffer180[self->index180][0], self->samples,
        HISTCHAN * sizeof(double));

    self->index60 = (self->index60 + 1) % BUFFERS_60;
    self->index180 = (self->index180 + 1) % BUFFERS_180;

    memset(self->samples60, 0, HISTCHAN * sizeof(double));
    memset(self->samples180, 0, HISTCHAN * sizeof(double));

    for (int k = 0; k < BUFFERS_60; ++k)
    {
        for (int n = 0; n < HISTCHAN; ++n)
            self->samples60[n] += self->buffer60[k][n];
    }

    for (int k = 0; k < BUFFERS_180; ++k)
    {
        for (int n = 0; n < HISTCHAN; ++n)
            self->samples180[n] += self->buffer180[k][n];
    }

    pico_peaks(self,
        self->samples, self->buckets, &self->counts_5, &self->socs_5);
    pico_peaks(self,
        self->samples60, self->buckets60, &self->counts_60, &self->socs_60);
    pico_peaks(self,
        self->samples180, self->buckets180, &self->counts_180, &self->socs_180);

    return 0;
}

int pico_measure(struct pico_data *self, int time)
{
    self->overflow = 0;

    PICO_CHECK(PH_ClearHistMem(DEVICE, BLOCK));
    PICO_CHECK(PH_StartMeas(DEVICE, time));

    while (1)
    {
        int done = 0;
        PICO_CHECK(done = PH_CTCStatus(DEVICE));
        if(done)
            break;
        usleep(0.01);
    }

    int Flags = 0;
    PICO_CHECK(PH_StopMeas(DEVICE));
    PICO_CHECK(PH_GetBlock(DEVICE, self->countsbuffer, BLOCK));
    PICO_CHECK(Flags = PH_GetFlags(DEVICE));

    if (Flags & FLAG_OVERFLOW)
        self->overflow = 1;

    return 0;
}

static int pico_open(struct pico_data *self)
{
    int Resolution = 0;
    int Countrate0 = 0;
    int Countrate1 = 0;
    int Offset0 = 0;

    char libversion[ERRBUF] = { 0 };
    char serial[ERRBUF] = { 0 };
    char model[ERRBUF] = { 0 };
    char version[ERRBUF] = { 0 };

    printf("PicoHarp Configuration:\n");
    printf("Offset    %d\n", self->Offset);
    printf("CFDLevel0 %d\n", self->CFDLevel0);
    printf("CFDLevel1 %d\n", self->CFDLevel1);
    printf("CFDZeroX0 %d\n", self->CFDZeroX0);
    printf("CFDZeroX1 %d\n", self->CFDZeroX1);
    printf("SyncDiv   %d\n", self->SyncDiv);
    printf("Range     %d\n", self->Range);

    PICO_CHECK(PH_GetLibraryVersion(libversion));
    printf("PH_GetLibraryVersion %s\n", libversion);

    PICO_CHECK(PH_OpenDevice(DEVICE, serial));
    printf("PH_OpenDevice %s\n", serial);

    PICO_CHECK(PH_Initialize(DEVICE, MODE_HIST));

    PICO_CHECK(PH_GetHardwareVersion(DEVICE, model, version));
    printf("PH_GetHardwareVersion %s %s\n", model, version);

    PICO_CHECK(PH_Calibrate(DEVICE));

    PICO_CHECK(PH_SetSyncDiv(DEVICE, self->SyncDiv));
    PICO_CHECK(PH_SetCFDLevel(DEVICE, 0, self->CFDLevel0));
    PICO_CHECK(PH_SetCFDLevel(DEVICE, 1, self->CFDLevel1));
    PICO_CHECK(PH_SetCFDZeroCross(DEVICE, 0, self->CFDZeroX0));
    PICO_CHECK(PH_SetCFDZeroCross(DEVICE, 1, self->CFDZeroX1));
    PICO_CHECK(Offset0 = PH_SetOffset(DEVICE, self->Offset));
    PICO_CHECK(PH_SetStopOverflow(DEVICE, 1, HISTCHAN-1));
    PICO_CHECK(PH_SetRange(DEVICE, self->Range));
    PICO_CHECK(Resolution = PH_GetResolution(DEVICE));

    sleep(1);

    Countrate0 = PH_GetCountRate(DEVICE, 0);
    Countrate1 = PH_GetCountRate(DEVICE, 1);

    printf("Resolution=%dps Countrate0=%d/s Countrate1=%d/s\n",
        Resolution, Countrate0, Countrate1);

    return 0;
}

void pico_init(struct pico_data *self)
{
    /* initialize defaults */
    self->pk_auto = 1;
    self->peak = 45;
    self->shift = 652;
    self->counts_5 = 1;
    self->counts_60 = 1;
    self->counts_180 = 1;
    self->socs_5 = 0;
    self->socs_60 = 0;
    self->socs_180 = 0;
    self->freq = 499652713;
    self->charge = 0;
    self->time = 5000;

    pico_open(self);
}

#ifdef PICO_TEST_MAIN
static struct pico_data p;

int main(void)
{
    p.Offset = 0;
    p.CFDLevel0 = 300;
    p.CFDLevel1 = 20;
    p.CFDZeroX0 = 10;
    p.CFDZeroX1 = 11;
    p.SyncDiv = 1;
    p.Range = 3;

    pico_init(&p);
    pico_measure(&p, 5000);
}
#endif
