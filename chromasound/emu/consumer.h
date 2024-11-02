#ifndef CONSUMER_H
#define CONSUMER_H

#include "producer.h"

template <typename T>
class Consumer {
    public:
        Consumer();
        void producer(Producer<T>* const producer);

    protected:
        T* consumeA(const int size) const;
        T* consumeB(const int size) const;

        Producer<T>* producerA();
        Producer<T>* producerB();

    private:
        Producer<T>* _producerA;
        Producer<T>* _producerB;
};

template <typename T>
Consumer<T>::Consumer()
    : _producerA(nullptr)
    , _producerB(nullptr)  { }

template <typename T>
void Consumer<T>::producer(Producer<T>* const producer) {
    if (_producerA) {
        _producerB = producer;
    } else {
        _producerA = producer;
    }
}

template <typename T>
T* Consumer<T>::consumeA(const int size) const {
    if (_producerA == nullptr)
        return nullptr;

    return _producerA->next(size);
}

template <typename T>
T* Consumer<T>::consumeB(const int size) const {
    if (_producerB == nullptr)
        return nullptr;

    return _producerB->next(size);
}

template<typename T>
Producer<T>* Consumer<T>::producerA()
{
    return _producerA;
}

template<typename T>
Producer<T>* Consumer<T>::producerB()
{
    return _producerB;
}


#endif // CONSUMER_H
