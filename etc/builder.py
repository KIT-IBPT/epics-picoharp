# Builder definitions for picoharp

from iocbuilder import Device, Substitution
from iocbuilder.modules import asyn

class Picoharp(Device):
    Dependencies = (asyn.Asyn,)

    LibFileList = ['pico']
    DbdFileList = ['picoharp']

    def __init__(self,
            port, event, offset, CFDlevel0, CFDlevel1, CFDzeroX0, CFDzeroX1,
            syncdiv, range):
        self.__super.__init__()
        self.port       = port
        self.event      = event
        self.offset     = offset
        self.CFDlevel0  = CFDlevel0
        self.CFDlevel1  = CFDlevel1
        self.CFDzeroX0  = CFDzeroX0
        self.CFDzeroX1  = CFDzeroX1
        self.syncdiv    = syncdiv
        self.range      = range

    def Initialise(self):
        print 'initPicoAsyn("%(port)s", %(event)d, %(offset)d, ' \
            '%(CFDlevel0)d, %(CFDlevel1)d, %(CFDzeroX0)d, %(CFDzeroX1)d, ' \
            '%(syncdiv)d, %(range)d)' % self.__dict__


class PicoharpDb(Substitution):
    Dependencies = (Picoharp,)
    TemplateFile = 'picoharp.db'
    Arguments = ('DEVICE', 'PORT', 'EVENT')
