PYTHON = python3

export EPICS_BASE = /opt/epics/base-7.0.3

install: ioc/Makefile
	make -C ioc

ioc: ioc/Makefile

clean:
	rm -rf ioc *.pyc

ioc/Makefile: makeioc.py
	$(PYTHON) makeioc.py

.PHONY: clean install ioc
