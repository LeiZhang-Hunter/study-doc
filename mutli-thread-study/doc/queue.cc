#include <memory>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <pthread.h>
#include <functional>
#include <deque>

using namespace std;
using namespace std::placeholders;
class Observable;
int sum = 50;

class noncopyable{
protected:
    noncopyable() = default;
    ~noncopyable() = default;

private:
    noncopyable(const noncopyable&) = delete;
    const noncopyable& operator=( const noncopyable& ) = delete;
};


class MutexLock :public noncopyable{
public:
    MutexLock()
    {
        pthread_mutexattr_init(&mutexattr);
        pthread_mutex_init(&mutex, nullptr);
    }

    MutexLock(int type)
    {
        int res;
        pthread_mutexattr_init(&mutexattr);
        res = pthread_mutexattr_settype(&mutexattr,type);
        pthread_mutex_init(&mutex, &mutexattr);
    }

    ~MutexLock()
    {
        pthread_mutex_destroy(&mutex);
    }

    int lock()
    {
        int res = pthread_mutex_lock(&mutex);
        return res;
    }

    void unLock()
    {
        pthread_mutexattr_destroy(&mutexattr);
        pthread_mutex_unlock(&mutex);
    }

    pthread_mutex_t* getMutex()
    {
        return &mutex;
    }
private:
    pthread_mutex_t mutex;
    pthread_mutexattr_t mutexattr;
};

class MutexLockGuard
{
public:
    MutexLockGuard(MutexLock & mutex)
            : _mutex(mutex)
    {
        _mutex.lock();
    }

    ~MutexLockGuard()
    {
        _mutex.unLock();
    }

private:
    MutexLock & _mutex;
};

MutexLock mutex(PTHREAD_MUTEX_ERRORCHECK_NP);


class Condition:noncopyable
{
public:
    //explicit用于修饰只有一个参数的构造函数，表明结构体是显示是的，不是隐式的，与他相对的另一个是implicit，意思是隐式的
    //explicit关键字只需用于类内的单参数构造函数前面。由于无参数的构造函数和多参数的构造函数总是显示调用，这种情况在构造函数前加explicit无意义。
    Condition(MutexLock& mutex) : mutex_(mutex)
    {
        int res = pthread_cond_init(&pcond_, nullptr);
    }

    ~Condition()
    {
        pthread_cond_destroy(&pcond_);
    }

    void wait()
    {
        int res = pthread_cond_wait(&pcond_,mutex_.getMutex());
    }

    void notify()
    {
        int res = (pthread_cond_signal(&pcond_));
    }

    void notifyAll()
    {
        (pthread_cond_broadcast(&pcond_));
    }

private:
    MutexLock& mutex_;
    pthread_cond_t pcond_;
};

deque<int> queue;
Condition cond(mutex);

int dequeue()
{
    mutex.lock();
    while(queue.empty())
    {
        cond.wait();
    }
    int top = queue.front();
    queue.pop_front();
    mutex.unLock();
    return top;
}

void enqueue(int x)
{
    mutex.lock();
    queue.push_back(x);
    cond.notify();
}


class CThread{
public:
    //线程进程
    static void* threadProc(void* args)
    {
        while (1)
        {
            int a = dequeue();
            printf("(%ld)%d;\n",pthread_self(),a);
        }
    }
};



int main()
{
    int count = 5;
    int i;
    pthread_t tid;
    shared_ptr<CThread> threadObj = make_shared<CThread>();
    for(i=0;i<count;i++)
    {
        pthread_create(&tid, nullptr,&CThread::threadProc, nullptr);
    }

    while (1)
    {

        sleep(2);
        enqueue(3);
    }
}