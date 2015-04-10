TOP=../..
include $(TOP)/configure/CONFIG

DB += picoharp.db
DB += picodata.db

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

export TOP
export EPICSDBBUILDER

$(COMMON_DIR)/%.db: ../%.py ../support.py
	$(PYTHON) $< $@

clean::
	rm -f *.pyc
