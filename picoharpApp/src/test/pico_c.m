function s = pico_c(f, shif, charge)
% picoharp peaks in c
if ~libisloaded('picopeaks')
  loadlibrary('picopeaks.so', 'picopeaks.h')
end
sp = libpointer('doublePtr', zeros(1, 936));
calllib('picopeaks', 'picopeaks', shif, charge, f, sp);
s = sp.value;
pause(0.1);


