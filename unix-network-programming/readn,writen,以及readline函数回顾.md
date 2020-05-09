####readn,writen,以及readline函数回顾

readn函数

	ssize_t readn(int fd,void* vptr,size_t n)
	{
		size_t nleft;
		ssize_t nread;
		
		char* ptr = vptr;

		while(nleft > 0)
		{
			if((nread = read(fd,ptr,nleft) <= 0))
			{
				if(errno == EINTR)
				{
					nread = 0;
				}else if(nread == 0){
					break;
				}else{
					return -1;
				}
			}

			nleft -= nread;
			ptr+= nread;
		}

		retun n-nleft;
	}

解释:

由于套接字缓冲区内存有限，我们发送数据过大，可能不会一次性全部冲刷到套接字的缓冲区所以我们一次性是读不完的，所以需要循环读取，当我们又没读完的数据nleft 比 0 大的时候会继续去读取，如果中间发生了Enter错误程序并不会去推出，而是继续读取，EINTR一般表示捕获到了信号中断

同理我们在继续来写writen 函数,套接字区域有限也不可能一次性都写完，所以我们也需要继续写入


 	int writen(int fd,void *vptr,size_t n)
	{
		size_t nleft;
		ssize_t = nwrite;

		nleft = n;

		char *ptr = vptr;
		
		while(nleft > 0)
		{
			if((nwrite = write(fd,ptr,nleft)) <= 0)
			{
				if(nwrite < 0 &&errno == EINTR)
				{
					nwrite = 0;
				}else{
					return -1;
				}
			}

			nleft = nleft - nwrite;

			ptr+= nleft;
		}

		return n;
	}

最后一个readline函数 可以读取一行内容，我们需要和readn一起用，因为read函数是不可靠的在收到信号中断的情况下会退出

	int readline(fd,void* vptr,size_t len)
	{
		size_t nleft;

		ssize_t rc,n;

		

		for(n = 1;n<len;n++)
		{
			if((rc = readn(fd,&c)) == 1)
			{
				*ptr++ = c;
				if(c == "\n")
				{
					break;
				}
			}else if(rc == 0)
			{
				*ptr = 0;
				return n-1;
			}else
				return -1;
		}

		*ptr = 0;
		return n;
	}