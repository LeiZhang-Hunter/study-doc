#include <stdint.h>
typedef struct{
	uint32_t id;
	uint32_t fd:24;//ռ��32λ����ֻ����24λ
	uint32_t reactor_id:8;//ռ��32λ����ֻ����8λ
}swSession;

int main()
{
	return 0;
}
