TOP = ../..
include $(TOP)/configure/CONFIG

SCRIPTS += ../stIOC.sh
SCRIPTS += stIOC.boot

include $(TOP)/configure/RULES

%.boot: ../%.cmd
	$(CP) $< $@
