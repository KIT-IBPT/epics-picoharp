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

#define DECLARE_RANGE(name, suffix...) \
    double name##_fast suffix; \
    double name##_5 suffix; \
    double name##_60 suffix; \
    double name##_180 suffix; \
    double name##_all suffix

struct pico_data
{
    int device;     /* Device ID used when talking to picoharp. */

    /* EPICS */

    /* These variables belong in a structure.  Unfortunately given the history
     * of this project doing this will take more time than we have. */

    DECLARE_RANGE(samples,      [HISTCHAN]);    // Raw samples from picoharp
    DECLARE_RANGE(raw_buckets,  [BUCKETS]);     // Uncorrected fill patter
    DECLARE_RANGE(fixup,        [BUCKETS]);     // Correction factor
    DECLARE_RANGE(max_fixup);                   // Maximum correction factor
    DECLARE_RANGE(buckets,      [BUCKETS]);     // Corrected fill pattern
    DECLARE_RANGE(socs);                        // Sum of Charge Squared
    DECLARE_RANGE(turns);                       // Turn count for capture


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

    int current_time;           // Time used for current capture

    /* PicoHarp sampling parameters (st.cmd) */
    double Offset;
    double CFDLevel0;
    double CFDZeroX0;
    double CFDZeroX1;
    double CFDLevel1;
    double SyncDiv;
    double Range;

    double deadtime;            // Detector deadtime for correction

    bool parameter_updated;     // Set if parameters should be reloaded
    double reset_accum;         // If set to 1 then accumulator will be reset

    /* PicoHarp acquisition results */
    int overflow;
    unsigned int countsbuffer[HISTCHAN];
    char errstr[ERRBUF];


    int index60;
    int index180;

    double buffer5[HISTCHAN];
    double buffer60[BUFFERS_60][HISTCHAN];
    double buffer180[BUFFERS_180][HISTCHAN];
    double turns_buffer5;
    double turns_buffer60[BUFFERS_60];
    double turns_buffer180[BUFFERS_180];
};

/* Called to initialise connection to picoharp with given serial number, returns
 * false if device not found. */
bool pico_init(struct pico_data *self, const char *serial);

/* Processes data captured by call to pico_measure. */
void pico_process_fast(struct pico_data *self);
void pico_process_5s(struct pico_data *self);

/* Communicates with picoharp to perform a measurement using currently
 * configured settings, reconfigures picoharp as necessary. */
bool pico_measure(struct pico_data *self, int time);

/* Called once during IOC initialisation, builds a table of available picoharp
 * devices. */
void scanPicoDevices(void);

#endif
