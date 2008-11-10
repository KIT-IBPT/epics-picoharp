shif = 3;
charge = 2.0;

ni = (0:58539) / 58540;
f = cos(ni) .* exp(ni);

c = round(linspace(0,58540,937));
c = c(1:end-1);
ix = reshape((shif+repmat(1:10,936,1)+repmat(c',1,10))',1,9360);
S = sum(reshape(f(ix),10,936));
total_counts = sum(S);
S = S / total_counts * charge;
S1 = pico_c(f, shif, charge);

assert(all(abs(S - S1)) < eps);


