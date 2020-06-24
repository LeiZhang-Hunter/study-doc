#Buffer的功能实现

muduo的buffer使用的是一块连续内存 vector<char> 类型的有点像一个队列

buffer是从末尾写入头部读取

buffer 分为两部分input buffer 和 output buffer

####1.input buffer

muduo 会 从socket中读取数据，写入到input buffer 这是通过readFd（）实现的，我们看下readFd

```
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
  // saved an ioctl()/FIONREAD call to tell how much to read
  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = writableBytes();
  vec[0].iov_base = begin()+writerIndex_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;
  // when there is enough space in this buffer, don't read into extrabuf.
  // when extrabuf is used, we read 128k-1 bytes at most.
  const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
  const ssize_t n = sockets::readv(fd, vec, iovcnt);
  if (n < 0)
  {
    *savedErrno = errno;
  }
  else if (implicit_cast<size_t>(n) <= writable)
  {
    writerIndex_ += n;
  }
  else
  {
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  // if (n == writable + sizeof extrabuf)
  // {
  //   goto line_30;
  // }
  return n;
}

```

客户端会读入input buffer 

####2.output buffer

TcpConnection::send 去完成,我么看下源码

```
void TcpConnection::send(const StringPiece& message)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(message);
    }
    else
    {
      void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
      loop_->runInLoop(
          std::bind(fp,
                    this,     // FIXME
                    message.as_string()));
                    //std::forward<string>(message)));
    }
  }
}
```
如果在当前线程会直接调用sendLoop 去 把消息发出去，如果不在当前线程 会调用 runInLoop 把 消息转移到对应的线程

```
void EventLoop::runInLoop(Functor cb)
{
  if (isInLoopThread())
  {
    cb();
  }
  else
  {
    queueInLoop(std::move(cb));
  }
}

void EventLoop::queueInLoop(Functor cb)
{
  {
  MutexLockGuard lock(mutex_);
  pendingFunctors_.push_back(std::move(cb));
  }

  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();
  }
}

void EventLoop::wakeup()
{
  uint64_t one = 1;
  ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}
```
这里做的是一个唤醒操作 判断是否在本现场当中，如果不在本线程中就调用wakeup去唤醒对应的线程

####3.buffer的数据结构

```
--------------------------------------------------------------------------
|            |                              |                            |
|            |                              |                            |
-------------------------------------------------------------------------|
0           readIndex                      writeIndex                    size()
```

prependable = readIndex
readable = writeIndex - readIndex
writable = size() - writeIndex

####4. buffer 的操作

基本的read-write cycle

buffer 初始化后的情况，如果对着buffer 写入 200个字节

```
8          200(readable)           824(writeable)
-------------------------------------------|
|        |          |                      |
|--------|----------|----------------------|
0        readIndex(8)writeIndex(208)        size(1032)
```

如果从Buffer read 中读取了50个字节 那么 readIndex 会向后移动，writeIndex会保持不变，readable和writeable的值也有变化。

readIndex 会向后 移动50，变成了58

如果再继续写入200个字节 writeIndex 会变成408

可读数据变成了350，如果全部数据都被读出来了就变为了

```
        readable(0)
         8                       824(writeable)
------------------------------------------|
|        |                                |
|--------|--------------------------------|
0        readIndex(8)                  size(1032)
        writeIndex(8)
```


自动增长:

muduo buffer不是固定长度的，他可以自动增长，这是使用vector的好处。

如果说可写字节数只有624，这时候需要写入1000个字节,那么buffer会自动增长，从而容纳全部数据 vector会自动扩容原来的指针会失效

如果在频繁读写以后muduo 的readIndex 被移动到比较靠后的位置，这时候muduo会提前把数据放到前面去，再写入