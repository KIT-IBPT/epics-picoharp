from support import *
import support

EVENT_FAST  = Parameter('EVENT_FAST', 'Fast event code')
EVENT_5S    = Parameter('EVENT_5S', '5 second event code')
CURRENT     = Parameter('CURRENT', 'Stored beam charge')
BUCKETS     = Parameter('BUCKETS', 'Number of bunches in machine revolution')
BUCKETS_1   = Parameter('BUCKETS_1',
    'Number of bunches in machine revolution less 1')
PROFILE     = Parameter('PROFILE', 'Number of bins in a single bucket')


def add_events(constructor, default_event = EVENT_FAST):
    def added(*args, **kargs):
        event = kargs.pop('EVNT', default_event)
        return constructor(SCAN = 'Event', EVNT = event, *args, **kargs)
    return added


def picoharp_core():
    Waveform = add_events(support.Waveform)
    aIn      = add_events(support.aIn)
    stringIn = add_events(support.stringIn)


    aIn('MAX_BIN', EGU = 'counts',
        HIGH = 59000,   HSV  = 'MINOR',
        HIHI = 65535,   HHSV = 'MAJOR',
        DESC = 'Maximum bin count')

    aIn('COUNT_RATE_0', EGU = 'Hz', DESC = 'Master clock count rate')
    aIn('COUNT_RATE_1', EGU = 'Hz', DESC = 'Sample data count rate')
    aIn('RESOLUTION',  EGU = 'ps', DESC = 'Histogram bin width')
    support.stringIn('ERROR',
        SCAN = '1 second', DESC = 'Picoharp error string')

    stringIn('RESET_TIME',
        EVNT = EVENT_5S, DESC = "Time long term history last reset")

    dcct = aOut('CURRENT',
        DOL = CP(MS(ImportRecord(CURRENT))),
        OMSL = 'closed_loop', DESC = 'DCCT stored current readback')
    aOut('DCCT_ALARM',
        DOL = dcct.SEVR, OMSL = 'closed_loop', SCAN = '.1 second',
        DESC = 'DCCT alarm status')


def picoharp_config():
    # All of the configuration fields are written using ao records with autosave
    # and automatic processing on startup.
    def config(name, **fields):
        return autosave(aOut(name, PINI = 'YES', **fields))

    config('TIME', EGU = 'ms', LOPR = 1, HOPR = 60000,
        DESC = 'Acquisition time')
    config('SHIFT',
        EGU = 'bucket', LOPR = 0, HOPR = BUCKETS_1,
        DESC = 'Circular Shift')
    config('SAMPLE_WIDTH',
        EGU = 'bins', LOPR = 1, HOPR = PROFILE,
        DESC = 'Sample width')

    config('OFFSET', EGU = 'ps', HOPR = 1000000000,
        DESC = 'Offset in picoseconds')
    config('CFDZEROX0', EGU = 'mV', HOPR = 20, DESC = 'Clock zero cross level')
    config('CFDZEROX1', EGU = 'mV', HOPR = 20, DESC = 'Data zero cross level')
    config('CFDLEVEL0', EGU = 'mV', HOPR = 800,
        DESC = 'Clock discriminator level')
    config('CFDLEVEL1', EGU = 'mV', HOPR = 800,
        DESC = 'Data discriminator level')
    config('SYNCDIV', HOPR = 8, DESC = 'Input rate divider')
    config('RANGE', HOPR = 7, DESC = 'Measurement range code')
    config('DEADTIME', LOPR = 0, HOPR = BUCKETS_1, DESC = 'Detector dead time')

    aOut('RESET_ACCUM', DESC = 'Reset long term waveform')


def make_data_instance(SUFFIX, EVENT):
    SetRecordNames(lambda name: '%s:%s_%s' % (DEVICE, name, SUFFIX))
    SetAsynAddr(lambda name: '%s_%s' % (name.lower(), SUFFIX.lower()))

    Waveform = add_events(support.Waveform, EVENT)
    aIn      = add_events(support.aIn, EVENT)


    Waveform('SAMPLES', 65536, DESC = 'Raw picoharp signal')
    Waveform('RAW_BUCKETS', BUCKETS, DESC = 'Uncorrected fill pattern')
    Waveform('FIXUP', BUCKETS, DESC = 'Fill pattern correction factor')
    Waveform('BUCKETS', BUCKETS, DESC = 'Corrected fill pattern')

    aIn('MAX_FIXUP', PREC = 2, DESC = 'Maximum correction factor')
    aIn('SOCS', EGU = 'nC^2', DESC = 'Total counts squared',
        HIGH = 490, HSV = 'MINOR', HIHI = 500, HHSV = 'MAJOR')
    aIn('TURNS', EGU = 'turns', DESC = 'Number of turns captured')

    Waveform('PROFILE', PROFILE, DESC = 'Bucket profile')
    aIn('FLUX', PREC = 3, EGU = 'count/turn', DESC = 'Counts observed per turn')
    aIn('NFLUX', PREC = 1, EGU = 'count/turn/nC',
        DESC = 'Normalised counts observed per turn')
    aIn('TOTAL_COUNT', EGU = 'count', DESC = 'Total number of counts observed')

    peak = aIn('PEAK',
        EGU = 'bins', LOPR = 1, HOPR = PROFILE,
        LLSV = 'MAJOR', LSV  = 'MINOR', HSV  = 'MINOR', HHSV = 'MAJOR',
        DESC = 'Offset of first peak')
    # Computation of peak limits, done during IOC initialisation.
    records.calcout('PEAK_LOLO',
        PINI = 'YES', INPA = PROFILE, CALC = '0.1*A', OUT = peak.LOLO)
    records.calcout('PEAK_LOW',
        PINI = 'YES', INPA = PROFILE, CALC = '0.2*A', OUT = peak.LOW)
    records.calcout('PEAK_HIGH',
        PINI = 'YES', INPA = PROFILE, CALC = '0.8*A', OUT = peak.HIGH)
    records.calcout('PEAK_HIHI',
        PINI = 'YES', INPA = PROFILE, CALC = '0.9*A', OUT = peak.HIHI)


SetTemplateRecordNames(DEVICE)

ImportRecord(RecordName('BUCKETS_5')).add_alias(RecordName('BUCKETS'))
ImportRecord(RecordName('FLUX_5')).add_alias(RecordName('FLUX'))

picoharp_core()
picoharp_config()


make_data_instance("FAST", EVENT_FAST)
make_data_instance("5", EVENT_5S)
make_data_instance("60", EVENT_5S)
make_data_instance("180", EVENT_5S)
make_data_instance("ALL", EVENT_5S)


WriteRecords(sys.argv[1], Disclaimer(__file__))
