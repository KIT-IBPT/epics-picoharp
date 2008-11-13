#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include <phlib.h>
#include <phdefin.h>

#include "picopeaks.h"

/* TODO return from function on error */

#define PICO_CHECK(call) \
{ \
int __status = call; \
if(__status < 0) \
{ \
  char __errstr[ERRBUF]; \
  PH_GetErrorString(__errstr, __status); \
  sprintf(self->errstr, "%s", __errstr); \
  printf(#call " %s\n", self->errstr); \
  PH_CloseDevice(DEVICE); \
  return -1; \
} \
}

/* where is this linked against? */
double round (double x);

double
matlab_mod (double x, double y)
{
  /* matlab real modulo */
  double n = floor (x / y);
  return x - n * y;
}

/* return index of first peak in picoharp buffer */
int
pico_detect_peak (PicoData * self)
{

  int i, peak;
  double pk = self->samples[0];
  int first_peak_auto = 1;

  if (self->pk_auto != 1)
    {
      return self->peak;
    }

  /* find first peak */
  for (i = 1; i < VALID_SAMPLES; ++i)
    {
      if (self->samples[i] > pk)
	{
	  pk = self->samples[i];
	  first_peak_auto = i + 1;
	}
    }

  /* peak magic */
  peak = round (matlab_mod (first_peak_auto, VALID_SAMPLES * 1.0 / BUCKETS));

  if (peak < 5)
    {
      if (peak < 1)
	{
	  peak = 1;
	}
    }
  else if (peak > 5 && peak < 58)
    {
      peak = peak - 5;
    }
  else
    {
      peak = 53;
    }

  return peak;

}

int
pico_peaks (PicoData * self, int peak, double *f, double *s, double *total_counts)
{

  int j, k;
  *total_counts = 0;
  double perm[BUCKETS];

  /* average over samples in each bucket */
  for (k = 0; k < BUCKETS; ++k)
    {
      double sum = 0;
      int bucket_start = round (VALID_SAMPLES * (k * 1.0 / BUCKETS));
      for (j = 0; j < SAMPLES_PER_BUCKET; ++j)
	{
	  sum += f[bucket_start + j + peak];
	}
      s[k] = sum;
      *total_counts += sum;
    }

  if (*total_counts <= 0)
    {
      *total_counts = 1;
    }

  for (k = 0; k < BUCKETS; ++k)
    {
      perm[k] = s[k] / *total_counts * self->current;
    }

  /* circular shift */
  for (k = 0; k < BUCKETS; ++k)
    {
      s[k] = perm[(k + (int) self->shift) % BUCKETS] + LOG_PLOT_OFFSET;
    }

  return 0;
}

int
pico_average (PicoData * self)
{

  int n, k;
  self->counts_fill = 0.0;
  /* all counts > 0 */
  double max_bin = 0.0;

  for (n = 0; n < HISTCHAN; ++n)
    {
      self->samples[n] = self->countsbuffer[n];
      self->fill[n] = self->samples[n] + LOG_PLOT_OFFSET;
    }

  for (n = 0; n < HISTCHAN; ++n)
    {
      self->counts_fill += self->samples[n];
    }

  for (n = 0; n < HISTCHAN; ++n)
    {
      if (self->samples[n] > max_bin)
	{
	  max_bin = self->samples[n];
	}
    }

  self->flux = self->counts_fill / self->time * 1000 / self->freq * BUCKETS;

  self->max_bin = max_bin;

  int peak = pico_detect_peak (self);

  /* 60 and 180 second averages */

  memcpy(&self->buffer60[self->index60][0], self->samples,
         HISTCHAN * sizeof(double));

  memcpy(&self->buffer180[self->index180][0], self->samples,
         HISTCHAN * sizeof(double));

  self->index60 = (self->index60 + 1) % BUFFERS_60;
  self->index180 = (self->index180 + 1) % BUFFERS_180;

  memset(self->samples60, 0, HISTCHAN * sizeof(double));
  memset(self->samples180, 0, HISTCHAN * sizeof(double));

  for(k = 0; k < BUFFERS_60; ++k)
    {
      for(n = 0; n < HISTCHAN; ++n) 
        {
          self->samples60[n] += self->buffer60[k][n];
        }
    }

  for(k = 0; k < BUFFERS_180; ++k)
    {
      for(n = 0; n < HISTCHAN; ++n) 
        {
          self->samples180[n] += self->buffer180[k][n];
        }
    }

  pico_peaks (self, peak, self->samples, self->buckets, &self->counts_5);
  pico_peaks (self, peak, self->samples60, self->buckets60, &self->counts_60);
  pico_peaks (self, peak, self->samples180, self->buckets180, 
              &self->counts_180);

  /*
  for (n = 0; n < BUCKETS; ++n)
    {
      self->buckets60[n] = self->buckets60[n] + 1;
      self->buckets180[n] = self->samples[n];
    }
  */
  return 0;
}

void
pico_init (PicoData * self)
{
  /* initialize defaults */
  self->pk_auto = 1;
  self->peak = 45;
  self->shift = 650;
  self->counts_5 = 1;
  self->counts_60 = 1;
  self->counts_180 = 1;
  self->freq = 499652713;
  self->current = 10;
  self->time = 5000;
  
  pico_open(self);
  
}

int
pico_measure (PicoData * self, int time)
{

  int Flags = 0;

  self->overflow = 0;

  PICO_CHECK (PH_ClearHistMem (DEVICE, BLOCK));
  PICO_CHECK (PH_StartMeas (DEVICE, time));

  while (1)
    {
      int done = 0;
      PICO_CHECK (done = PH_CTCStatus(DEVICE));
      if(done)
	break;
      usleep(0.01);
    }

  PICO_CHECK (PH_StopMeas (DEVICE));
  PICO_CHECK (PH_GetBlock (DEVICE, self->countsbuffer, BLOCK));
  PICO_CHECK (Flags = PH_GetFlags (DEVICE));

  if (Flags & FLAG_OVERFLOW)
    {
      self->overflow = 1;
    }

  return 0;
}

int
pico_open (PicoData * self)
{

  int Resolution = 0;
  int Countrate0 = 0;
  int Countrate1 = 0;
  int Offset0 = 0;

  char libversion[ERRBUF] = { 0 };
  char serial[ERRBUF] = { 0 };
  char model[ERRBUF] = { 0 };
  char version[ERRBUF] = { 0 };

  PICO_CHECK (PH_GetLibraryVersion (libversion));
  printf ("PH_GetLibraryVersion %s\n", libversion);

  PICO_CHECK (PH_OpenDevice (DEVICE, serial));
  printf ("PH_OpenDevice %s\n", serial);

  PICO_CHECK (PH_Initialize (DEVICE, MODE_HIST));

  PICO_CHECK (PH_GetHardwareVersion (DEVICE, model, version));
  printf ("PH_GetHardwareVersion %s %s\n", model, version);

  PICO_CHECK (PH_Calibrate (DEVICE));

  PICO_CHECK (PH_SetSyncDiv (DEVICE, self->SyncDiv));
  PICO_CHECK (PH_SetCFDLevel (DEVICE, 0, self->CFDLevel0));
  PICO_CHECK (PH_SetCFDLevel (DEVICE, 1, self->CFDLevel1));
  PICO_CHECK (PH_SetCFDZeroCross (DEVICE, 1, self->CFDZeroX1));
  PICO_CHECK (Offset0 = PH_SetOffset (DEVICE, self->Offset));
  PICO_CHECK (PH_SetStopOverflow (DEVICE, 1, HISTCHAN));
  PICO_CHECK (PH_SetRange (DEVICE, self->Range));
  PICO_CHECK (Resolution = PH_GetResolution (DEVICE));

  sleep (1);

  Countrate0 = PH_GetCountRate (DEVICE, 0);
  Countrate1 = PH_GetCountRate (DEVICE, 1);

  printf ("Resolution=%dps Countrate0=%d/s Countrate1=%d/s\n",
	  Resolution, Countrate0, Countrate1);

  return 0;

}

#ifdef PICO_TEST_MAIN
static PicoData p;
int
main ()
{
  pico_init (&p);
  pico_measure (&p, 5000);
  printf ("pico_open: %s\n", p.errstr);
}
#endif
