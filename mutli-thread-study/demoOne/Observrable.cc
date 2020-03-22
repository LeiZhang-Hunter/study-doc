//
// Created by zhanglei on 20-3-21.
//

#include "Observerable.h"
#include <algorithm>

void Observrable::register_(Observer *x) {
    observers_.push_back(x);
}

void Observrable::unregister_(Observer *x) {
    std::vector<Observer*>::iterator it;
    it = std::find(observers_.begin(), observers_.end(), x);
    if (it != observers_.end())
    {
        std::swap(*it, observers_.back());
        observers_.pop_back();
    }
}