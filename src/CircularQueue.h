#include <vector>

template<class T>
class CircularQueue
{
    private:
        std::vector<T> circularQueue{};
        int max_size = 0;
        int counter = 0;
        int back_index = 0;
        int front_index = 0;
        void unroll_queue();
        std::vector<T> unrolled_queue{};

    public:

        // Initializes a vector with size equal to size
        CircularQueue(int max_size);

        CircularQueue(std::vector<T> list);
    
        std::vector<T> get_queue();

        int get_size();
        
        int get_max_size();

        void push(T value);

        T pop();

        T get_front();

        T get_back();

        auto begin();

        auto end();
};

template<class T>
CircularQueue<T>::CircularQueue(int max_size)
{
    this->max_size = max_size;
    this->circularQueue.reserve(max_size);
    this->back_index = 0;
    this->front_index = (this->counter - 1) % max_size;
}

template <class T>
CircularQueue<T>::CircularQueue(std::vector<T> list)
{
    this->circularQueue = list;
    this->max_size = list.size();
    this->circularQueue.reserve(this->max_size);
    this->counter = this->max_size;
    this->front_index = (this->counter - 1) % max_size;
    this->back_index = 0;
}

template<class T>
void CircularQueue<T>::push(T value)
{
    if (this->counter < this->max_size)
    {
        this->circularQueue.push_back(value);
    }
    else
    {
        int index = counter % this->max_size;
        auto position = this->circularQueue.begin() + index;
        this->circularQueue.erase(position);
        this->circularQueue.insert(position, value);

        this->back_index = (counter + 1) % max_size;
    }

    this->counter++;
    this->front_index = (this->counter - 1) % max_size;
}

template<class T>
T CircularQueue<T>::pop()
{
    T value = this->circularQueue[back_index];
    auto position = this->circularQueue.begin() + back_index;

    this->circularQueue.erase(position);
    
    if (this->circularQueue.size() == 1)
    {
        this->front_index = 0;
    }
    if (this->back_index > this->circularQueue.size() - 1)
    {
        this->back_index = 0;
    }

    return value;
}

template<class T>
std::vector<T> CircularQueue<T>::get_queue()
{
    return this->circularQueue;
}

template<class T>
int CircularQueue<T>::get_size()
{
    return this->circularQueue.size();
}

template<class T>
int CircularQueue<T>::get_max_size()
{
    return this->max_size;
}

template<class T>
T CircularQueue<T>::get_front()
{
    return this->circularQueue[this->front_index];
}

template<class T>
T CircularQueue<T>::get_back()
{
    return this->circularQueue[back_index];
}

template<class T>
void CircularQueue<T>::unroll_queue()
{
    unrolled_queue = {};
    int i = this->front_index;
    while (i != back_index)
    {
        this->unrolled_queue.push_back(this->circularQueue[i]);
        --i;

        if (i < 0)
        {
            i = this->circularQueue.size() - 1;
        }
    }
    this->unrolled_queue.push_back(this->circularQueue[i]);
}

template<class T>
auto CircularQueue<T>::begin()
{
    CircularQueue<T>::unroll_queue();
    return this->unrolled_queue.begin();
}

template<class T>
auto CircularQueue<T>::end()
{
    CircularQueue<T>::unroll_queue();
    return this->unrolled_queue.end();
}