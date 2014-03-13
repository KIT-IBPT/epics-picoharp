# Builder definitions for picoharp

from iocbuilder import Device, Substitution
from iocbuilder.modules import asyn

class Picoharp(Device):
    Dependencies = (asyn.Asyn,)

    LibFileList = ['pico']
    DbdFileList = ['picoharp']

    def __init__(self,
            port, serial, event, offset,
            CFDlevel0, CFDlevel1, CFDzeroX0, CFDzeroX1,
            syncdiv, range):
        self.__super.__init__()
        self.port       = port
        self.serial     = serial
        self.event      = event
        self.offset     = offset
        self.CFDlevel0  = CFDlevel0
        self.CFDlevel1  = CFDlevel1
        self.CFDzeroX0  = CFDzeroX0
        self.CFDzeroX1  = CFDzeroX1
        self.syncdiv    = syncdiv
        self.range      = range

    def InitialiseOnce(self):
        print 'scanPicoDevices'

    def Initialise(self):
        print 'initPicoAsyn("%(port)s", "%(serial)s", %(event)d, %(offset)d, ' \
            '%(CFDlevel0)d, %(CFDlevel1)d, %(CFDzeroX0)d, %(CFDzeroX1)d, ' \
            '%(syncdiv)d, %(range)d)' % self.__dict__


class PicoharpDb(Substitution):
    Dependencies = (Picoharp,)
    TemplateFile = 'picoharp.db'
    Arguments = ('DEVICE', 'PORT', 'EVENT')

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

    def __init__(self, device, serial, port = None, event = None):
        if port is None:
            port = 'PICO%d' % self.alloc_port()
        if event is None:
            event = self.alloc_event()

        Picoharp(port, serial, event, 0, 300, 100, 10, 5, 1, 3)
        for suffix in ['FAST', '5', '60', '180', 'ALL']:
            PicoharpData(
                DEVICE = device, PORT = port, EVENT = event,
                SUFFIX = suffix, ASUFFIX = suffix.lower())
        PicoharpDb(DEVICE = device, PORT = port, EVENT = event)
