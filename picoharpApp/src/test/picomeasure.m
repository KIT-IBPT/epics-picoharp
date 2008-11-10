function [buf, co] = picomeasure(t)
% [buf, co] = picomeasure(t)
% picomeasure test stub
HISTCHAN = 65536;
buf = uint32(rand(1,HISTCHAN));
co = sum(buf);
