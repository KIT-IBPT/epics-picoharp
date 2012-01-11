TOP=../..

include $(TOP)/configure/CONFIG

USR_CFLAGS += -std=gnu99

USR_CFLAGS += -Werror
USR_CFLAGS += -Wall -Wextra
USR_CFLAGS += -Wno-unused-parameter
USR_CFLAGS += -Wno-missing-field-initializers
USR_CFLAGS += -Wundef
USR_CFLAGS += -Wshadow
USR_CFLAGS += -Wcast-align
USR_CFLAGS += -Wwrite-strings
USR_CFLAGS += -Wredundant-decls
USR_CFLAGS += -Wmissing-prototypes
USR_CFLAGS += -Wmissing-declarations
USR_CFLAGS += -Wstrict-prototypes

LIBRARY_IOC += pico

pico_SRCS += picoAsyn.c asynHelper.c picopeaks.c

DBD += picoharp.dbd
picoharp_DBD += base.dbd
picoharp_DBD += asyn.dbd
picoharp_DBD += pico.dbd

REALTOP := $(shell cd $(TOP); pwd)/picoharpApp/src
USR_LDFLAGS = -L$(REALTOP) -Wl,-rpath,$(REALTOP)

pico_SYS_LIBS += phlib

include $(TOP)/configure/RULES
