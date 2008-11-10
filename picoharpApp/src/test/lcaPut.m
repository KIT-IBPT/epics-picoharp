function lcaPut(pv, value)
% lcaPut test stub
global DATABASE
pv = mangle(pv);
DATABASE.(pv) = value;

