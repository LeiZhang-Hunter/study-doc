#include <stdint.h>
#include <stdio.h>
typedef struct _swMemoryPool
{
	void* object;
	void* (*alloc)(struct _swMemoryPool* pool,uint32_t size);
	void (*free)(struct _swMemoryPool* pool,void* ptr);
	void (*destroy)(struct _swMemoryPool* pool);
}swMemoryPool;

typedef struct _swFixedPool{
	void* memory;
	size_t size;//记录固定内存池的大小

	swFixedPool* head;
//	
	swFixedPool* tail;

	uint32_t slice_num;
	uint32_t slice_size;
	
	uint32_t slice_use;

	uint8_t shared;
	
}swFixedPool;

typedef struct _swFixedPool_slice{
	char data[0];

	struct swFixedPool_slice* next;
	struct swFixedPool_slice* pre;
	
	uint8_t lock;
}swFixedPool_slice;

typedef struct _swShareMemory_mmap{
	int size;
	char mapfile[64];
	int tmpfd;
	int key;
	int shmid;
	void* mem;
}swShareMemory;

swMemoryPool* swFixedPool_new(uint32_t slice_num,uint32_t slice_size,uint8_t shared)
{
	size_t size = slice_num*slice_size + slice_num*sizeof(swFixedPool_slice);	
	size_t alloc_size = size + sizeof(swFixedPool) + sizeof(swMemoryPool);
	void* memory = (shared == 1) ? sw_shm_malloc(alloc_size) : sw_malloc(alloc_size);

	swFixedPool *object = memory;
	
	memory += sizeof(swFixedPool);

	bzero(object,sizeof(swFixedPool));

	object->shared = shared;
	object->slice_num = slice_num;
	object->slice_size = slice_size;
	object->size = size;
	
	swMemoryPool *pool = memory;
	memory += sizeof(swMemoryPool);
	pool->object = object;
	pool->alloc = swFixedPool_alloc;		
	pool->free = swFixedPool_free;
	pool->destroy = swFixedPool_destroy;

	object->memory = memory;

	swFixedPool_init(object);
	return pool;
}

static void* swFixedPool_alloc(swMemory Pool,uint32_t size)
{
	swFixedPool* object = pool->object;//固定内存池的地址
	swFixedPool_slice* slice;
	slice = slice->head;
	if(slice->head == 0)
	{
		slice->lock = 1;

		object->use++;

		object->head = slice->next;
		
		slice->next->pre = NULL;

		object->tail->next = slice;

		slice->next = NULL;
		slice->pre = object->tail;
		object->tail = slice;
			
		return slice->data;	
	}else{
		return NULL;
	}	 
}

static void swFixedPool_free(swMemory* pool,void* ptr)
{
	swFixedPool* object = pool->object;
	swFixedPool_slice *slice;
	
	//断言
	assert(ptr > object->memory && ptr < object->memory + object->size);	
	slice = ptr - sizeof(swFixedPool_slice);
	
	if(slice->lock)
	{
		object->slice_use--;
	}

	slice->lock = 0;
	
	if(slice->pre == NULL)
	{
		return;
	}

	if(slice->next == NULL)
	{
		slice->pre->next = NULL;
		object->tail = slice->pre;
	}else{
		slice->pre->next = slice->next;
		slice->next->pre = slice->pre;
	}
	
	slice->pre = NULL;
	slice->next = object->head;
	object->head->pre = slice;
	object->head = slice;
}

static void swFixedPool_destroy(swMemoryPool* pool)
{
	swFixedPool* object = pool->object;
	if(object->shared)
	{
		sw_shm_free(object);
	}else{
		sw_free(object);
	}
}



int main()
{
	printf("%d",sizeof(swFixedPool_slice));
	return 0;
}
