#include <stdio.h>
#include <stdlib.h>
char* return_word()
{
	//char* buf="hello world";
	char buf[] = "hello world";
	printf("%s\n", buf);
	return buf;
}

int main()
{
	char* string = return_word();
	printf("%s\n", string);
	exit(0);
}