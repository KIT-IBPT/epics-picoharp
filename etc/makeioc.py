import os, sys
import pkg_resources

pkg_resources.require('iocbuilder==3.45')

import iocbuilder
iocbuilder.ConfigureIOC(architecture = 'linux-x86_64')
from iocbuilder import ModuleVersion

ModuleVersion('asyn',      '4-22')
ModuleVersion('picoharp',  home = '..', use_name = False)

from iocbuilder.modules import picoharp


ioc_name = 'BL01B-DI-IOC-03'

iocbuilder.EpicsEnvSet('EPICS_CA_MAX_ARRAY_BYTES', 2000000)


# Picoharp fill pattern monitor
picoharp.Picoharp('PICO2', '1013107', 102, 0, 300, 20, 10, 11, 1, 3)
picoharp.PicoharpDb(DEVICE = 'BL01B-DI-PICO-02', PORT = 'PICO2', EVENT = 102)

picoharp.Picoharp('PICO1', '1001267', 101, 0, 300, 20, 10, 11, 1, 3)
picoharp.PicoharpDb(DEVICE = 'BL01B-DI-PICO-01', PORT = 'PICO1', EVENT = 101)

iocbuilder.WriteNamedIoc('ioc', ioc_name)
