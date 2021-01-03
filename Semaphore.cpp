#include "Semaphore.hpp"


Semaphore::Semaphore(unsigned val) : counter(val), waiting_threads(0) {
    pthread_mutex_init(&global_mutex, NULL); // TODO: CHECK NULL or ErrorSomehting
    pthread_cond_init(&condition, NULL);
}

Semaphore::Semaphore() : counter(0), waiting_threads(0) {
    pthread_mutex_init(&global_mutex, NULL);
    pthread_cond_init(&condition, NULL);
}

/* sem_post() increments (unlocks) the semaphore pointed to by sem.  If
     the semaphore's value consequently becomes greater than zero, then
     another process or thread blocked in a sem_wait(3) call will be woken
     up and proceed to lock the semaphore.*/

void Semaphore::up() {
    pthread_mutex_lock(&global_mutex);
    counter++;
    if (counter > 0 && waiting_threads > 0) {
        pthread_cond_signal(&condition);
    }
    pthread_mutex_unlock(&global_mutex);
}

/* decrements (locks) the semaphore pointed to by sem.  If
     the semaphore's value is greater than zero, then the decrement
     proceeds, and the function returns, immediately.  If the semaphore
     currently has the value zero, then the call blocks until either it
     becomes possible to perform the decrement (i.e., the semaphore value
     rises above zero), or a signal handler interrupts the call*/

void Semaphore::down() {
    pthread_mutex_lock(&global_mutex);
    if (counter == 0) {
        waiting_threads++;
        do {
            pthread_cond_wait(&condition, &global_mutex);

        } while (counter == 0);
        waiting_threads--;
    }
    counter--;
    pthread_mutex_unlock(&global_mutex);
}

Semaphore::~Semaphore() {
    pthread_mutex_destroy(&global_mutex);
    pthread_cond_destroy(&condition);

}