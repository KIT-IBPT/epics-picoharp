function k = mangle(key)
% k = mangle(key)
% mangle key to make a valid structure fieldname
% allows using structs as associative arrays
k = strrep(key, ':', '_');
k = strrep(k, '-', '_');

