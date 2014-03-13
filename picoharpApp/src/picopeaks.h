#ifndef __PICOPEAKS_H__
#define __PICOPEAKS_H__

#include <phdefin.h>

#define BLOCK 0

#define ERRBUF 1024
#define BUCKETS 936
#define LOG_PLOT_OFFSET 1e-8
#define VALID_SAMPLES 58540
#define SAMPLES_PER_PROFILE 62  /* floor(VALID_SAMPLES/BUCKETS) */
#define BUFFERS_60 12
/*(60/5)*/
#define BUFFERS_180 36
/*(180/5)*/

struct pico_data
{
    int device;     /* Device ID used when talking to picoharp. */

    /* EPICS */

    double samples_fast[HISTCHAN];  // Samples and 60, 180 second buffers
    double samples_5[HISTCHAN];
    double samples_60[HISTCHAN];
    double samples_180[HISTCHAN];
    double samples_all[HISTCHAN];

    double raw_buckets_fast[BUCKETS];   // Raw buckets
    double raw_buckets_5[BUCKETS];
    double raw_buckets_60[BUCKETS];
    double raw_buckets_180[BUCKETS];
    double raw_buckets_all[BUCKETS];

    double fixup_fast[BUCKETS];     // Correction factor
    double fixup_5[BUCKETS];
    double fixup_60[BUCKETS];
    double fixup_180[BUCKETS];
    double fixup_all[BUCKETS];

    double max_fixup_fast;          // Maxium correction factor
    double max_fixup_5;
    double max_fixup_60;
    double max_fixup_180;
    double max_fixup_all;

    double buckets_fast[BUCKETS];   // Corrected buckets
    double buckets_5[BUCKETS];
    double buckets_60[BUCKETS];
    double buckets_180[BUCKETS];
    double buckets_all[BUCKETS];

    double socs_fast;               // Sum of counts squared
    double socs_5;
    double socs_60;
    double socs_180;
    double socs_all;

    double profile[SAMPLES_PER_PROFILE];
    double peak;
    double flux;
    double nflux;
    double time;
    double max_bin;
    double shift;
    double sample_width;
    double count_rate_0;    // Channel count rates
    double count_rate_1;
    double resolution;

    double freq; /* master oscillator (Hz)*/
    double charge; /* DCCT charge (nC) */
    double dcct_alarm; /* DCCT monitor alarm status (!) */


    int index60;
    int index180;

    /* PicoHarp sampling parameters (st.cmd) */
    double Offset;
    double CFDLevel0;
    double CFDZeroX0;
    double CFDZeroX1;
    double CFDLevel1;
    double SyncDiv;
    double Range;

    bool parameter_updated;     // Set if parameters should be reloaded
    double reset_accum;         // If set to 1 then accumulator will be reset

    /* PicoHarp acquisition results */
    int overflow;
    unsigned int countsbuffer[HISTCHAN];
    char errstr[ERRBUF];

#ifdef PICO_TEST_MATLAB_HEADER

    double buffer60[786432];
    double buffer180[2359296];

#else

    double buffer60[BUFFERS_60][HISTCHAN];
    double buffer180[BUFFERS_180][HISTCHAN];

#endif
};

/* Called to initialise connection to picoharp with given serial number, returns
 * false if device not found. */
bool pico_init(struct pico_data *self, const char *serial);

/* Processes data captured by call to pico_measure. */
void pico_average(struct pico_data *self);

/* Communicates with picoharp to perform a measurement using currently
 * configured settings, reconfigures picoharp as necessary. */
bool pico_measure(struct pico_data *self, int time);

/* Called once during IOC initialisation, builds a table of available picoharp
 * devices. */
void scanPicoDevices(void);

#endif
