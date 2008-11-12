#ifndef __PICOPEAKS_H__
#define __PICOPEAKS_H__

#define DEVICE 0
#define BLOCK 0

#define ERRBUF 1024
#define BUCKETS 936
#define LOG_PLOT_OFFSET 1e-8
#define VALID_SAMPLES 58540
#define BUFFER_SAMPLES 65536
#define SAMPLES_PER_BUCKET 10

typedef struct PICODATA
{

  /* EPICS */
  double buckets[BUCKETS];
  double buckets60[BUCKETS];
  double buckets180[BUCKETS];
  double fill[BUFFER_SAMPLES];
  double peak;
  double pk_auto;
  double flux;
  double time;
  double max_bin;
  double shift;
  double counts_fill;
  double counts_5;
  double counts_60;
  double counts_180;

  double freq; /* master oscillator (Hz)*/
  double current; /* DCCT current (mA) */

  /* where does charge come from ? */
  double samples[BUFFER_SAMPLES];
  double charge;
  
  /* PicoHarp sampling parameters (st.cmd) */
  int Offset;
  int nCFDZeroX0;
  int CFDLevel0;
  int CFDZeroX1;
  int CFDLevel1;
  int SyncDiv;
  int Range;
  int Tacq;

  /* PicoHarp acquisition results */
  int overflow;
  int totalcounts;
  int countsbuffer[BUFFER_SAMPLES];
  char errstr[ERRBUF];

} PicoData;

int pico_average(PicoData * self);
void pico_init(PicoData * self, int Offset, int CFDLevel0, int CFDLevel1, 
               int CFDZeroX1, int SyncDiv, int Range);
int pico_peaks(PicoData * self, int peak, double * f, double * s);
int pico_acquire(PicoData * self);
void pico_peaks_matlab(int shift, double charge, double * f, double * s);

#endif


