#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    thread_func_args->thread_complete_success = false;

    // Wait before attempting to obtain the mutex
    usleep(thread_func_args->wait_to_obtain_ms * 1000);

    // Obtain the mutex
    int rc = pthread_mutex_lock(thread_func_args->mutex);
    if (rc != 0) {
        ERROR_LOG("pthread_mutex_lock failed with code %d", rc);
        return thread_param;
    }

    // Wait while holding the mutex
    usleep(thread_func_args->wait_to_release_ms * 1000);

    // Release the mutex
    rc = pthread_mutex_unlock(thread_func_args->mutex);
    if (rc != 0) {
        ERROR_LOG("pthread_mutex_unlock failed with code %d", rc);
        return thread_param;
    }

    // If we made it here, everything went perfectly
    thread_func_args->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     */
    
    // Allocate memory on the heap for thread_data so it persists after this function returns
    struct thread_data* data = (struct thread_data*) malloc(sizeof(struct thread_data));
    if (data == NULL) {
        ERROR_LOG("Failed to allocate memory for thread_data");
        return false;
    }

    // Setup the struct fields
    data->mutex = mutex;
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    data->thread_complete_success = false;

    // Create the thread, passing the data struct as the argument
    int rc = pthread_create(thread, NULL, threadfunc, (void*)data);
    if (rc != 0) {
        ERROR_LOG("pthread_create failed with code %d", rc);
        free(data); // Clean up allocated memory if thread creation fails
        return false;
    }

    return true;
}
