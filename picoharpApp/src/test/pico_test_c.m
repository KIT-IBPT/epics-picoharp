pico_data.time = t;
pico_data.freq = frev;
pico_data.countsbuffer = buf;
pico_data.shift = shift_0;
pico_data.current = charge;
pico_data.pk_auto = pk_auto;

calllib('picopeaks', 'pico_average', pico_data);

fprintf('checking results\n');

subplot(3,1,1)
plot(buckets, 'ro')
hold on
plot(pico_data.buckets, 'bx')
hold off

subplot(3,1,2)
plot(bucketsf, 'ro')
hold on
plot(pico_data.buckets60, 'bx')
hold off

subplot(3,1,3)
plot(bucketsf2, 'ro')
hold on
plot(pico_data.buckets180, 'bx')
hold off

assert((pico_data.max_bin - max_bin) == 0, 'max_bin');
assert((pico_data.flux - CountsperTurn) == 0, 'flux');
assert((pico_data.counts_5 - total_counts) == 0,  'counts_5');
assert((pico_data.counts_60 - total_counts0) == 0, 'counts_60');
assert((pico_data.counts_180 - total_counts1) == 0, 'counts_180');
assert((pico_data.counts_fill - co) == 0, 'counts_fill');

assert(all(pico_data.buckets180 - bucketsf2 == 0));
assert(all(pico_data.buckets60 - bucketsf == 0));
assert(all(pico_data.buckets - buckets == 0));

fprintf('ok\n');

drawnow
