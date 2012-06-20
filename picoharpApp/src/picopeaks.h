#ifndef __PICOPEAKS_H__
#define __PICOPEAKS_H__

#include <phdefin.h>

#define BLOCK 0

#define ERRBUF 1024
#define BUCKETS 936
#define LOG_PLOT_OFFSET 1e-8
#define VALID_SAMPLES 58540
#define SAMPLES_PER_BUCKET 10
#define BUFFERS_60 12
/*(60/5)*/
#define BUFFERS_180 36
/*(180/5)*/

struct pico_data
{
    int device;     /* Device ID used when talking to picoharp. */

    /* EPICS */
    double buckets[BUCKETS];
    double buckets60[BUCKETS];
    double buckets180[BUCKETS];
    double fill[HISTCHAN];
    double peak;
    double pk_auto;
    double flux;
    double nflux;
    double time;
    double max_bin;
    double shift;
    double counts_fill;
    double counts_5;
    double counts_60;
    double counts_180;
    double socs_5;      // Sum of counts squared
    double socs_60;
    double socs_180;

    double freq; /* master oscillator (Hz)*/
    double charge; /* DCCT charge (nC) */
    double dcct_alarm; /* DCCT monitor alarm status (!) */

    /* samples and 60 and 180 second buffers */

    double samples[HISTCHAN];
    double samples60[HISTCHAN];
    double samples180[HISTCHAN];

    int index60;
    int index180;

    /* PicoHarp sampling parameters (st.cmd) */
    int Offset;
    int nCFDZeroX0;
    int CFDLevel0;
    int CFDZeroX0;
    int CFDZeroX1;
    int CFDLevel1;
    int SyncDiv;
    int Range;

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
