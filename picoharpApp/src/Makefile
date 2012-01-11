TOP=../..

include $(TOP)/configure/CONFIG

USR_CFLAGS += -std=gnu99

USR_CFLAGS += -Werror
USR_CFLAGS += -Wall
USR_CFLAGS += -Wbad-function-cast
USR_CFLAGS += -Wmissing-noreturn
USR_CFLAGS += -Wcast-qual
USR_CFLAGS += -Wcomment
USR_CFLAGS += -Wcomments
USR_CFLAGS += -Wformat
USR_CFLAGS += -Wimplicit-int
USR_CFLAGS += -Wimport
USR_CFLAGS += -Wmissing-braces
USR_CFLAGS += -Wmissing-declarations
USR_CFLAGS += -Wmissing-prototypes
USR_CFLAGS += -Wmultichar
USR_CFLAGS += -Wnested-externs
USR_CFLAGS += -Wparentheses
USR_CFLAGS += -Wpointer-arith
USR_CFLAGS += -Wsign-compare
USR_CFLAGS += -Wunknown-pragmas
USR_CFLAGS += -Wtrigraphs
USR_CFLAGS += -Wwrite-strings

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
