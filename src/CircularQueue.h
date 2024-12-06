#include <vector>

// This class implements a circular buffer. Refer to this Wikipedia page for more information:
// https://en.wikipedia.org/wiki/Circular_buffer
// This implementation was heavily influenced by the ideas laid out in the Circular buffer
// Wikipedia page.
template<class T>
class CircularQueue
{
    public:
        // Initializes a CircularQueue with max_size. Reserves a vector with a size of max_size.
        CircularQueue(int max_size);

        // Initializes a CircularQueue with the contents of list. The front of the CircularQueue
        // will be the last value of list, and the back will be the first value of list.
        CircularQueue(std::vector<T> list);
    
        // Returns the vector representing the circular buffer. The "front" of the queue might not
        // necessarily be the last value of the vector, and the "back" of the queue might not
        // necessarily be the first value.
        std::vector<T> get_queue();

        // Return the total number of elements in the circular queue
        int get_size();
        
        // Return the maximum number of elements the circular queue can store
        int get_max_size();

        // Add a value to the front of the circular queue. If adding another value would cause the
        // queue to exceed the maximum size, then the value overwrites the back of the queue. The
        // front of the queue now points to the newly added value, and the back of the queue moves
        // to the next value in the sequence.
        void push(T value);

        // Remove and return the value from the back of the circular buffer. In other words, this
        // circular buffer maintains a first in, first out policy.
        T pop();

        // Return the value at the front of the queue
        T get_front();

        // Return the value at the back of the queue
        T get_back();

        // Return the last value of the vector holding the circular queue. This might not
        // necessarily be the back of the circular queue
        T get_last();

        // Remove and return the last value of the vector holding the circular queue (not
        // necessarily the value of the back of the circular queue)
        T pop_last();

        // Return an iterator pointing to the front of the queue
        auto begin();

        // Return an iterator pointing to one element past the back of the queue
        auto end();

        auto erase(typename std::vector<T>::iterator iter);

    private:
        // The vector containing all the values in the circular queue
        std::vector<T> circularQueue{};

        // The maximum size of the circular queue
        int max_size = 0;

        int counter = 0;
        
        int back_index = 0;
        int front_index = 0;

        std::vector<T> unrolled_queue{};
};

template<class T>
CircularQueue<T>::CircularQueue(int max_size)
{
    this->max_size = max_size;
    circularQueue.reserve(max_size);
    back_index = 0;
    front_index = (counter - 1) % max_size;
}

template <class T>
CircularQueue<T>::CircularQueue(std::vector<T> list)
{
    circularQueue = list;
    max_size = list.size();
    circularQueue.reserve(max_size);
    counter = max_size;
    front_index = (counter - 1) % max_size;
    back_index = 0;
}

template<class T>
void CircularQueue<T>::push(T value)
{
    if (counter < max_size)
    {
        circularQueue.push_back(value);
    }
    else
    {
        int index = counter % max_size;
        auto position = circularQueue.begin() + index;
        circularQueue.erase(position);
        circularQueue.insert(position, value);

        back_index = (counter + 1) % max_size;
    }

    counter++;
    front_index = (counter - 1) % max_size;
}

template<class T>
T CircularQueue<T>::pop()
{
    T value = circularQueue[back_index];
    auto position = circularQueue.begin() + back_index;

    circularQueue.erase(position);
    
    if (circularQueue.size() == 1)
    {
        front_index = 0;
    }
    if (back_index > circularQueue.size() - 1)
    {
        back_index = 0;
    }

    return value;
}

template<class T>
std::vector<T> CircularQueue<T>::get_queue()
{
    return circularQueue;
}

template<class T>
int CircularQueue<T>::get_size()
{
    return circularQueue.size();
}

template<class T>
int CircularQueue<T>::get_max_size()
{
    return max_size;
}

template<class T>
T CircularQueue<T>::get_front()
{
    return circularQueue[front_index];
}

template<class T>
T CircularQueue<T>::get_back()
{
    return circularQueue[back_index];
}

template<class T>
T CircularQueue<T>::get_last()
{
    return circularQueue.back();
}

template<class T>
T CircularQueue<T>::pop_last()
{
    counter--;
    if (front_index == max_size - 1)
    {
        front_index = (counter - 1) % max_size;
    }
    auto last = circularQueue.back();
    circularQueue.pop_back();
    return last;
}

template<class T>
auto CircularQueue<T>::begin()
{
    return circularQueue.begin();
}

template<class T>
auto CircularQueue<T>::end()
{
    return circularQueue.end();
}


template<class T>
auto CircularQueue<T>::erase(typename std::vector<T>::iterator iter)
{
    counter--;
    front_index = (counter - 1) % max_size;
    return circularQueue.erase(iter);
}