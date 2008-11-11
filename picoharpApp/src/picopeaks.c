#include <stdio.h>
#include <math.h>
#include <string.h>

#include "picopeaks.h"

/* where is this linked against? */
double round(double x);

double matlab_mod(double x, double y)
{
  /* matlab real modulo */
  double n = floor(x / y);
  return x - n * y;
}

/* return index of first peak in picoharp buffer */
int pico_detect_peak(PicoData * self)
{

  int i, peak;
  double pk = self->samples[0];
  int first_peak_auto = 1;
  
  if(self->pk_auto != 1)
    {
      return self->peak;
    }
  
  /* find first peak */
  for(i = 1; i < VALID_SAMPLES; ++i)
    {
      if(self->samples[i] > pk)
        {
          pk = self->samples[i];
          first_peak_auto = i + 1;
        }
    }
  
  /* peak magic */
  peak = round(matlab_mod(first_peak_auto, VALID_SAMPLES*1.0/BUCKETS));

  if(peak < 5)
    {
      if(peak < 1)
        {
          peak = 1;
        }
    }
  else if(peak > 5 && peak < 58)
    {
      peak = peak - 5;
    }
  else
    {
      peak = 53;
    }

  return peak;

}

int pico_peaks(PicoData * self, int peak, double * f, double * s)
{

  int j, k;
  double total_counts = 0;
  double perm[BUCKETS];

  /* average over samples in each bucket */
  for(k = 0; k < BUCKETS; ++k)
    {
      double sum = 0;
      int bucket_start = round(VALID_SAMPLES * (k * 1.0 / BUCKETS));
      for(j = 0; j < SAMPLES_PER_BUCKET; ++j)
        {
          sum += f[bucket_start + j + peak];
        }
      s[k] = sum;
      total_counts += sum;
    }

  if(total_counts <= 0)
    {
      total_counts = 1;
    }
  
  for(k = 0; k < BUCKETS; ++k)
    {
      perm[k] = s[k] / total_counts * self->charge;
    }
  
  /* circular shift */
  for(k = 0; k < BUCKETS; ++k) 
    {
      s[k] = perm[(k + (int)self->shift) % BUCKETS] + LOG_PLOT_OFFSET;
    }
  
  return 0;
}

int 
pico_average(PicoData * self)
{
  int n;

  double co = 0.0;

  double max_bin = 0.0;
  for(n = 0; n < BUFFER_SAMPLES; ++n)
    {
      co += self->samples[n];
    }

  for(n = 0; n < BUFFER_SAMPLES; ++n)
    {
      if(self->samples[n] > max_bin)
        {
          max_bin = self->samples[n];
        }
    }

  self->max_bin = max_bin;
  self->flux = co / self->time * 1000 / self->freq * BUCKETS;
  
  for(n = 0; n < BUCKETS; ++n)
    {
      self->buckets60[n] = self->buckets60[n] + 1;
    }

  int peak = pico_detect_peak(self);
  
  pico_peaks(self, peak, self->samples, self->buckets);
  /*picopeaks(self, self->samples, self->buckets60);*/
  pico_peaks(self, peak, self->samples, self->buckets180);
  
  return 0;
}

int 
pico_acquire(double * dest)
{
  int n;
  for(n = 0; n < BUFFER_SAMPLES; ++n)
    {
      dest[n] = n * 10;
    }
  return 0;
}

void 
pico_init(PicoData * self)
{
  /* initialize defaults */
  self->pk_auto = 1;
  self->peak = 45;
  self->shift = 650;
  self->counts_5 = 1;
  self->counts_60 = 1;
  self->counts_180 = 1;
  self->freq = 499652713;
  self->charge = 10;
  self->time = 5000;
}

/* matlab test case interface */
void pico_peaks_matlab(int shift, double charge, double * f, double * s)
{
  PicoData self;
  int peak;
  self.pk_auto = 1;
  self.shift = shift;
  self.charge = charge;
  memcpy(self.samples, f, sizeof(self.samples));
  
  peak = pico_detect_peak(&self);
  pico_peaks(&self, peak, self.samples, self.buckets);
  
  memcpy(s, self.buckets, sizeof(self.buckets));
}

/* CountsperTurn = co /t * 1000 / frev * 936 ; */

