# Builder definitions for picoharp

from iocbuilder import Device, Substitution
from iocbuilder.modules import asyn

class Picoharp(Device):
    Dependencies = (asyn.Asyn,)

    LibFileList = ['pico']
    DbdFileList = ['picoharp']

    def __init__(self, port, serial, event_fast, event_5s):
        self.__super.__init__()
        self.port       = port
        self.serial     = serial
        self.event_fast = event_fast
        self.event_5s   = event_5s

    def InitialiseOnce(self):
        print 'scanPicoDevices'

    def Initialise(self):
        print 'initPicoAsyn(' \
            '"%(port)s", "%(serial)s", %(event_fast)d, %(event_5s)d)' % \
                self.__dict__


class PicoharpDb(Substitution):
    Dependencies = (Picoharp,)
    TemplateFile = 'picoharp.db'
    Arguments = ('DEVICE', 'PORT', 'EVENT_FAST', 'EVENT_5S')

class PicoharpData(Substitution):
    Dependecies = (Picoharp,)
    TemplateFile = 'picodata.db'
    Arguments = ('DEVICE', 'PORT', 'EVENT', 'SUFFIX', 'ASUFFIX')

class PicoharpInstance:
    pico_port = 0
    pico_event = 100

    @classmethod
    def alloc_port(cls):
        cls.pico_port += 1
        return cls.pico_port

    @classmethod
    def alloc_event(cls):
        cls.pico_event += 1
        return cls.pico_event

    def __init__(self, device, serial):
        port = 'PICO%d' % self.alloc_port()
        event_fast = self.alloc_event()
        event_5s   = self.alloc_event()

        Picoharp(port, serial, event_fast, event_5s)

        event = event_fast
        for suffix in ['FAST', '5', '60', '180', 'ALL']:
            PicoharpData(
                DEVICE = device, PORT = port, EVENT = event,
                SUFFIX = suffix, ASUFFIX = suffix.lower())
            event = event_5s

        PicoharpDb(
            DEVICE = device, PORT = port,
            EVENT_FAST = event_fast, EVENT_5S = event_5s)
