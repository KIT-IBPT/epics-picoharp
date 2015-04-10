from support import *
import support

EVENT_FAST  = Parameter('EVENT_FAST', 'Fast event code')
EVENT_5S    = Parameter('EVENT_5S', '5 second event code')
CURRENT     = Parameter('CURRENT', 'Stored beam charge')
BUCKETS_1   = Parameter('BUCKETS_1',
    'Number of bunches in machine revolution minus 1')
PROFILE     = Parameter('PROFILE', 'Number of bins in a single bucket')


SetTemplateRecordNames(DEVICE)


def add_events(constructor):
    def added(*args, **kargs):
        event = kargs.pop('EVNT', EVENT_FAST)
        return constructor(SCAN = 'Event', EVNT = event, *args, **kargs)
    return added

Waveform = add_events(Waveform)
aIn = add_events(aIn)
stringIn = add_events(stringIn)

ImportRecord(RecordName('BUCKETS_5')).add_alias(RecordName('BUCKETS'))
ImportRecord(RecordName('FLUX_5')).add_alias(RecordName('FLUX'))

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


# All of the configuration fields are written using ao records with autosave and
# automatic processing on startup.
def config(name, **fields):
    return autosave(aOut(name, PINI = 'YES', **fields))

config('TIME', EGU = 'ms', LOPR = 1, HOPR = 60000, DESC = 'Acquisition time')
config('SHIFT',
    EGU = 'bucket', LOPR = 0, HOPR = BUCKETS_1,
    DESC = 'Circular Shift')
config('SAMPLE_WIDTH',
    EGU = 'bins', LOPR = 1, HOPR = PROFILE,
    DESC = 'Sample width')

config('OFFSET', EGU = 'ps', HOPR = 1000000000, DESC = 'Offset in picoseconds')
config('CFDZEROX0', EGU = 'mV', HOPR = 20, DESC = 'Clock zero cross level')
config('CFDZEROX1', EGU = 'mV', HOPR = 20, DESC = 'Data zero cross level')
config('CFDLEVEL0', EGU = 'mV', HOPR = 800, DESC = 'Clock discriminator level')
config('CFDLEVEL1', EGU = 'mV', HOPR = 800, DESC = 'Data discriminator level')
config('SYNCDIV', HOPR = 8, DESC = 'Input rate divider')
config('RANGE', HOPR = 7, DESC = 'Measurement range code')
config('DEADTIME', HOPR = BUCKETS_1, DESC = 'Detector dead time')

aOut('RESET_ACCUM', DESC = 'Reset long term waveform')


WriteRecords(sys.argv[1], Disclaimer(__file__))
