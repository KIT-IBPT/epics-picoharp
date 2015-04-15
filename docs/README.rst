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
for a single PicoHarp.  Alas, configuring the PicoHarp device is relatively
complex.  Creating the IOC is automated through the IOC_Builder_ tool, but
unfortunately this has too many Diamond specific dependencies for practical use
elsewhere.

To configure a PicoHarp we must first perform some calculations.  The two basic
parameters are the bunch number (called :math:`buckets` in the code) and the
machine revolution frequency.  At Diamond we have:

..  math::
    buckets &= 936 \\
    f_{rev} &= 533820 \,\text{Hz}

From these numbers we need to compute the following three numbers to configure
the driver:

======================= ========================================================
`bin_size`              Control parameter (in range 0 to 7), determines PicoHarp
                        sample size in nanseconds via equation:

                        ..  math::
                            sample\_time = 4 \cdot 2^{bin\_size} \,\text{ps}

                        Compute this as the smallest number in the range 0 to 7
                        such that :math:`valid\_samples < 65536`.

`valid_samples`         Number of valid samples, must be no more than 65536,
                        determined from `bin_size` as:

                        ..  math::
                            \frac{10^{12}}{f_{rev} \cdot sample\_time}

`samples_per_bucket`    Lower bound on valid samples per machine bunch, computed
                        as

                        ..  math::
                            \lfloor{valid\_samples / buckets}\rfloor
======================= ========================================================

From these values the startup script and substitution files can now be computed.


Startup Script
~~~~~~~~~~~~~~

The following calls must be made before `iocInit`:

`scanPicoDevices`
    This discovers all connected PicoHarp devices and will print out the serial
    string associated with each device.  This must be called once before
    initialising any PicoHarp instance.

`initPicoAsyn`
    This takes the following 9 arguments, which we have just determined:

    ======================= ====================================================
    `asyn_port`             This is an arbitrary string used internally to
                            identify PicoHarp instance: a single IOC can operate
                            multiple PicoHarps if ports are assigned.

    `serial_string`         The serial code printed out by `scanPicoDevices` for
                            PicoHarp of interest should be passed here as a
                            string.

    `event_fast`            The driver uses two EPICS event codes internally.
    `event_5s`

    `buckets`               Number of bunches in the ring.

    `bin_size`              Number in range 0 to 7 corresponding to selected bin
                            size.

    `valid_samples`         Number of samples used for fill pattern measurement.

    `samples_per_bucket`    Samples per bucket as computed above.

    `f_rev`                 Machine revolution frequency in Hz.
    ======================= ====================================================


Database Substitutions
~~~~~~~~~~~~~~~~~~~~~~

One substitution instance for `db/picoharp.db` and for `db/picodata.db`
must be made.  Note that the array sizes in the substitutions *must* be correct
as otherwise a segmentation fault is quite likely to occur!

`picoharp.db`
    This must be instantiated once (per PicoHarp) with the following parameters:

    ======================= ====================================================
    `DEVICE`                Prefix used to name the PicoHarp instance.  Should
                            be the same for all substitution instances for a
                            single PicoHarp.

    `PORT`                  This must be identical to the `asyn_port` parameter
                            passed to `initPicoAsyn`.

    `CURRENT`               This should name an EPICS PV which provides an
                            and up to date reading of the machine beam current
                            in mA.  This will be used to scale the computed fill
                            pattern.

    `EVENT_FAST`            The `event_fast` value passed to `initPicoAsyn`
    `EVENT_5S`              The `event_5s` value passed to `initPicoAsyn`

    `BUCKETS_1`             Must be `buckets`-1.

    `PROFILE`               Must be `samples_per_bucket`.
    ======================= ====================================================

`picodata.db`
    This must be instantiated once (per PicoHarp) with the following parameters:

    ======================= ====================================================
    `DEVICE`                Should be the same as above.
    `PORT`                  Must be the same as above.
    `EVENT_FAST`            The `event_fast` value passed to `initPicoAsyn`
    `EVENT_5S`              The `event_5s` value passed to `initPicoAsyn`
    `BUCKETS`               Must be `buckets` as computed above.
    `PROFILE`               Must be `samples_per_bucket`
    ======================= ====================================================


References
----------

..  target-notes::


..  _PicoQuant: http://www.picoquant.com/products/category/tcspc-and-time-tagging-modules/picoharp-300-stand-alone-tcspc-module-with-usb-interface

..  _PHLib: http://www.picoquant.com/dl_software/PicoHarp300/PicoHarp300_SW_and_DLL_v3_0_0_1.zip

..  _EPICSdbbuilder: http://controls.diamond.ac.uk/downloads/python/epicsdbbuilder/

..  _IOC_Builder: http://controls.diamond.ac.uk/downloads/python/iocbuilder/
