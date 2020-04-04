#include <memory>
#include <iostream>
#include <string.h>
#include <vector>
#include <pthread.h>
#include <map>
#include <functional>
using namespace std;
using namespace std::placeholders;
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

class Stock{
public:
    Stock(const string& key)
    {
        keyData = key;
    }

    const string& key()
    {
        return keyData;
    }

private:
    string keyData;
};

class StockFactory
{
public:
    shared_ptr<Stock> get(const string& key)
    {
        shared_ptr<Stock> pStock;
        MutexLockGuard guard(mutex_);
        weak_ptr<Stock> wkStock = stocks[key];
        pStock = wkStock.lock();
        if(!pStock){
            pStock.reset(new Stock(key),std::bind(&StockFactory::deleteStock,this,_1));
            wkStock = pStock;
        }
        return pStock;
    }

private:
    mutable MutexLock mutex_;
    map<string,shared_ptr<Stock>> stocks;

    void deleteStock(Stock* stock)
    {
        if (stock) {
            MutexLockGuard lock(mutex_);
            stocks.erase(stock->key());
        }
        delete stock; // sorry, I lied
    }
};
StockFactory factory;

void  test()
{
    shared_ptr<Stock> a = factory.get("ahahah");
}

int main()
{
    test();
    return  0;

}