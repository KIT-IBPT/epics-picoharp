import os, sys
import pkg_resources

pkg_resources.require('iocbuilder==3.45')

import iocbuilder
iocbuilder.ConfigureIOC(architecture = 'linux-x86_64')
from iocbuilder import ModuleVersion

ModuleVersion('pvlogging', '1-1-1')
ModuleVersion('asyn',      '4-21')
ModuleVersion('picoharp',  home = '..', use_name = False)

from iocbuilder.modules import picoharp
from iocbuilder.modules import pvlogging


ioc_name = 'BL01B-DI-IOC-12'

iocbuilder.EpicsEnvSet('EPICS_CA_MAX_ARRAY_BYTES', 2000000)

# Picoharp fill pattern monitor
current = 'SR-DI-DCCT-01:SIGNAL'
picoharp.PicoharpInstance('BL01B-DI-PICO-01', '1013107', current)

pvlogging.PvLogging()
iocbuilder.WriteNamedIoc('ioc', ioc_name)
