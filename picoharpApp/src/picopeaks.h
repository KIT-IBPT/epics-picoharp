#ifndef __PICOPEAKS_H__
#define __PICOPEAKS_H__

#define BUCKETS 936
#define LOG_PLOT_OFFSET 1e-8
#define VALID_SAMPLES 58540
#define BUFFER_SAMPLES 65536
#define SAMPLES_PER_BUCKET 10

typedef struct PICODATA
{

  /* public members */
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

  double samples[BUFFER_SAMPLES];
  double charge;
  double freq;
  double current;

} PicoData;

int pico_average(PicoData * self);
void pico_init(PicoData * self);
int pico_peaks(PicoData * self, int peak, double * f, double * s);
int pico_acquire(double * dest);
void pico_peaks_matlab(int shift, double charge, double * f, double * s);

#endif


