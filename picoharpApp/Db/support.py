import os
import sys

EPICSDBBUILDER = os.environ['EPICSDBBUILDER']
if '/' in EPICSDBBUILDER:
    sys.path.append(EPICSDBBUILDER)
else:
    from pkg_resources import require
    require('epicsdbbuilder==%s' % EPICSDBBUILDER)


from epicsdbbuilder import *
InitialiseDbd(os.environ['EPICS_BASE'])
LoadDbdFile(os.path.join(os.environ['TOP'], 'dbd/picoharp.dbd'))

DEVICE = Parameter('DEVICE', 'Device name')
PORT = Parameter('PORT', 'Asyn port')

def asyn_addr(addr):
    return addr.lower()

def SetAsynAddr(addr):
    global asyn_addr
    asyn_addr = addr


def record_creator(record, dtype):
    def creator(name, **kargs):
        address = kargs.pop('address', None)
        if address is None:
            address = asyn_addr(name)
        return getattr(records, record)(name,
            DTYP = dtype, address = '@asyn($(PORT))%s' % address, **kargs)
    return creator

create_waveform = record_creator('waveform', 'asynFloat64ArrayIn')
def Waveform(name, length, **kargs):
    return create_waveform(name, NELM = length, FTVL = 'DOUBLE', **kargs)

aIn  = record_creator('ai', 'asynFloat64')
aOut = record_creator('ao', 'asynFloat64')
stringIn = record_creator('stringin', 'asynOctetRead')

def autosave(record, field = 'VAL', phase = 1):
    record.add_metadata('autosave %d %s' % (phase, field))
