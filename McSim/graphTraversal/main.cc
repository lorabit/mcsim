#include <iostream>
#include <x86intrin.h>
int main()
{
	_mm_clwb((void*)&a);
	return 0;		    
}
