#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include<fcntl.h>
#define SW_SHM_MMAP_FILE_LEN  64 
typedef struct _swShareMemory_mmp
{
	size_t size;//标记共享内存的大小
	char mapfile[SW_SHM_MMAP_FILE_LEN];//共享内存使用的映射文件名
	int tmpfd;//内存映射文件的描述符
	int key;//共享内存使用的key值
	int shmid;//shm系列函数共享内存id
	void *mem;//共享内存起始地址
} swShareMemory;



//创建共享内存
void *swShareMemory_mmap_create(swShareMemory *object,size_t size,char *mapfile);

//释放共享内存
int swShareMemory_mmap_free(swShareMemory* object); 

void printShareMemory(swShareMemory* mem);

//共享内存
//int swShare_mmap_free(swShareMemory *object);

int main()
{
	swShareMemory object;//在栈中声明一个共享内存对象
	void* memory;
	size_t size = sizeof(swShareMemory);
	printf("size_t:%u\n",size);
	memory = swShareMemory_mmap_create(&object,size,NULL);
	if(memory == NULL)
	{
		printf("memory is NULL\n");
	}
	printShareMemory(&object);
	if(swShareMemory_mmap_free(&object)==0)
	{
		printf("free mem ok\n");
	}else{
		printf("free mem failed\n");
	}
	return 0;
}

void *swShareMemory_mmap_create(swShareMemory *object,size_t size,char *mapfile)
{
	//初始化参数
	void *mem;
	int tmpfd=-1;
	int flag=MAP_SHARED;
	bzero(object,sizeof(swShareMemory)); //置字符串前n格直接为0 包括\0
	if(mapfile == NULL)
	{
		mapfile = "/dev/zero";
	}	
	
	if((tmpfd=open(mapfile,O_RDWR)) < 0)
	{
		//打开文件描述符失败
		return NULL;
	}
	
	//赋值共享内存结构提中的文件名称
	strncpy(object->mapfile,mapfile,SW_SHM_MMAP_FILE_LEN);

	object->tmpfd = tmpfd;//文件描述符

	mem = mmap(NULL,size,PROT_READ | PROT_WRITE ,flag,tmpfd,0);

	if(!mem)
	{
		printf("mmap failed\n");
		return NULL;
	}else{
		object->size = size;
		object->mem = mem;
		return mem;
	}
}

int swShareMemory_mmap_free(swShareMemory* mem)
{
	return munmap(mem->mem,mem->size);
}

void printShareMemory(swShareMemory* mem)
{
	printf("size:%d\n",mem->size);
	printf("tmpfd:%d\n",mem->tmpfd);
	printf("key:%d\n",mem->key);
	printf("shmid:%d\n",mem->shmid);
	printf("mem_address:%d\n",mem->mem);
	printf("mapfile:%s\n",mem->mapfile);
}
