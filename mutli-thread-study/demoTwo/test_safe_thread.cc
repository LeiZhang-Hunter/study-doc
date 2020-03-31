#include <memory>
#include <iostream>
#include <string.h>
#include <vector>
#include <pthread.h>
using namespace std;

class MutexLock
{
public:
    MutexLock()
    {
        pthread_mutex_init(&_mutex, NULL);
    }

    ~MutexLock()
    {
        pthread_mutex_destroy(&_mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&_mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&_mutex);
    }

    pthread_mutex_t * getMutexLockPtr()
    {
        return &_mutex;
    }
private:
    pthread_mutex_t _mutex;
};

//RAII
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
        _mutex.unlock();
    }

private:
    MutexLock & _mutex;
};

//end of namespace wd

class Observable;

class Observer{
public:
    virtual void update() = 0;
};

class Foo : public Observer
{
public:
    virtual void update()
    {
        printf("%ld:Foo::update() %p\n",pthread_self(), this);
    }
};



class Observable{
public:
    void register_(weak_ptr<Observer> x)
    {
        observers.push_back(x);
    }

    void notifyObservers()
    {
        MutexLockGuard guard(mutex_);
        Iterator it = observers.begin();
        while(it != observers.end())
        {
            shared_ptr<Observer> obj(it->lock());
            if(obj)
            {
                obj->update();
            }else{
                it = observers.erase(it);
            }
        }
    }

private:
    mutable MutexLock mutex_;
    vector<weak_ptr<Observer>> observers;
    typedef std::vector<weak_ptr<Observer>>::iterator Iterator;
};

void* threadOne(void* arg)
{
    int count = 0;
    while (1) {
        Foo *p = new Foo;

        subject.notifyObservers();
        delete p;
        subject.notifyObservers();
        count++;
        if(count== sum)
        {
            break;
        }
    }
}

void* threadTwo(void* arg)
{
    int count = 0;
    while (1) {
        Foo *p = new Foo;
        p->observe(&subject);
        subject.notifyObservers();
        delete p;
        subject.notifyObservers();
        count++;
        if(count== sum)
        {
            break;
        }
    }

}

void* threadThree(void* arg)
{
    int count = 0;
    while (1) {
        Foo *p = new Foo;
        p->observe(&subject);
        subject.notifyObservers();
        delete p;
        subject.notifyObservers();
        count++;
        if(count== sum)
        {
            break;
        }
    }
}

int main()
{
    pthread_t thread1;
    pthread_t thread2;
    pthread_t thread3;
    pthread_create(&thread1,NULL,threadOne,NULL);
    pthread_create(&thread2,NULL,threadTwo,NULL);
    pthread_create(&thread3,NULL,threadThree,NULL);

    pthread_join(thread1,NULL);
    pthread_join(thread2,NULL);
    pthread_join(thread3,NULL);
}