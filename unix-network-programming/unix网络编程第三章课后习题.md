#unix网络编程第三章课后习题

3.2 为什么要把readn 和 writen 由 void* 转化为char*

因为void* 不允许进行算数运算，但是char* 可以进行算数运算
	
3.3