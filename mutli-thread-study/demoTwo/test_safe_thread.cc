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
    Foo()
    {
        printf("%ld:Foo::create() %p\n",pthread_self(), this);
    }
    virtual void update()
    {
        printf("%ld:Foo::update() %p\n",pthread_self(), this);
    }

    ~Foo()
    {
        printf("%ld:Foo::destroy() %p\n",pthread_self(), this);
    }
};


class Observable{
public:
    void register_(weak_ptr<Observer> x)
    {
        MutexLockGuard guard(mutex_);
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
                it++;
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

Observable subject;
int sum = 10;

void* threadOne(void* arg)
{
    shared_ptr<Foo> foo = make_shared<Foo>();
    weak_ptr<Foo> w(foo);
    subject.register_(w);
    subject.notifyObservers();


}

void* threadTwo(void* arg)
{
    shared_ptr<Foo> foo = make_shared<Foo>();
    subject.register_(foo);
    subject.notifyObservers();


}

void* threadThree(void* arg)
{
    shared_ptr<Foo> foo = make_shared<Foo>();
    subject.register_(foo);
    subject.notifyObservers();
}

int main()
{

    pthread_t thread1;
    pthread_t thread2;
    pthread_t thread3;
    pthread_create(&thread1, nullptr,threadOne,nullptr);
    pthread_create(&thread2,nullptr,threadTwo,nullptr);
    pthread_create(&thread3,nullptr,threadThree,nullptr);

    pthread_join(thread1,nullptr);
    pthread_join(thread2,nullptr);
    pthread_join(thread3,nullptr);
}