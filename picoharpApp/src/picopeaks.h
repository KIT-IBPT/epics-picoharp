#ifndef __PICOPEAKS_H__
#define __PICOPEAKS_H__

#include <phdefin.h>

#define BLOCK 0

#define ERRBUF 1024

#define BUFFERS_60  12  /*(60/5)*/
#define BUFFERS_180 36  /*(180/5)*/

#define DECLARE_RANGE(name, prefix...) \
    double prefix name##_fast; \
    double prefix name##_5; \
    double prefix name##_60; \
    double prefix name##_180; \
    double prefix name##_all

struct pico_data
{
    int device;     /* Device ID used when talking to picoharp. */

    /**************************************************************************/
    /* Configuration settings programmed during initialisation. */

    double range;               // Configures picoharp bin size
    int bucket_count;           // Number of bunches per machine revolution
    int valid_samples;          // Number of bins per revolution
    int samples_per_bucket;     // Number of bins per bunch
    double turns_per_sec;       // Machine revolutions per second

    /**************************************************************************/
    /* Values published through EPICS. */

    /* These variables belong in one or more structures.  Unfortunately given
     * the history of this project doing this will take too long to do. */
    DECLARE_RANGE(samples, *);  // Raw samples from picoharp
    DECLARE_RANGE(raw_buckets, *); // Uncorrected fill patter
    DECLARE_RANGE(fixup, *);    // Correction factor
    DECLARE_RANGE(max_fixup);   // Maximum correction factor
    DECLARE_RANGE(buckets, *);  // Corrected fill pattern
    DECLARE_RANGE(socs);        // Sum of Charge Squared
    DECLARE_RANGE(turns);       // Turn count for capture

    DECLARE_RANGE(profile, *);  // Single bunch profile
    DECLARE_RANGE(peak);        // Position of profile peak
    DECLARE_RANGE(flux);        // Counts per turn
    DECLARE_RANGE(nflux);       // Counts per turn per nC
    DECLARE_RANGE(total_count); // Total counts captured (= flux * turns)

    double max_bin;             // Maximum count in one bin
    double count_rate_0;        // Channel count rates
    double count_rate_1;
    double resolution;          // Bin width in ps

    char error[ERRBUF];
    char reset_time[ERRBUF];


    /**************************************************************************/
    /* Configuration settings programmable through EPICS. */

    /* PicoHarp sampling parameters */
    double offset;
    double cfdlevel0;
    double cfdzerox0;
    double cfdzerox1;
    double cfdlevel1;
    double syncdiv;

    double time;                // Sample time in ms for picoharp
    double shift;               // Pattern shift
    double sample_width;        // Number of points to measure for bins
    double deadtime;            // Detector deadtime for correction
    double reset_accum;         // If set to 1 then accumulator will be reset


    /**************************************************************************/
    /* Synchrotron state readbacks (delivered through EPICS). */

    double current;             // Stored beam current in mA
    double dcct_alarm;          // DCCT monitor alarm status (!)


    /**************************************************************************/
    /* Internal variables. */

    int current_time;           // Time used for current capture

    bool parameter_updated;     // Set if parameters should be reloaded

    /* PicoHarp acquisition results */
    int overflow;
    unsigned int countsbuffer[HISTCHAN];

    int *bucket_start;          // Start of each bucket in raw sample profile


    int index60;
    int index180;

    double buffer5[HISTCHAN];
    double buffer60[BUFFERS_60][HISTCHAN];
    double buffer180[BUFFERS_180][HISTCHAN];
    double bufferall[HISTCHAN];
    double turns_buffer5;
    double turns_buffer60[BUFFERS_60];
    double turns_buffer180[BUFFERS_180];
    double count_buffer5;
    double count_buffer60[BUFFERS_60];
    double count_buffer180[BUFFERS_180];
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
