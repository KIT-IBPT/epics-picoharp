#include <stdio.h>
#include <math.h>

#define BUCKETS 936
#define VALID_SAMPLES 58540
#define SAMPLES_PER_BUCKET 10

int picopeaks(int shift, double charge, double * f, double * s)
{
  int i, j, k;

  double total_counts = 0;
  for(k = 0; k < BUCKETS; k++)
    {
      double sum = 0;
      int bucket_start = round(VALID_SAMPLES * (k * 1.0 / BUCKETS));
      for(j = 0; j < SAMPLES_PER_BUCKET; ++j)
        {
          sum += f[bucket_start + j + shift];
        }
      s[k] = sum;
      total_counts += sum;
    }

  for(k = 0; k < BUCKETS; k++)
    {
      s[k] = s[k] / total_counts * charge;
    }

  return 0;
}

/*

#define BUCKETS 936
#define BUCKETS_PLUS_1 937
#define VALID_SAMPLES 58540
#define SAMPLES_PER_BUCKET 10

void rlinspace(double * r, double x, double y, int n)
{
  int i;
  for(i = 0; i < n; ++i)
    {
      r[i] = round(x + (y - x) * (i * 1.0 / (n-1)));
    }
}

int main()
{
  int i, j, k;

  double c[BUCKETS_PLUS_1];
  int ix[SAMPLES_PER_BUCKET * BUCKETS];
  double s0[SAMPLES_PER_BUCKET * BUCKETS];
  double s[BUCKETS];
  double f[VALID_SAMPLES];

  double charge = 2.0;
  int shift = 3;

  for(i = 0; i < VALID_SAMPLES; ++i)
    {
      f[i] = exp(i * 1.0 / VALID_SAMPLES) * cos(i * 1.0 / VALID_SAMPLES);
    }

  rlinspace(c, 0, VALID_SAMPLES, BUCKETS_PLUS_1);

  for(i = 0; i < BUCKETS; ++i)
    {
      printf("%f\n", c[i]);
    }
  
  for(k = 0; k < BUCKETS; k++)
    {
      for(j = 0; j < SAMPLES_PER_BUCKET; ++j)
        {
          ix[k * SAMPLES_PER_BUCKET + j] = c[k] + (j + 1) + shift;
        }
    }

  for(i = 0; i < BUCKETS * SAMPLES_PER_BUCKET; ++i)
    {
      printf("%d\n", ix[i]);
    }

  for(i = 0; i < BUCKETS * SAMPLES_PER_BUCKET; ++i)
    {
      s0[i] = f[ix[i]-1];
    }

  for(k = 0; k < BUCKETS; k++)
    {
      double sum = 0;
      for(j = 0; j < SAMPLES_PER_BUCKET; ++j)
        {
          sum += s0[k * SAMPLES_PER_BUCKET + j];
        }
      s[k] = sum;
    }
  
  double total_counts = 0;
  for(k = 0; k < BUCKETS; k++)
    {
      double sum = 0;
      int bucket_start = round(VALID_SAMPLES * (k * 1.0 / BUCKETS));
      for(j = 0; j < SAMPLES_PER_BUCKET; ++j)
        {
          sum += f[bucket_start + j + shift];
        }
      s[k] = sum;
      total_counts += sum;
    }
  for(k = 0; k < BUCKETS; k++)
    {
      s[k] = s[k] / total_counts * charge;
      printf("%32.32f\n", s[k]);
    }
  return 0;
}
*/
