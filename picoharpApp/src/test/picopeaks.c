#include <stdio.h>
#include <math.h>

#define BUCKETS 936
#define LOG_PLOT_OFFSET 1e-8
#define VALID_SAMPLES 58540
#define SAMPLES_PER_BUCKET 10

double matlab_mod(double x, double y)
{
  /* matlab real modulo */
  double n = floor(x / y);
  return x - n * y;
}

int picopeaks(int shift, int peak, double charge, double * f, double * s)
{

  int i, j, k;
  double pk = f[0];
  int first_peak_auto = 1;
  double total_counts = 0;
  double perm[BUCKETS];

  /* find first peak */
  for(i = 1; i < VALID_SAMPLES; ++i)
    {
      if(f[i] > pk)
        {
          pk = f[i];
          first_peak_auto = i + 1;
        }
    }

  /* peak magic */
  peak = round(matlab_mod(first_peak_auto, 58540.0/936));

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
      perm[k] = s[k] / total_counts * charge;
    }
  
  /* circular shift */
  for(k = 0; k < BUCKETS; ++k) 
    {
      s[k] = perm[(k + shift) % BUCKETS] + LOG_PLOT_OFFSET;
    }
  
  return 0;
}

/* TODO
   1) thread for picoharp reading
   2) message queue for data
   3) event for signal
   4) proper device support? or something else... (genSub)
*/
