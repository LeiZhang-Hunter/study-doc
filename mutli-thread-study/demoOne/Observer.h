//
// Created by zhanglei on 20-3-21.
//

#ifndef TEST_OBSERVER_H
#define TEST_OBSERVER_H
class Observrable;
class Observer{
public:
    virtual ~Observer();
    virtual void update() = 0;
    void observe(Observrable* s);

protected:
    Observrable* subject;
};



#endif //TEST_OBSERVER_H
