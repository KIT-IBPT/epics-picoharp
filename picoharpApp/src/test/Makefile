LDLIBS=-lm
picopeaks.so: ../picopeaks.c
	$(CC) -shared $< -o $@

clean: 
	rm picopeaks.so