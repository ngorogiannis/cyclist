ls(x,y) * y->z * ls(z,nil) |- ls(x,z) * ls(z,nil)
[a] < [b] : ls[a](x,y) * y->z * ls(z,nil) |- ls[b](x,z) * ls(z,nil)
ls[a](x,y) * y->z * ls(z,nil) |- [a] < [b'] : ls[b'](x,z) * ls(z,nil)
[a] < [b] : ls[a](x,y) * y->z * ls[c](z,nil) |- ls[b](x,z) * ls[c](z,nil)
ls[a](x,y) * y->z * ls[c](z,nil) |- [a] < [b'], [c] < [d'] : ls[b'](x,z) * ls[d'](z,nil)
ls[a](x,y) * y->z * ls[c](z,nil) |- [a] < [b'], [d'] <= [c] : ls[b'](x,z) * ls[d'](z,nil)
