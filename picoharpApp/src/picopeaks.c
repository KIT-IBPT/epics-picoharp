#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#include <phlib.h>
#include <phdefin.h>

#include "picopeaks.h"

#define PICO_CHECK(call) \
{ \
int status = call; \
if(status < 0) \
{ \
  char errstr0[ERRBUF]; \
  PH_GetErrorString(errstr0, status); \
  sprintf(self->errstr, #call " %s", errstr0); \
  printf("%s\n", self->errstr); \
  PH_CloseDevice(DEVICE); \
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
pico_peaks (PicoData * self, int peak, double *f, double *s)
{

  int j, k;
  double total_counts = 0;
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
      total_counts += sum;
    }

  if (total_counts <= 0)
    {
      total_counts = 1;
    }

  for (k = 0; k < BUCKETS; ++k)
    {
      perm[k] = s[k] / total_counts * self->charge;
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
  int n;
  double sum_counts = 0.0;
  /* all counts > 0 */
  double max_bin = 0.0;

  for (n = 0; n < BUFFER_SAMPLES; ++n)
    {
      self->samples[n] = self->countsbuffer[n];
    }

  for (n = 0; n < BUFFER_SAMPLES; ++n)
    {
      sum_counts += self->samples[n];
    }

  for (n = 0; n < BUFFER_SAMPLES; ++n)
    {
      if (self->samples[n] > max_bin)
	{
	  max_bin = self->samples[n];
	}
    }

  self->flux = sum_counts / self->time * 1000 / self->freq * BUCKETS;
  self->max_bin = max_bin;

  for (n = 0; n < BUCKETS; ++n)
    {
      self->buckets60[n] = self->buckets60[n] + 1;
      self->buckets180[n] = self->samples[n];
    }

  int peak = pico_detect_peak (self);

  pico_peaks (self, peak, self->samples, self->buckets);
  /*picopeaks(self, self->samples, self->buckets60); */
  /* pico_peaks(self, peak, self->samples, self->buckets180); */

  return 0;
}

int
pico_acquire (PicoData * self)
{
  int n;
  for (n = 0; n < BUFFER_SAMPLES; ++n)
    {
      self->countsbuffer[n] = n * 10;
    }
  return 0;
}

void
pico_init (PicoData * self, int Offset, int CFDLevel0, int CFDLevel1,
	   int CFDZeroX1, int SyncDiv, int Range)
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

  self->Offset = Offset;
  self->CFDLevel0 = CFDLevel0;
  self->CFDLevel1 = CFDLevel1;
  self->CFDZeroX1 = CFDZeroX1;
  self->SyncDiv = SyncDiv;
  self->Range = Range;

}

int
pico_measure (PicoData * self)
{

  int Flags = 0;
  int n;

  self->overflow = 0;
  self->totalcounts = 0;

  PICO_CHECK (PH_ClearHistMem (DEVICE, BLOCK));
  PICO_CHECK (PH_StartMeas (DEVICE, self->Tacq));

  while (1)
    {
      if (PH_CTCStatus (DEVICE))
	break;
      /* need delay here */
    }

  PICO_CHECK (PH_StopMeas (DEVICE));

  PICO_CHECK (PH_GetBlock (DEVICE, self->countsbuffer, BLOCK));
  PICO_CHECK (Flags = PH_GetFlags (DEVICE));


  if (Flags & FLAG_OVERFLOW)
    {
      self->overflow = 1;
    }

  for (n = 0; n < HISTCHAN; ++n)
    {
      self->totalcounts += self->countsbuffer[n];
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

#ifdef PICO_TEST_MATLAB
/* matlab test case interface */
void
pico_peaks_matlab (int shift, double charge, double *f, double *s)
{
  PicoData self;
  int peak;
  self.pk_auto = 1;
  self.shift = shift;
  self.charge = charge;
  memcpy (self.samples, f, sizeof (self.samples));

  peak = pico_detect_peak (&self);
  pico_peaks (&self, peak, self.samples, self.buckets);

  memcpy (s, self.buckets, sizeof (self.buckets));
}
#endif

#ifdef PICO_TEST_MAIN
int
main ()
{

  PicoData p;
  p.Tacq = 100;
  int status = pico_open (&p);
  pico_measure (&p);
  printf ("pico_open: %s\n", p.errstr);
}
#endif
