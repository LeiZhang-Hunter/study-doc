#include <memory>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <pthread.h>
using namespace std;
class Observable;
int sum = 50;

class MutexLock{
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

    void lock()
    {
        int res = pthread_mutex_lock(&mutex);
        std::cout<<res<<std::endl;
    }

    void unLock()
    {
        pthread_mutexattr_destroy(&mutexattr);
        pthread_mutex_unlock(&mutex);
    }
private:
    pthread_mutex_t mutex;
    pthread_mutexattr_t mutexattr;
};

MutexLock mutex(PTHREAD_MUTEX_RECURSIVE);


void foo()
{
    mutex.lock();
    // do something
    mutex.unLock();
}

void* func(void* arg)
{
    mutex.lock();
    printf("3333\n");
}

int main()
{
    pthread_t tid;
    pthread_create(&tid, nullptr,func, nullptr);

    int res;
    mutex.lock();
    sleep(5);
    mutex.unLock();
    sleep(3);
}