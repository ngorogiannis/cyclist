fields: next;

proc main(x, y)
	precondition: ls(y,x) * [a][1]ls(x,nil); 
	postcondition: ls(y,nil);
{
	z := x;
	assert (ls(y,z) * [a][1]ls(z,nil));
	(while z!=nil do
		z := z.next;
		assert([b'] < [a] : ls(y, z) * ls[b'](z,nil))
	od
	||
	while z!=nil do
		z := z.next;
		assert([b'] < [a] : ls(y, x) * ls[b'](z,nil))
	od)
}
