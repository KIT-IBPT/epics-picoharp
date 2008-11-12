LDLIBS=-lm -lphlib -Wl,-rpath=$(PWD)/.. -L$(PWD)/..
CFLAGS=-I$(PWD)/..

all: picopeaks.so picomeasure

picomeasure: ../picopeaks.c
	$(CC) -DPICO_TEST_MAIN=1 $(CFLAGS) $(LDLIBS) $< -o $@

picopeaks.so: ../picopeaks.c
	$(CC) -DPICO_TEST_MATLAB=1 $(CFLAGS) $(LDLIBS) -shared $< -o $@

clean: 
	rm picopeaks.so