//
// Created by zhanglei on 20-3-22.
//

#include "Observer.h"
#include "Observerable.h"
#include <pthread.h>
#include <stdio.h>

Observrable subject;
int sum = 1000000;

class Foo : public Observer
{
    virtual void update()
    {
        printf("%ld:Foo::update() %p\n",pthread_self(), this);
    }
};

void* threadOne(void* arg)
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