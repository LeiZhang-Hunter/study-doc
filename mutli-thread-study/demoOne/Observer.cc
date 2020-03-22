//
// Created by zhanglei on 20-3-21.
//
#include "Observerable.h"
Observer::~Observer() {
    subject->unregister_(this);
}

void Observer::observe(Observrable *s) {
    s->register_(this);
    subject = s;
}

