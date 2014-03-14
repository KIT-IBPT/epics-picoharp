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


/* Accumulates raw samples into bins and returns total sum. */
static double sum_peaks(
    struct pico_data *self,
    int peak, int shift, const double samples[], double bins[])
{
    /* Compute accumulation window from programmed sample width and discovered
     * peak, but ensure window fits entirely within a single bin. */
    int start_offset = peak - (int) self->sample_width / 2;
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
        bins[(k + BUCKETS - shift) % BUCKETS] = sum;
        total += sum;
    }
    return total;
}


/* Apply pileup correction to bucket charges and convert bucket fill into
 * charge. */
static void correct_peaks(
    struct pico_data *self,
    int length, double turns, const double raw_buckets[], double fixup[],
    double *max_fixup, double buckets[])
{
    *max_fixup = 0;
    double total_counts = 0;
    for (int i = 0; i < BUCKETS; i ++)
    {
        /* Compute pileup factor by counting number of observed events preceding
         * this one.  We convert counts into counts per turn. */
        double sum = 0;
        for (int j = 1; j <= length; j ++)
            sum += raw_buckets[(i + BUCKETS - j) % BUCKETS];
        double sum_turn = sum / turns;

        /* Compute pileup correction. */
        fixup[i] = 1 / (1 - sum_turn);
        if (fixup[i] > *max_fixup)
            *max_fixup = fixup[i];

        /* Compute corrected count per bucket. */
        buckets[i] = raw_buckets[i] * fixup[i];
        total_counts += buckets[i];
    }

    /* Scale buckets by pileup correction and current scaling factor. */
    for (int i = 0; i < BUCKETS; i ++)
        if (total_counts > 0)
        {
            buckets[i] *= self->charge / total_counts;
            if (buckets[i] < LOG_PLOT_OFFSET)
                /* Fudge for EDM display. */
                buckets[i] = LOG_PLOT_OFFSET;
        }
        else
            buckets[i] = 0;
}


/* Compute profile by folding all buckets together. */
static void compute_profile(const double samples[], double profile[])
{
    memset(profile, 0, SAMPLES_PER_PROFILE * sizeof(double));
    for (int n = 0; n < BUCKETS; n++)
    {
        int ix = bucket_start[n];
        for (int s = 0; s < SAMPLES_PER_PROFILE; s++)
            profile[s] += samples[ix + s];
    }
}

/* Discover the peak of the profile. */
static int compute_peak(const double profile[])
{
    double max = 0;
    int peak = 0;
    for (int i = 0; i < SAMPLES_PER_PROFILE; i ++)
    {
        if (profile[i] > max)
        {
            max = profile[i];
            peak = i;
        }
    }
    return peak;
}

static double compute_max_bin(const double samples[])
{
    double max_bin = 0.0;
    for (int n = 0; n < HISTCHAN; n ++)
    {
        if (samples[n] > max_bin)
            max_bin = samples[n];
    }
    return max_bin;
}

/* Updates flux. */
static void compute_flux(
    struct pico_data *self, const double samples[], double turns,
    double *total_count, double *flux, double *nflux)
{
    *total_count = 0;
    for (int n = 0; n < HISTCHAN; ++n)
        *total_count += samples[n];
    *flux = *total_count / turns;

    if (self->charge > 0.001)
        *nflux = *flux / self->charge;
    else
        *nflux = 0;
}


static void accum_buffer(
    struct pico_data *self,
    int count, const double samples_in[], int *ix,
    double buffer[][HISTCHAN], double samples_out[],
    double turns_buffer[], double *turns)
{
    turns_buffer[*ix] = self->turns_5;
    memcpy(buffer[*ix], samples_in, HISTCHAN * sizeof(double));
    *ix = (*ix + 1) % count;

    memset(samples_out, 0, HISTCHAN * sizeof(double));
    for (int i = 0; i < count; i ++)
        for (int j = 0; j < HISTCHAN; j ++)
            samples_out[j] += buffer[i][j];

    *turns = 0;
    for (int i = 0; i < count; i ++)
        *turns += turns_buffer[i];
}

#define ACCUM_BUFFER(self, count) \
    accum_buffer(self, BUFFERS_##count, \
        self->samples_5, &self->index##count, \
        self->buffer##count, self->samples_##count, \
        self->turns_buffer##count, &self->turns_##count)


/* The monstrous list of parameters in this function should really be gathered
 * into one or more structures, but the current architecture makes this
 * exceptionally messy to do. */
static void process_pico_peaks(
    struct pico_data *self, double turns, const double samples[],
    double raw_buckets[], double fixup[], double *max_fixup,
    double buckets[], double *socs, double profile[], double *peak,
    double *flux, double *nflux, double *total_count)
{
    compute_flux(self, samples, turns, total_count, flux, nflux);

    compute_profile(samples, profile);
    *peak = compute_peak(profile);

    sum_peaks(self, (int) *peak, (int) self->shift, samples, raw_buckets);

    correct_peaks(
        self, (int) self->deadtime,
        turns, raw_buckets, fixup, max_fixup, buckets);

    *socs = 0;
    for (int k = 0; k < BUCKETS; ++k)
        *socs += buckets[k] * buckets[k];
}

#define PROCESS_PICO_PEAKS(self, period) \
    process_pico_peaks(self, \
        self->turns_##period, self->samples_##period, \
        self->raw_buckets_##period, \
        self->fixup_##period, &self->max_fixup_##period, \
        self->buckets_##period, &self->socs_##period, \
        self->profile_##period, &self->peak_##period, \
        &self->flux_##period, &self->nflux_##period, \
        &self->total_count_##period)


void pico_process_fast(struct pico_data *self)
{
    /* Capture raw data and convert into common format. */
    for (int n = 0; n < HISTCHAN; n ++)
        self->samples_fast[n] = self->countsbuffer[n];

    self->max_bin = compute_max_bin(self->samples_fast);
    self->turns_fast = 1e-3 * self->current_time * self->freq / BUCKETS;

    PROCESS_PICO_PEAKS(self, fast);

    /* Accumulate fast buffer into 5 second buffer. */
    for (int i = 0; i < HISTCHAN; i ++)
        self->buffer5[i] += self->samples_fast[i];
    self->turns_buffer5 += self->turns_fast;
}


void pico_process_5s(struct pico_data *self)
{
    memcpy(self->samples_5, self->buffer5, sizeof(self->samples_5));
    self->turns_5 = self->turns_buffer5;

    if (self->reset_accum != 0)
    {
        memset(self->samples_all, 0, sizeof(self->samples_all));
        self->turns_all = 0;
        self->reset_accum = 0;
    }

    /* Accumulate intermediate buffers. */
    ACCUM_BUFFER(self, 60);
    ACCUM_BUFFER(self, 180);

    for (int i = 0; i < HISTCHAN; i ++)
        self->samples_all[i] += self->samples_5[i];
    self->turns_all += self->turns_5;

    /* Process everything. */
    PROCESS_PICO_PEAKS(self, 5);
    PROCESS_PICO_PEAKS(self, 60);
    PROCESS_PICO_PEAKS(self, 180);
    PROCESS_PICO_PEAKS(self, all);

    /* Reset 5 second buffer. */
    memset(self->buffer5, 0, sizeof(self->buffer5));
    self->turns_buffer5 = 0;
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

    PICO_CHECK(PH_SetSyncDiv(self->device, (int) self->SyncDiv));
    PICO_CHECK(PH_SetInputCFD(
        self->device, 0, (int) self->CFDLevel0, (int) self->CFDZeroX0));
    PICO_CHECK(PH_SetInputCFD(
        self->device, 1, (int) self->CFDLevel1, (int) self->CFDZeroX1));
    PICO_CHECK(PH_SetOffset(self->device, (int) self->Offset));
    PICO_CHECK(PH_SetStopOverflow(self->device, 1, HISTCHAN-1));
    PICO_CHECK(PH_SetBinning(self->device, (int) self->Range));

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
    self->current_time = time;

    while (true)
    {
        int done = 0;
        PICO_CHECK(PH_CTCStatus(self->device, &done));
        if(done)
            break;
        usleep(10000);  // 10 ms poll
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
