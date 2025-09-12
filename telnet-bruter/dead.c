
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "headers/main.h"
#include "headers/connection.h"

void *watch_time()
{
    int fake_time;

    while(1)
    {
        fake_time = time(0);

        // Safe iteration with bounds checking
        int max_check = (maxfds > MAX_SOCKETS) ? MAX_SOCKETS : maxfds + 1;
        
        for(int i = 0; i < max_check; i++)
        {
            // Check if bruter slot is actually in use
            if(bruter[i].fd > 0 && bruter[i].last_recv > 0)
            {
                if((fake_time - bruter[i].last_recv) >= TIMEOUT)
                {
#ifdef DEBUG
                    printf("Timeout detected for fd %d (%s)\n", bruter[i].fd, bruter[i].address);
#endif
                    disconnect(&bruter[i]);
                    ATOMIC_INC(&failed);
                }
            }
        }

        sleep(1);
    }
    
    return NULL;
}
