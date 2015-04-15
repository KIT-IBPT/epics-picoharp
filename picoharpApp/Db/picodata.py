from support import *
import support

EVENT_FAST  = Parameter('EVENT_FAST', 'Fast event code')
EVENT_5S    = Parameter('EVENT_5S', '5 second event code')
BUCKETS     = Parameter('BUCKETS', 'Number of bunches in machine revolution')
PROFILE     = Parameter('PROFILE', 'Number of bins in a single bucket')


def add_events(constructor, EVENT):
    def added(*args, **kargs):
        return constructor(SCAN = 'Event', EVNT = EVENT, *args, **kargs)
    return added



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


make_data_instance("FAST", EVENT_FAST)
make_data_instance("5", EVENT_5S)
make_data_instance("60", EVENT_5S)
make_data_instance("180", EVENT_5S)
make_data_instance("ALL", EVENT_5S)

WriteRecords(sys.argv[1], Disclaimer(__file__))
