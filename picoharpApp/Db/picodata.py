from support import *

SUFFIX  = Parameter('SUFFIX', 'Suffix for name')
ASUFFIX = Parameter('ASUFFIX', 'Suffix for address')
EVENT   = Parameter('EVENT', 'Event number used for update signalling')
BUCKETS = Parameter('BUCKETS', 'Number of bunches in machine revolution')
PROFILE = Parameter('PROFILE', 'Number of bins in a single bucket')

SetRecordNames(lambda name: '%s:%s_%s' % (DEVICE, name, SUFFIX))
SetAsynAddr(lambda name: '%s_%s' % (name.lower(), ASUFFIX))

def add_events(constructor):
    def added(*args, **kargs):
        return constructor(SCAN = 'Event', EVNT = EVENT, *args, **kargs)
    return added

Waveform = add_events(Waveform)
aIn = add_events(aIn)


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


WriteRecords(sys.argv[1], Disclaimer(__file__))
