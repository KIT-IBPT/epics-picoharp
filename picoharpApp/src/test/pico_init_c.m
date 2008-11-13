% picoharp peaks in c

if libisloaded('picopeaks')
  unloadlibrary('picopeaks');
end

loadlibrary('picopeaks.so', 'picopeaks.h')

pico_data = libstruct('PICODATA');
