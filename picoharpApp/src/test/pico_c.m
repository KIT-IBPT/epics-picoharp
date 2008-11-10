function s = pico_c(f, shift_0, shif, charge)
% picoharp peaks in c
if ~libisloaded('picopeaks')
  loadlibrary('picopeaks.so', 'picopeaks.h')
end
sp = libpointer('doublePtr', zeros(1, 936));
calllib('picopeaks', 'picopeaks', shift_0, shif, charge, f, sp);
s = sp.value;



