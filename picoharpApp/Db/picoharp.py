from support import *
import support

EVENT_FAST  = Parameter('EVENT_FAST', 'Fast event code')
EVENT_5S    = Parameter('EVENT_5S', '5 second event code')
# CHARGE      = Parameter('CHARGE', 'Stored beam charge')
CHARGE      = 'SR23C-DI-DCCT-01:CHARGE'
# FREQ        = Parameter('FREQ', 'Machine frequency')
FREQ        = 'LI-RF-MOSC-01:FREQ'

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
support.stringIn('ERROR', address = 'errstr',
    SCAN = '1 second', DESC = 'Picoharp error string')

stringIn('RESET_TIME',
    EVNT = EVENT_5S, DESC = "Time long term history last reset")

dcct = aOut('DCCT', address = 'charge',
    DOL = CP(MS(ImportRecord(CHARGE))),
    OMSL = 'closed_loop', DESC = 'DCCT charge')
aOut('DCCT:ALARM', address = 'dcct_alarm',
    DOL = dcct.SEVR, OMSL = 'closed_loop', SCAN = '.1 second',
    DESC = 'DCCT alarm status')
dcct = aOut('FREQ',
    DOL = CP(MS(ImportRecord(FREQ))),
    OMSL = 'closed_loop', DESC = 'Master oscillator frequency')


# All of the configuration fields are written using ao records with autosave and
# automatic processing on startup.
def config(name, **fields):
    return autosave(aOut(name, PINI = 'YES', **fields))

config('TIME',
    EGU = 'ms', LOPR = 1, HOPR = 60000, DESC = 'Acquisition time')
config('SHIFT',
    EGU = 'bucket', LOPR = 0, HOPR = 935, DESC = 'Circular Shift')
config('SAMPLE_WIDTH',
    EGU = 'bins', LOPR = 1, HOPR = 62, DESC = 'Sample width')
config('OFFSET', address = 'Offset',
    EGU = 'ps', HOPR = 1000000000, DESC = 'Offset in picoseconds')
config('CFDZEROX0', address = 'CFDZeroX0',
    EGU = 'mV', HOPR = 20, DESC = 'Clock zero cross level')
config('CFDZEROX1', address = 'CFDZeroX1',
    EGU = 'mV', HOPR = 20, DESC = 'Data zero cross level')
config('CFDLEVEL0', address = 'CFDLevel0',
    EGU = 'mV', HOPR = 800, DESC = 'Clock discriminator level')
config('CFDLEVEL1', address = 'CFDLevel1',
    EGU = 'mV', HOPR = 800, DESC = 'Data discriminator level')
config('SYNCDIV', address = 'SyncDiv',
    HOPR = 8, DESC = 'Input rate divider')
config('RANGE', address = 'Range',
    HOPR = 7, DESC = 'Measurement range code')
config('DEADTIME',
    HOPR = 936, DESC = 'Detector dead time')

aOut('RESET_ACCUM', DESC = 'Reset long term waveform')


WriteRecords(sys.argv[1], Disclaimer(__file__))
