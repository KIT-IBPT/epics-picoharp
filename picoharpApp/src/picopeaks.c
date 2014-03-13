#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include <phlib.h>
#include <phdefin.h>

#include "picopeaks.h"


/* Variable recording the start of each bucket in the raw sample profile. */
static int bucket_start[BUCKETS];


/* TODO return from function on error */

#define PICO_CHECK(call) \
    { \
        int __status = call; \
        if(__status < 0) \
        { \
            char __errstr[ERRBUF]; \
            PH_GetErrorString(__errstr, __status); \
            sprintf(self->errstr, "%s", __errstr); \
            printf(#call " %s\n", __errstr); \
            return false; \
        } \
    }


/* Table of available Picoharp devices, initialised at startup. */
struct pico_device { bool available; char serial[8]; };
static struct pico_device pico_devices[MAXDEVNUM];


/* Builds table of available picoharp devices. */
void scanPicoDevices(void)
{
    printf("scanPicoDevices\n");

    char libversion[8];
    if (PH_GetLibraryVersion(libversion) == 0)
        printf("PH_GetLibraryVersion %s\n", libversion);
    else
        printf("Unable to interrogate library.\n");

    for (int dev = 0; dev < MAXDEVNUM; dev ++)
    {
        pico_devices[dev].available =
            PH_OpenDevice(dev, pico_devices[dev].serial) == 0;
        if (pico_devices[dev].available)
            printf("Found picoharp s/n %s (slot %d)\n",
                pico_devices[dev].serial, dev);
    }
}


/* Scans the table of available devices and returns the id of the device with
 * the matching serial number, or failing that, the first available device if
 * find_first is set, or -1 to indicate failure. */
static int find_pico_device(const char *serial)
{
    int first = -1;
    for (int dev = 0; dev < MAXDEVNUM; dev ++)
    {
        if (pico_devices[dev].available)
        {
            if (strcmp(serial, pico_devices[dev].serial) == 0)
            {
                pico_devices[dev].available = false;
                return dev;
            }
            if (first == -1)
                first = dev;
        }
    }

    /* If we drop through to here then we need to resort to returning the first
     * discovered device if possible. */
    if (first >= 0)
    {
        printf("Requested s/n %s, returning s/n %s instead\n",
            serial, pico_devices[first].serial);
        pico_devices[first].available = false;
        return first;
    }
    else
    {
        printf("Unable to find any available device for s/n %s\n", serial);
        return -1;
    }
}


/* return index of first peak in picoharp buffer */
static int pico_detect_peak(struct pico_data *self)
{
    double max = 0;
    int peak = 0;
    for (int i = 0; i < SAMPLES_PER_PROFILE; i ++)
    {
        if (self->profile[i] > max)
        {
            max = self->profile[i];
            peak = i;
        }
    }
    return peak;
}


/* Accumulates raw samples into bins and returns total sum. */
static double sum_peaks(struct pico_data *self, double *samples, double *bins)
{
    /* Compute accumulation window from programmed sample width and discovered
     * peak, but ensure window fits entirely within a single bin. */
    int start_offset = (int) self->peak - (int) self->sample_width / 2;
    int end_offset = start_offset + (int) self->sample_width;
    if (start_offset < 0)
        start_offset = 0;
    if (end_offset > SAMPLES_PER_PROFILE)
        end_offset = SAMPLES_PER_PROFILE;

    double total = 0;
    for (int k = 0; k < BUCKETS; ++k)
    {
        int start = bucket_start[k] + start_offset;
        int end = bucket_start[k] + end_offset;
        double sum = 0;
        for (int j = start; j < end; j ++)
            sum += samples[j];
        bins[k] = sum;
        total += sum;
    }
    return total;
}


static void pico_peaks(
    struct pico_data *self, double *f, double *s, double *sum_of_squares)
{
    double total_counts = sum_peaks(self, f, s);
    if (total_counts <= 0)
        total_counts = 1;

    double charge = self->charge;
    if (charge < 0)
        charge = 0;

    double perm[BUCKETS];
    *sum_of_squares = 0;
    for (int k = 0; k < BUCKETS; ++k)
    {
        perm[k] = s[k] / total_counts * charge;
        *sum_of_squares += perm[k] * perm[k];
    }

    /* circular shift */
    for (int k = 0; k < BUCKETS; ++k)
        s[k] = perm[(k + (int) self->shift) % BUCKETS] + LOG_PLOT_OFFSET;
}


/* Analyses captured data, does not communicate with instrument. */
void pico_average(struct pico_data *self)
{
    /* Capture the raw counts. */
    for (int n = 0; n < HISTCHAN; ++n)
        self->samples[n] = self->countsbuffer[n];
    /* Compute the bucket profile by summing samples over all buckets. */
    memset(self->profile, 0, sizeof(self->profile));
    for (int n = 0; n < BUCKETS; n++)
    {
        int ix = bucket_start[n];
        for (int s = 0; s < SAMPLES_PER_PROFILE; s++)
            self->profile[s] += self->samples[ix + s];
    }

    double counts_fill = 0.0;
    for (int n = 0; n < HISTCHAN; ++n)
        counts_fill += self->samples[n];

    /* all counts > 0 */
    double max_bin = 0.0;
    for (int n = 0; n < HISTCHAN; ++n)
    {
        if (self->samples[n] > max_bin)
            max_bin = self->samples[n];
    }

    self->flux = counts_fill / self->time * 1000 / self->freq * BUCKETS;
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
    for (int k = 0; k < BUFFERS_60; ++k)
        for (int n = 0; n < HISTCHAN; ++n)
            self->samples60[n] += self->buffer60[k][n];

    memset(self->samples180, 0, HISTCHAN * sizeof(double));
    for (int k = 0; k < BUFFERS_180; ++k)
        for (int n = 0; n < HISTCHAN; ++n)
            self->samples180[n] += self->buffer180[k][n];

    pico_peaks(self, self->samples, self->buckets, &self->socs_5);
    pico_peaks(self, self->samples60, self->buckets60, &self->socs_60);
    pico_peaks(self, self->samples180, self->buckets180, &self->socs_180);
}


static bool pico_set_config(struct pico_data *self)
{
    printf("Setting PicoHarp Configuration:\n");
    printf("Offset    %g\n", self->Offset);
    printf("CFDLevel0 %g\n", self->CFDLevel0);
    printf("CFDLevel1 %g\n", self->CFDLevel1);
    printf("CFDZeroX0 %g\n", self->CFDZeroX0);
    printf("CFDZeroX1 %g\n", self->CFDZeroX1);
    printf("SyncDiv   %g\n", self->SyncDiv);
    printf("Range     %g\n", self->Range);

    PICO_CHECK(PH_SetSyncDiv(self->device, self->SyncDiv));
    PICO_CHECK(PH_SetInputCFD(
        self->device, 0, self->CFDLevel0, self->CFDZeroX0));
    PICO_CHECK(PH_SetInputCFD(
        self->device, 1, self->CFDLevel1, self->CFDZeroX1));
    PICO_CHECK(PH_SetOffset(self->device, self->Offset));
    PICO_CHECK(PH_SetStopOverflow(self->device, 1, HISTCHAN-1));
    PICO_CHECK(PH_SetBinning(self->device, self->Range));

    PICO_CHECK(PH_GetResolution(self->device, &self->resolution));

    return true;
}


/* Captures data from instrument, does not process captured data. */
bool pico_measure(struct pico_data *self, int time)
{
    if (self->parameter_updated)
    {
        self->parameter_updated = false;
        pico_set_config(self);
    }

    self->overflow = 0;

    PICO_CHECK(PH_ClearHistMem(self->device, BLOCK));
    PICO_CHECK(PH_StartMeas(self->device, time));

    while (true)
    {
        int done = 0;
        PICO_CHECK(PH_CTCStatus(self->device, &done));
        if(done)
            break;
        usleep(0.01);
    }

    int Flags = 0;
    PICO_CHECK(PH_StopMeas(self->device));
    PICO_CHECK(PH_GetHistogram(self->device, self->countsbuffer, BLOCK));
    PICO_CHECK(PH_GetFlags(self->device, &Flags));

    int count_rate_0, count_rate_1;
    PICO_CHECK(PH_GetCountRate(self->device, 0, &count_rate_0));
    PICO_CHECK(PH_GetCountRate(self->device, 1, &count_rate_1));
    self->count_rate_0 = count_rate_0;
    self->count_rate_1 = count_rate_1;

    if (Flags & FLAG_OVERFLOW)
        self->overflow = 1;

    return true;
}

static bool pico_open(struct pico_data *self)
{
    PICO_CHECK(PH_Initialize(self->device, MODE_HIST));

    char model[16];
    char partnum[8];
    char version[8];
    PICO_CHECK(PH_GetHardwareInfo(self->device, model, partnum, version));
    printf("PH_GetHardwareInfo %s %s %s\n", model, partnum, version);

    PICO_CHECK(PH_Calibrate(self->device));

    return pico_set_config(self);
}

bool pico_init(struct pico_data *self, const char *serial)
{
    /* Initialise the bucket boundaries.  Doesn't matter if we do this more than
     * once, the result is the same. */
    for (int i = 0; i < BUCKETS; i ++)
        bucket_start[i] = (int) round((float) i * VALID_SAMPLES / BUCKETS);

    /* initialize defaults */
    self->peak = 45;
    self->sample_width = 10;
    self->shift = 652;
    self->socs_5 = 0;
    self->socs_60 = 0;
    self->socs_180 = 0;
    self->freq = 499652713;
    self->charge = 0;
    self->time = 5000;
    self->device = find_pico_device(serial);
    if (self->device < 0)
        return false;

    return pico_open(self);
}


#ifdef PICO_TEST_MAIN
static struct pico_data p;

int main(void)
{
    scanPicoDevices();
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
