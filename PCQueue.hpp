#ifndef _QUEUEL_H
#define _QUEUEL_H

#include "Headers.hpp"

// Single Producer - Multiple Consumer queue
template<typename T>
class PCQueue {

public:
    PCQueue() {
        PCqueue = new queue<T>;
        readers_inside = 0;
        writers_inside = 0;
        writers_waiting = 0;
        popping = 0;
        pthread_cond_init(&read_allowed, NULL);
        pthread_cond_init(&queue_size, NULL);
        pthread_cond_init(&write_allowed, NULL);
        pthread_mutex_init(&global_lock,
                           NULL); //TODO: check if PTHREAD_MUTEX_ERRORCHECK is what we need.
    }

    ~PCQueue() {
        pthread_cond_destroy(&read_allowed);
        pthread_cond_destroy(&queue_size);
        pthread_cond_destroy(&write_allowed);
        pthread_mutex_destroy(&global_lock);
        delete PCqueue;
        PCqueue = NULL;
    }

    // Blocks while queue is empty. When queue holds items, allows for a single
    // thread to enter and remove an item from the front of the queue and return it.
    // Assumes multiple consumers.
    T pop() {
        T returnValue;
        pthread_mutex_lock(&global_lock);
        popping++;
        // printf("pcqueue size is %d\n", PCqueue.size());
        while (PCqueue->size() == 0) pthread_cond_wait(&queue_size, &global_lock);
        while (writers_inside > 0 || writers_waiting > 0)pthread_cond_wait(&read_allowed, &global_lock);
        popping--;
        readers_inside++;
        returnValue = PCqueue->front();
        PCqueue->pop();
        readers_inside--;
        if (readers_inside == 0) {
            pthread_cond_signal(&write_allowed);
        }
        pthread_mutex_unlock(&global_lock);
        return returnValue;
    }

    // Allows for producer to enter with *minimal delay* and push items to back of the queue.
    // Hint for *minimal delay* - Allow the consumers to delay the producer as little as possible.
    // Assumes single producer
    void push(const T &item) {
        pthread_mutex_lock(&global_lock);
        writers_waiting++;
        while (writers_inside + readers_inside > 0)
            pthread_cond_wait(&write_allowed, &global_lock);
        writers_waiting--;
        writers_inside++;
        PCqueue->push(item);

        writers_inside--;

        pthread_cond_signal(&queue_size);
        pthread_cond_broadcast(&read_allowed);
        pthread_cond_signal(&write_allowed);

        pthread_mutex_unlock(&global_lock);
    }

    int getWritersWaiting() {

        return popping;
    }

private:
    queue<T> *PCqueue;
    int readers_inside, writers_inside, writers_waiting, popping;
    pthread_cond_t read_allowed;
    pthread_cond_t write_allowed;
    pthread_cond_t queue_size;
    pthread_mutex_t global_lock;
};
// Recommendation: Use the implementation of the std::queue for this exercise
#endif