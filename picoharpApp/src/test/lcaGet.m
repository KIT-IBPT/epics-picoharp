function value = lcaGet(pv)
global DATABASE
% lcaGet test stub
pv = mangle(pv);
if isfield(DATABASE, pv)
  value = DATABASE.(pv);
else
  error('PV not in database');
end
