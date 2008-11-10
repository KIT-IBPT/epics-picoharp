if libisloaded('picopeaks')
  unloadlibrary('picopeaks');
end

shift_0 = 650;
shif = 3;
charge = 2.0;

f = cos((1:58540) / 100);

% first_peak_auto - floor(first_peak_auto ./ (58540/936.0)) * 58540/936.0

[pk, first_peak_auto] = max(f);
shif = round(mod(first_peak_auto, 58540/936) );
if  shif < 5
  disp('<5');
  shif = max(shif,1);
elseif (shif > 5 & shif < 58)
  shif = shif - 5;
else
  shif = 53;
end

% f = rand(1, 58540);
% f = 1:58540;

c = round(linspace(0,58540,937));
c = c(1:end-1);
ix = reshape((shif+repmat(1:10,936,1)+repmat(c',1,10))',1,9360);
S = sum(reshape(f(ix),10,936));
total_counts = sum(S);
Ch = S / total_counts * charge;
S1 = pico_c(f, shift_0, shif, charge);

buckets = circshift( Ch ,[1 -shift_0] ) + 1e-8 ;

plot(buckets, 'r')
hold on
plot(S1, 'b')
hold off

assert(all(abs(buckets - S1)) < eps);

