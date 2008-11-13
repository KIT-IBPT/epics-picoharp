LDLIBS=-lm -lphlib -Wl,-rpath=$(PWD)/.. -L$(PWD)/..
CFLAGS=-I$(PWD)/.. -g

all: picopeaks.so picomeasure picopeaks.h

picopeaks.h: ../picopeaks.h
	cpp -DPICO_TEST_MATLAB_HEADER=1 -E -I.. $< > $@

picomeasure: ../picopeaks.c
	$(CC) -DPICO_TEST_MAIN=1 $(CFLAGS) $(LDLIBS) $< -o $@

picopeaks.so: ../picopeaks.c
	$(CC) $(CFLAGS) $(LDLIBS) -shared $< -o $@

clean: 
	rm picopeaks.so