#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int main()
{
	//初始化一个变量
	char* str=NULL;
	//分配一个存储空间
	str = (char*)calloc(10,sizeof(char));
	//在存储空间里copy字符串hello
	strcpy(str,"hello");
	//输出字符串
	printf("%sccc\n",(str));
	free(str);
	//
	//测试calloc初始完空间以后对元素进行了初始化
	int i;
	int* pn = (int*)calloc(10,sizeof(int));
	for(i=0;i<10;i++)
		printf("%d\n",pn[i]);
	printf("\n");
	free(pn);
	
	//开辟100个整形
	//  colloc与malloc类似,但是主要的区别是存储在已分配的内存空间中的值默认为0,使用malloc时,已分配的内存中可以是任意的值.
	int* pn = (int*)malloc(sizeof(int)*100);
	int i;
	for(i=0;i<100;i++)
	{
		printf("%d\n",pn[i]);
	}
	printf("\n");
	free(pn);
	
	return 0;
}
