#include "Observer.h"
#include "Observerable.h"
#include <stdio.h>
class Foo : public Observer
{
    virtual void update()
    {
        printf("%ld:Foo::update() %p\n",pthread_self(), this);
    }
};
int main()
{
    Foo* p = new Foo;
    Foo* p2 = new Foo;
    Foo* p3 = new Foo;
    Foo* p4 = new Foo;
    Observrable subject;
    p->observe(&subject);
    p2->observe(&subject);
    p3->observe(&subject);
    p4->observe(&subject);
    subject.notifyObservers();
    delete p;
    delete p2;
    delete p3;
    delete p4;
    subject.notifyObservers();
}