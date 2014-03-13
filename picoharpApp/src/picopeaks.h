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
    double samples_5[HISTCHAN];
    double samples_60[HISTCHAN];
    double samples_180[HISTCHAN];
    double buckets_5[BUCKETS];
    double buckets_60[BUCKETS];
    double buckets_180[BUCKETS];
    double socs_5;
    double socs_60;
    double socs_180;
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

bool pico_init(struct pico_data *self, const char *serial);
void pico_average(struct pico_data *self);
bool pico_measure(struct pico_data *self, int time);
void scanPicoDevices(void);

#endif
