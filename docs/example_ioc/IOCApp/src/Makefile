TOP = ../..
include $(TOP)/configure/CONFIG

PROD_IOC = IOC
DBD += IOC.dbd
IOC_DBD += base.dbd
IOC_DBD += asyn.dbd
IOC_DBD += picoharp.dbd
IOC_SRCS += IOC_registerRecordDeviceDriver.cpp
IOC_LIBS += pico
IOC_LIBS += asyn
IOC_LIBS += $(EPICS_BASE_IOC_LIBS)
IOC_SRCS += IOCMain.cpp

include $(TOP)/configure/RULES

