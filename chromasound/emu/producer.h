#ifndef PRODUCER_H
#define PRODUCER_H

template <typename T>
class Producer {
    public:
        Producer();

        bool active() const;
        void activate();
        void deactivate();

        virtual T* next(int size) = 0;

    private:
        bool _active;

};

template<typename T>
Producer<T>::Producer()
    : _active(true)
{

}

template<typename T>
bool Producer<T>::active() const
{
    return _active;
}

template<typename T>
void Producer<T>::activate()
{
    _active = true;
}

template<typename T>
void Producer<T>::deactivate()
{
    _active = false;
}

#endif // PRODUCER_H
