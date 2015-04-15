EPICS Driver for PicoHarp 300
=============================

..  default-role:: literal



This EPICS driver supports the use of the PicoQuant PicoHarp 300 for fill
pattern measurement.


Dependencies
------------

To build this support module edit `configure/RELEASE` for the following
dependencies:

`EPICS_BASE`
    This module has been developed with EPICS version 3.14.12.3, but has no
    particular version dependencies.

`ASYN`
    Similary Asyn version 4.21 was used, but the version is unlikely to matter
    too much.

`PHLIB`
    The PicoHarp interface library can be downloaded from the PicoQuant web site
    at PicoQuant_ and the library used for development was downloaded from
    PHLib_.  Some further configuration will be needed, see `PicoHarp Library`_
    below.

`EPICSDBBUILDER`
    This tool is used to build the database templates and can be downloaded at
    EPICSdbbuilder_.  This symbol should be set to the directory containing the
    `epicsdbbuilder` module, this does not need to be installed separately.


PicoHarp Library
~~~~~~~~~~~~~~~~

You will need to ensure that the PicoHarp 300 instrument is configured for
remote USB operation, this is a separately purchased option.  You will also need
to configure your target system so that when the PicoHarp is connected its USB
device nodes are user writeable: the corresponding USB id is `0e0d:0003`.

The makefile for this module expects to find the three header files
`errorcodes.h`, `phdefin.h`, and `phlib.h` in the directory `$(PHLIB)/include`
and expects to find `phlib.so` with the name `libphlib.so` in `$(PHLIB)/lib`.  I
suggest that this library be installed manually in a suitable directory, and
that you don't attempt to run the `install` script provided with the library.


Creating an IOC
---------------

The `docs/example_ioc` directory contains a simple IOC which implements support
for a single PicoHarp.  Creating the IOC is automated through the IOC_Builder_
tool, but unfortunately this has too many Diamond specific dependencies for
practical use elsewhere.

To configure a PicoHarp we must first perform some calculations.  The two basic
parameters are the bunch number (called `buckets` in the code) and the machine
revolution frequency.  At Diamond we have:

..  math::
    buckets &= 936 \\
    f_{rev} &= 533820 \,\text{Hz}

From these numbers we need to compute the following three numbers to configure
the driver:

======================= ========================================================
`bin_size`              Control parameter (in range 0 to 7), determines PicoHarp
                        sample size in nanseconds via equation:

                        ..  math::
                            sample\_time = 4 \times 2^{bin\_size} \,\text{ps}

                        Compute this as the smallest number in the range 0 to 7
                        such that :math:`valid\_samples < 65536`.

`valid_samples`         Number of valid samples, must be no more than 65536,
                        determined from `bin_size` as:

                        ..  math::
                            \frac{10^{12}}{f_{rev} \times sample\_time}

`samples_per_bucket`    Lower bound on valid samples per machine bunch, computed
                        as

                        ..  math::
                            \lfloor{valid\_samples / buckets}\rfloor
======================= ========================================================

From these values the startup script and substitution file can now be computed.


Startup Script
~~~~~~~~~~~~~~

The following calls must be made before `iocInit`:

`scanPicoDevices`
    This discovers all connected PicoHarp devices and will print out the serial
    string associated with each device.  This must be called once before
    initialising any PicoHarp instance.

`initPicoAsyn`
    This takes the following 9 arguments, which we have just determined:

    ======================== ===================================================
    `asyn_port`              This is an arbitrary string used internally to
                             identify PicoHarp instance: a single IOC can
                             operate multiple PicoHarps if ports are assigned.

    `serial_string`          The serial code printed out by `scanPicoDevices`
                             for the PicoHarp of interest should be passed here
                             as a string.

    `event_fast`, `event_5s` The driver uses two EPICS event codes internally,
                             they must be unique to each PicoHarp instance.
                             Event codes 100 and 101 seem to work ok.

    `buckets`                Number of bunches in the ring.

    `bin_size`               Number in range 0 to 7 corresponding to selected
                             bin size, computed above

    `valid_samples`          Number of samples used for fill pattern
                             measurement, computed above

    `samples_per_bucket`     Samples per bucket as computed above.

    `f_rev`                  Machine revolution frequency in Hz.
    ======================== ===================================================


Database Substitutions
~~~~~~~~~~~~~~~~~~~~~~

One substitution instance for `db/picoharp.db` must be made for each PicoHarp
controlled by this IOC.  Note that the array sizes in the substitutions *must*
be correct as otherwise a segmentation fault is quite likely to occur!

`picoharp.db`
    Instantiate with the following parameters:

    =============== ============================================================
    `DEVICE`        Prefix used to name the PicoHarp instance.

    `PORT`          This must be identical to the `asyn_port` parameter passed
                    to `initPicoAsyn`.

    `CURRENT`       This should name an EPICS PV which provides an up to date
                    reading of the machine beam current in mA.  This will be
                    used to scale the computed fill pattern.

    `EVENT_FAST`    The `event_fast` value passed to `initPicoAsyn`
    `EVENT_5S`      The `event_5s` value passed to `initPicoAsyn`

    `BUCKETS`       Must be `buckets`, ie number of bunches per turn.
    `BUCKETS_1`     Must be `buckets`-1.

    `PROFILE`       Must be `samples_per_bucket`.
    =============== ============================================================


EPICS Interface
---------------

The directory `opi/sr` contains three EDM screens which can be used to control
and view the fill pattern computed by the PicoHarp.  The top level screen is
`fill.edl`, with `fillconfig.edl` used to edit configuration settings and
`fillwf.edl` to provide a view on the raw PicoHarp data.

The PVs provided by the PicoHarp are documented below.  All of the PVs have
names of the form *device*:*pv* where *device* is the `DEVICE` name configured
in the database instance.  In the documentation below the *device*: part is
omitted from each name.


Status PVs
~~~~~~~~~~

`MAX_BIN`
    Returns the maximum number of samples in any one sampling bin observed by
    the PicoHarp in the most recent data acquisition.

`COUNT_RATE_0`
    This should be equal to :math:`f_{rev}`, if not the triggering configuration
    for the PicoHarp is not set correctly.

`COUNT_RATE_1`
    Ideally this needs to be close to `COUNT_RATE_0` -- this counts the data
    rate of PicoHarp samples.

`RESOLUTION`
    Should be equal to :math:`4\times2^{bin\_size}`.

`RESET_TIME`
    This records the exact time when `RESET_ACCUM` was last processed.

`ERROR`
    Returns error string associated with last PicoHarp interface action.


Configuration PVs
~~~~~~~~~~~~~~~~~

The following PVs are used to control the PicoHarp configuration.

`TIME`
    Controls the acquisition rate in ms, should be set to a fraction of 5
    seconds.  The `FAST` periodic PVs will update at intervals determined by
    this setting.

`SHIFT`
    This is a circular shift in bins applied to the computed fill pattern used
    to correct for trigger timing offsets.

`SAMPLE_WIDTH`
    Determines how many bins around the centre of the computed `PROFILE`\_\
    *rate* will be used to compute the number of samples in each bunch.

`OFFSET`
    This value is passed to the `PH_SetOffset` function of the PicoHarp library,
    but I have been unable to determine what it does.  Leave this setting at
    zero.

`CFDZEROX0`, `CFDZEROX1`, `CFDLEVEL0`, `CFDLEVEL1`
    These control the zero crossing and input detect levels for the two input
    triggers for the PicoHarp.  Input 0 should be connected to the machine
    revolution clock.

`SYNCDIV`
    This value is passed to the `PH_SetSyncDiv` function of the PicoHarp library
    to control synchronisation to an external source.  Should probably be left
    at its default setting of 1.

`RANGE`
    This needs to be left at the `bin_size` parameter determined above.  This
    can be changed in order to capture raw data with differing PicoHarp bin
    sizes, but the computed fill pattern will be wrong.

    The picoharp bin size is equal to :math:`4\times2^{RANGE}` ps.

`DEADTIME`
    This should be set to the PicoHarp's dead time, measured in machine buckets,
    approximately 42 at 500 MHz.  This is used by the fill pattern pileup
    correction algorithm.

`RESET_ACCUM`
    Processing this PV will reset the history accumulated into the `ALL` PVs.


Periodic PVs
~~~~~~~~~~~~

All of the PVs listed here have names of the form *device*:*pv*\_\ *rate* where
*rate* is one of `FAST`, `5`, `60`, `180`, or `ALL`.  The PVs with a named
*rate* represent a rolling history as follows:

=================== ============================================================
`FAST`              PVs with this suffix update at the rate determined by the
                    `TIME` PV.  This should be set to an integral fraction of 5
                    seconds.

`5`, `60`, `180`    These PVs update at 5 second intervals and deliver data
                    integrated over the corresponding number of seconds.

`ALL`               These PVs also update at 5 seconds, delivering data
                    integrated since the PV `RESET_ACCUM` was last processed.
=================== ============================================================

`BUCKETS`\_\ *rate*
    This is the computed fill pattern, after pileup correction and fill
    rotation.

`SAMPLES`\_\ *rate*
    This is the raw histogram captured from the PicoHarp, scaled to counts per
    turn, and accumulated over the corresponding period.

`RAW_BUCKETS`\_\ *rate*
    This is the corresponding computed fill pattern after fill shift correction
    but before fill pattern pileup correction.

`FIXUP`\_\ *rate*
    This is the computed pileup correction factor for each bucket.

`MAX_FIXUP`\_\ *rate*
    This is the maximum pileup correction factor.  If this number is much larger
    than 1 then the data capture rate is too high.

`SOCS`\_\ *rate*
    Sum Of Charges Squared, in :math:`nC^2`.

`TURNS`\_\ *rate*
    Total number of turns captured in this sample.  The calculation of this
    value is rather approximate.

`PROFILE`\_\ *rate*
    This waveform shows the pattern of PicoHarp bins within a single bunch,
    averaged over the entire fill.  The peak of the bunch should be centred, as
    bucket measurement will only occur within this region.

    If possible fine tuning of the PicoHarp trigger should be done to keep the
    centre of this profile away from the edges.

`FLUX`\_\ *rate*
    Counts per turn: number of captured events per machine revolution.  If this
    number is too much larger than 1 then pileup will distort the captured data
    beyond correction abilities of `FIXUP`\_\ *rate*.

`TOTAL_COUNT`\_\ *rate*
    Total number of captured events in the sample period.

`PEAK`\_\ *rate*
    Offset of peak in `PROFILE`\_\ *rate*, used for computing fill pattern from
    raw sample data.



References
----------

..  target-notes::


..  _PicoQuant: http://www.picoquant.com/products/category/tcspc-and-time-tagging-modules/picoharp-300-stand-alone-tcspc-module-with-usb-interface

..  _PHLib: http://www.picoquant.com/dl_software/PicoHarp300/PicoHarp300_SW_and_DLL_v3_0_0_1.zip

..  _EPICSdbbuilder: http://controls.diamond.ac.uk/downloads/python/epicsdbbuilder/

..  _IOC_Builder: http://controls.diamond.ac.uk/downloads/python/iocbuilder/
