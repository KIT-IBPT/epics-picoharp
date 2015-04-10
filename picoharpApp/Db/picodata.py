from support import *

SUFFIX = Parameter('SUFFIX', 'Suffix for name')
ASUFFIX = Parameter('ASUFFIX', 'Suffix for address')
EVENT = Parameter('EVENT', 'Event number used for update signalling')

SetRecordNames(lambda name: '%s:%s_%s' % (DEVICE, name, SUFFIX))
SetAsynAddr(lambda name: '%s_%s' % (name.lower(), ASUFFIX))

def add_events(constructor):
    def added(*args, **kargs):
        return constructor(SCAN = 'Event', EVNT = EVENT, *args, **kargs)
    return added

Waveform = add_events(Waveform)
aIn = add_events(aIn)


Waveform('SAMPLES', 65536, DESC = 'Raw picoharp signal')
Waveform('RAW_BUCKETS', 936, DESC = 'Uncorrected fill pattern')
Waveform('FIXUP', 936, DESC = 'Fill pattern correction factor')
Waveform('BUCKETS', 936, DESC = 'Corrected fill pattern')

aIn('MAX_FIXUP', PREC = 2, DESC = 'Maximum correction factor')
aIn('SOCS', EGU = 'nC^2', DESC = 'Total counts squared',
    HIGH = 490, HSV = 'MINOR', HIHI = 500, HHSV = 'MAJOR')
aIn('TURNS', EGU = 'turns', DESC = 'Number of turns captured')

Waveform('PROFILE', 62, DESC = 'Bucket profile')
aIn('PEAK', EGU = 'bins', LOPR = 1, HOPR = 63,
    LOLO = 6,   LLSV = 'MAJOR',
    LOW  = 12,  LSV  = 'MINOR',
    HIGH = 50,  HSV  = 'MINOR',
    HIHI = 57,  HHSV = 'MAJOR', DESC = 'Offset of first peak')
aIn('FLUX', PREC = 3, EGU = 'count/turn', DESC = 'Counts observed per turn')
aIn('NFLUX', PREC = 1, EGU = 'count/turn/nC',
    DESC = 'Normalised counts observed per turn')
aIn('TOTAL_COUNT', EGU = 'count', DESC = 'Total number of counts observed')

WriteRecords(sys.argv[1], Disclaimer(__file__))
