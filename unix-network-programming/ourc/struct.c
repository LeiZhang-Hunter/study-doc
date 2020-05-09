#include <stdint.h>
typedef struct{
	uint32_t id;
	uint32_t fd:24;//占了32位但是只能用24位
	uint32_t reactor_id:8;//占了32位但是只能用8位
}swSession;

int main()
{
	return 0;
}
