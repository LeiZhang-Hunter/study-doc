//
// Created by zhanglei on 20-3-21.
//

#ifndef TEST_CPP_OBSERVERABLE_H
#define TEST_CPP_OBSERVERABLE_H

#include "Observer.h"
#include <vector>
class Observrable{
public:
    void register_(Observer* x);
    void unregister_(Observer* x);

    void notifyObservers(){
        for (size_t i = 0; i < observers_.size(); ++i)
        {
            Observer* x = observers_[i];
            if (x) {
                x->update(); // (3)
            }
        }
    }

private:
    std::vector<Observer*> observers_;
};
#endif //TEST_CPP_OBSERVERABLE_H
