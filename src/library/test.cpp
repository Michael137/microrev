#include <iostream>

int main( int argc, char** argv )
{
	int a[10]  = { 1 };
	char b[10] = { 0 };
	std::cout << &a << " " << ( &a ) + 1 << std::endl;
	std::cout << &b << " " << ( &b ) + 1 << std::endl;
	std::cout << ( ( &a ) + 1 ) - &a << " " << ( ( &b ) + 1 ) - &b << std::endl;
	return 0;
}
