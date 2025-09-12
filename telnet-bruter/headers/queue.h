
#pragma once

typedef struct 
{
    int queued;
    char address[16];
} Queue;

// Function prototypes
int bruter_queue_init(int capacity);
void bruter_queue_cleanup(void);
int bruter_queue_ip(char *address);
char* bruter_queue_get(void);
int bruter_queue_size(void);
int bruter_queue_is_empty(void);
void *handle_queued(void *arg);
void bruter_queue_stats(void);

// Global variables
extern volatile int left_in_queue;
