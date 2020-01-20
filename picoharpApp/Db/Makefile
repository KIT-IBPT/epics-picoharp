TOP=../..
include $(TOP)/configure/CONFIG

DB += picoharp.db

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

export TOP
export EPICSDBBUILDER

$(COMMON_DIR)/%.db: ../%.py ../support.py
	$(PYTHON) $< $@

%.db.d:
	touch $@

CLEANS += *.pyc

ifndef BASE_3_15
clean::
	$(RM) -f $(CLEANS)
endif
