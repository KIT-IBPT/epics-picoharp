PYTHON = /dls_sw/prod/tools/RHEL6-x86_64/defaults/bin/dls-python
EPICS_VERSION = 3.14.12.3

export EPICS_BASE = /dls_sw/epics/R$(EPICS_VERSION)/base

install: ioc/Makefile
	make -C ioc

ioc: ioc/Makefile

clean:
	rm -rf ioc *.pyc

ioc/Makefile: makeioc.py
	$(PYTHON) makeioc.py

.PHONY: clean install ioc
