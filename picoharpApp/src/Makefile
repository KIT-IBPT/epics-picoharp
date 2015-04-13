TOP=../..
include $(TOP)/configure/CONFIG

# This line is a temporary hack while $(TOP) is in the definition of PHLIB.
PHLIB := '$(shell cd $(PHLIB); pwd)'

PHLIB_64 = $(PHLIB)/Linux/PHLib_v3.0.0.1_64bit/library

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
USR_CFLAGS += -Wconversion

LIBRARY_IOC += pico

pico_SRCS += picoAsyn.c asynHelper.c picopeaks.c

DBD += picoharp.dbd
picoharp_DBD += base.dbd
picoharp_DBD += asyn.dbd
picoharp_DBD += pico.dbd


USR_CPPFLAGS += -I$(PHLIB_64)
USR_LDFLAGS += -L$(PHLIB_64)
USR_LDFLAGS += -Wl,-rpath,$(PHLIB_64)

pico_SYS_LIBS += phlib

include $(TOP)/configure/RULES
