#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <phlib.h>
#include <phdefin.h>

#include "picopeaks.h"


#define LOG_PLOT_OFFSET     1e-20

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
            sprintf(self->error, "%s", __errstr); \
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


static double compute_pileup(
    struct pico_data *self, const double samples[], int bucket)
{
    int length = (int) self->deadtime;
    int shift = (int) self->shift;
    bucket = (bucket + shift) % BUCKETS;
    int start_bucket = (bucket + BUCKETS - length) % BUCKETS;
    int start = bucket_start[start_bucket];
    int end = bucket_start[bucket];

    double pileup = 0;
    for (int i = start; i != end; i = (i + 1) % VALID_SAMPLES)
        pileup += samples[i];
    return pileup;
}


/* Apply pileup correction to bucket charges and convert bucket fill into
 * charge. */
static void correct_peaks(
    struct pico_data *self,
    int length, double turns,
    const double samples[], const double raw_buckets[],
    double fixup[], double *max_fixup, double buckets[])
{
    *max_fixup = 0;
    double total_counts = 0;
    for (int i = 0; i < BUCKETS; i ++)
    {
        /* Compute pileup factor by counting number of observed events preceding
         * this one.  We convert counts into counts per turn. */
        double sum = compute_pileup(self, samples, i);

        /* Compute pileup correction. */
        fixup[i] = 1 / (1 - sum);
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

static unsigned int compute_max_bin(const unsigned int samples[])
{
    unsigned int max_bin = 0;
    for (int n = 0; n < HISTCHAN; n ++)
        if (samples[n] > max_bin)
            max_bin = samples[n];
    return max_bin;
}

static unsigned int compute_total_count(const unsigned int samples[])
{
    unsigned int total_count = 0;
    for (int n = 0; n < HISTCHAN; n ++)
        total_count += samples[n];
    return total_count;
}

/* Updates flux. */
static void compute_flux(
    struct pico_data *self,
    double turns, double total_count, double *flux, double *nflux)
{
    *flux = total_count / turns;
    if (self->charge > 0.001)
        *nflux = *flux / self->charge;
    else
        *nflux = 0;
}


/* The monstrous list of parameters in this function should really be gathered
 * into one or more structures, but the current architecture makes this
 * exceptionally messy to do. */
static void process_pico_peaks(
    struct pico_data *self, double turns, double samples[],
    double raw_buckets[], double fixup[], double *max_fixup,
    double buckets[], double *socs, double profile[], double *peak,
    double *flux, double *nflux, double total_count)
{
    compute_flux(self, turns, total_count, flux, nflux);

    compute_profile(samples, profile);
    *peak = compute_peak(profile);

    sum_peaks(self, (int) *peak, (int) self->shift, samples, raw_buckets);

    correct_peaks(
        self, (int) self->deadtime,
        turns, samples, raw_buckets, fixup, max_fixup, buckets);

    *socs = 0;
    for (int k = 0; k < BUCKETS; ++k)
        *socs += buckets[k] * buckets[k];

    /* Final fudge on raw samples for EDM display. */
    for (int n = 0; n < HISTCHAN; n ++)
        if (samples[n] < LOG_PLOT_OFFSET)
            samples[n] = LOG_PLOT_OFFSET;
}

#define PROCESS_PICO_PEAKS(self, period) \
    process_pico_peaks(self, \
        self->turns_##period, self->samples_##period, \
        self->raw_buckets_##period, \
        self->fixup_##period, &self->max_fixup_##period, \
        self->buckets_##period, &self->socs_##period, \
        self->profile_##period, &self->peak_##period, \
        &self->flux_##period, &self->nflux_##period, \
        self->total_count_##period)


static void accum_buffer(
    struct pico_data *self, int count, int *ix,
    double buffer[][HISTCHAN], double samples_out[],
    double turns_buffer[], double *turns,
    double count_buffer[], double *total_count)
{
    count_buffer[*ix] = self->total_count_5;
    turns_buffer[*ix] = self->turns_5;
    memcpy(buffer[*ix], self->buffer5, HISTCHAN * sizeof(double));
    *ix = (*ix + 1) % count;

    memset(samples_out, 0, HISTCHAN * sizeof(double));
    for (int i = 0; i < count; i ++)
        for (int j = 0; j < HISTCHAN; j ++)
            samples_out[j] += buffer[i][j];

    *turns = 0;
    *total_count = 0;
    for (int i = 0; i < count; i ++)
    {
        *turns += turns_buffer[i];
        *total_count += count_buffer[i];
    }
}

#define ACCUM_BUFFER(self, count) \
    accum_buffer(self, BUFFERS_##count, \
        &self->index##count, \
        self->buffer##count, self->samples_##count, \
        self->turns_buffer##count, &self->turns_##count, \
        self->count_buffer##count, &self->total_count_##count)

/* Converts buffer in samples to buffer in samples per turn. */
#define NORMALISE_SAMPLES(self, period) \
    for (int i = 0; i < HISTCHAN; i ++) \
        self->samples_##period[i] /= self->turns_##period


void pico_process_fast(struct pico_data *self)
{
    self->max_bin = compute_max_bin(self->countsbuffer);
    self->total_count_fast = compute_total_count(self->countsbuffer);
    self->turns_fast = 1e-3 * self->current_time * self->turns_per_sec;

    /* Accumulate fast buffer into 5 second buffer. */
    for (int i = 0; i < HISTCHAN; i ++)
        self->buffer5[i] += self->countsbuffer[i];
    self->turns_buffer5 += self->turns_fast;
    self->count_buffer5 += self->total_count_fast;

    /* Capture raw data and convert into common format. */
    for (int n = 0; n < HISTCHAN; n ++)
        self->samples_fast[n] = self->countsbuffer[n];
    NORMALISE_SAMPLES(self, fast);
    PROCESS_PICO_PEAKS(self, fast);
}

static void reset_accum(struct pico_data *self)
{
    memset(self->bufferall, 0, sizeof(self->samples_all));
    self->turns_all = 0;
    self->total_count_all = 0;
    self->reset_accum = 0;

    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    strftime(self->reset_time, ERRBUF, "%Y-%m-%d %H:%M:%S %z", &tm);
}

void pico_process_5s(struct pico_data *self)
{
    memcpy(self->samples_5, self->buffer5, sizeof(self->samples_5));
    self->turns_5 = self->turns_buffer5;
    self->total_count_5 = self->count_buffer5;

    if (self->reset_accum != 0)
        reset_accum(self);

    /* Accumulate intermediate buffers. */
    ACCUM_BUFFER(self, 60);
    ACCUM_BUFFER(self, 180);

    /* Accumulate total buffer. */
    for (int i = 0; i < HISTCHAN; i ++)
        self->bufferall[i] += self->buffer5[i];
    self->turns_all += self->turns_5;
    self->total_count_all += self->total_count_5;
    memcpy(self->samples_all, self->bufferall, sizeof(self->bufferall));

    /* Compute samples in events per turn. */
    NORMALISE_SAMPLES(self, 5);
    NORMALISE_SAMPLES(self, 60);
    NORMALISE_SAMPLES(self, 180);
    NORMALISE_SAMPLES(self, all);

    /* Process everything. */
    PROCESS_PICO_PEAKS(self, 5);
    PROCESS_PICO_PEAKS(self, 60);
    PROCESS_PICO_PEAKS(self, 180);
    PROCESS_PICO_PEAKS(self, all);

    /* Reset 5 second buffer. */
    memset(self->buffer5, 0, sizeof(self->buffer5));
    self->turns_buffer5 = 0;
    self->count_buffer5 = 0;
}


static bool pico_set_config(struct pico_data *self)
{
    printf("Setting PicoHarp Configuration:\n");
    printf("Offset    %g\n", self->offset);
    printf("CFDLevel0 %g\n", self->cfdlevel0);
    printf("CFDLevel1 %g\n", self->cfdlevel1);
    printf("CFDZeroX0 %g\n", self->cfdzerox0);
    printf("CFDZeroX1 %g\n", self->cfdzerox1);
    printf("SyncDiv   %g\n", self->syncdiv);
    printf("Range     %g\n", self->range);

    PICO_CHECK(PH_SetSyncDiv(self->device, (int) self->syncdiv));
    PICO_CHECK(PH_SetInputCFD(
        self->device, 0, (int) self->cfdlevel0, (int) self->cfdzerox0));
    PICO_CHECK(PH_SetInputCFD(
        self->device, 1, (int) self->cfdlevel1, (int) self->cfdzerox1));
    PICO_CHECK(PH_SetOffset(self->device, (int) self->offset));
    PICO_CHECK(PH_SetStopOverflow(self->device, 1, HISTCHAN-1));
    PICO_CHECK(PH_SetBinning(self->device, (int) self->range));

    PICO_CHECK(PH_GetResolution(self->device, &self->resolution));

    return true;
}


/* Captures data from instrument, does not process captured data. */
bool pico_measure(struct pico_data *self, int delay)
{
    if (self->parameter_updated)
    {
        self->parameter_updated = false;
        pico_set_config(self);
    }

    self->overflow = 0;

    PICO_CHECK(PH_ClearHistMem(self->device, BLOCK));
    PICO_CHECK(PH_StartMeas(self->device, delay));
    self->current_time = delay;

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

    /* initialize non zero defaults */
    self->sample_width = 1;
    self->time = 5000;
    self->reset_accum = 1;

    self->device = find_pico_device(serial);
    if (self->device < 0)
        return false;

    return pico_open(self);
}
