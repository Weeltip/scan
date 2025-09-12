#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#include "headers/queue.h"
#include "headers/main.h"
#include "headers/connection.h"

// Circular queue implementation
static int queue_head = 0;
static int queue_tail = 0;
static int queue_size = 0;
static int queue_capacity = 10000;  // More reasonable size
static Queue *queue = NULL;
static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;
static pthread_cond_t queue_not_full = PTHREAD_COND_INITIALIZER;

int bruter_queue_init(int capacity)
{
    if (capacity > 0) {
        queue_capacity = capacity;
    }
    
    queue = calloc(queue_capacity, sizeof(Queue));
    if (!queue) {
        fprintf(stderr, "Failed to allocate queue memory\n");
        return -1;
    }
    
    return 0;
}

void bruter_queue_cleanup(void)
{
    pthread_mutex_lock(&queue_mutex);
    
    if (queue) {
        free(queue);
        queue = NULL;
    }
    
    queue_head = 0;
    queue_tail = 0;
    queue_size = 0;
    
    pthread_mutex_unlock(&queue_mutex);
}

int bruter_queue_ip(char *address)
{
    if (!address || !queue) {
        return -1;
    }
    
    pthread_mutex_lock(&queue_mutex);
    
    // Wait if queue is full
    while (queue_size >= queue_capacity) {
        pthread_cond_wait(&queue_not_full, &queue_mutex);
    }
    
    // Add to circular queue
    strncpy(queue[queue_tail].address, address, sizeof(queue[queue_tail].address) - 1);
    queue[queue_tail].address[sizeof(queue[queue_tail].address) - 1] = '\0';
    queue[queue_tail].queued = 1;
    
    queue_tail = (queue_tail + 1) % queue_capacity;
    queue_size++;
    
    ATOMIC_INC(&left_in_queue);
    
    // Signal that queue is not empty
    pthread_cond_signal(&queue_not_empty);
    
    pthread_mutex_unlock(&queue_mutex);
    return 0;
}

char* bruter_queue_get(void)
{
    static char address[16];
    
    pthread_mutex_lock(&queue_mutex);
    
    // Wait if queue is empty
    while (queue_size == 0) {
        pthread_cond_wait(&queue_not_empty, &queue_mutex);
    }
    
    // Get from circular queue
    strncpy(address, queue[queue_head].address, sizeof(address) - 1);
    address[sizeof(address) - 1] = '\0';
    
    memset(&queue[queue_head], 0, sizeof(Queue));
    queue_head = (queue_head + 1) % queue_capacity;
    queue_size--;
    
    ATOMIC_DEC(&left_in_queue);
    
    // Signal that queue is not full
    pthread_cond_signal(&queue_not_full);
    
    pthread_mutex_unlock(&queue_mutex);
    
    return address;
}

int bruter_queue_size(void)
{
    pthread_mutex_lock(&queue_mutex);
    int size = queue_size;
    pthread_mutex_unlock(&queue_mutex);
    return size;
}

int bruter_queue_is_empty(void)
{
    return bruter_queue_size() == 0;
}

void *handle_queued(void *arg)
{
    char *address;
    
    // Initialize queue if not already done
    if (!queue && bruter_queue_init(0) != 0) {
        fprintf(stderr, "Failed to initialize queue\n");
        return NULL;
    }
    
    while(1)
    {
        // Only process if we have capacity and items in queue
        if(processing < ACTUAL_MAX_CONS && !bruter_queue_is_empty())
        {
            address = bruter_queue_get();
            if (address && strlen(address) > 0) {
                start_connection(address, NULL);
            }
        }
        else
        {
            // Sleep briefly to avoid busy waiting
            usleep(100000); // 100ms
        }
    }
    
    return NULL;
}

// Statistics function
void bruter_queue_stats(void)
{
    pthread_mutex_lock(&queue_mutex);
    printf("Queue Stats: Size=%d, Capacity=%d, Head=%d, Tail=%d\n", 
           queue_size, queue_capacity, queue_head, queue_tail);
    pthread_mutex_unlock(&queue_mutex);
}
