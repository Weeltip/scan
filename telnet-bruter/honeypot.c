
#include <stdio.h>
#include <string.h>

#include "headers/main.h"
#include "headers/honeypot.h"

int check_honeypot(Brute *brute)
{
    if(brute == NULL || brute->rdbuf == NULL)
        return 0;

    // Ensure buffer is null-terminated for safe string operations
    int buf_len = sizeof(brute->rdbuf) - 1;
    brute->rdbuf[buf_len] = '\0';

    // Convert to lowercase for case-insensitive search
    char lower_buf[sizeof(brute->rdbuf)];
    strncpy(lower_buf, brute->rdbuf, sizeof(lower_buf) - 1);
    lower_buf[sizeof(lower_buf) - 1] = '\0';
    
    // Convert to lowercase
    for(int i = 0; lower_buf[i] && i < sizeof(lower_buf) - 1; i++) {
        if(lower_buf[i] >= 'A' && lower_buf[i] <= 'Z') {
            lower_buf[i] += 32;
        }
    }

    // Enhanced honeypot detection patterns
    const char *honeypot_patterns[] = {
        "richard",           // Original pattern
    };

    // Search for honeypot indicators
    for(int i = 0; honeypot_patterns[i] != NULL; i++) {
        if(strstr(lower_buf, honeypot_patterns[i]) != NULL) {
#ifdef DEBUG
            printf("Honeypot detected: pattern '%s' found in response from %s\n", 
                   honeypot_patterns[i], brute->address);
#endif
            return 1;
        }
    }

    return 0;
}
