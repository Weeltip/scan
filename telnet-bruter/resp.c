#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "headers/main.h"
#include "headers/combos.h"

int check_login_resp(Brute *brute)
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

    // Enhanced login prompt detection
    const char *login_patterns[] = {
        "username",         // Username prompt
        "user name",        // User name prompt  
        "login",           // Login prompt
        "user",            // User prompt
        "enter",           // Enter prompt
        "password",        // Password prompt
        "passwd",          // Passwd prompt
        "pass",            // Pass prompt
        "pin",             // PIN prompt
        "code",            // Code prompt
        "authenticate",    // Authentication prompt
        "credentials",     // Credentials prompt
        "account",         // Account prompt
        "id:",             // ID prompt
        "name:",           // Name prompt
        ">>",              // Some prompts
        ":",               // Generic colon prompt
        NULL
    };

    // Search for login indicators
    for(int i = 0; login_patterns[i] != NULL; i++) {
        if(strstr(lower_buf, login_patterns[i]) != NULL) {
#ifdef DEBUG
            printf("Login prompt detected: pattern '%s' found in response from %s\n", 
                   login_patterns[i], brute->address);
#endif
            return 1;
        }
    }
    
    return 0;
}

int check_password_resp(Brute *brute)
{
    if(brute == NULL || brute->rdbuf == NULL)
        return 0;

    // Ensure buffer is null-terminated for safe string operations
    int buf_len = sizeof(brute->rdbuf) - 1;
    brute->rdbuf[buf_len] = '\0';
    
    int len = strnlen(brute->rdbuf, buf_len);
    if(len == 0)
        return 0;

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

    // Enhanced failure detection patterns
    const char *failure_patterns[] = {
        "incorrect",        // Incorrect login
        "invalid",          // Invalid credentials
        "failed",          // Failed login
        "denied",          // Access denied
        "refused",         // Connection refused
        "reject",          // Rejected
        "wrong",           // Wrong password
        "bad",             // Bad login
        "error",           // Error message
        "unauthorized",    // Unauthorized access
        "forbidden",       // Forbidden access
        "timeout",         // Timeout
        "closed",          // Connection closed
        "disconnected",    // Disconnected
        "try again",       // Try again
        "authentication failed", // Auth failed
        NULL
    };

    // Check for failure indicators first
    for(int i = 0; failure_patterns[i] != NULL; i++) {
        if(strstr(lower_buf, failure_patterns[i]) != NULL) {
#ifdef DEBUG
            printf("Login failure detected: pattern '%s' found in response from %s\n", 
                   failure_patterns[i], brute->address);
#endif
            return 0;
        }
    }

    // Check for success indicators (shell prompts)
    const char success_chars[] = {':', '>', '$', '#', '%', '~', ']', ')'};
    int success_chars_count = sizeof(success_chars) / sizeof(success_chars[0]);

    // Look for shell prompt indicators at the end of response
    for(int i = len - 1; i >= 0 && i >= len - 10; i--) {  // Check last 10 chars
        for(int j = 0; j < success_chars_count; j++) {
            if(brute->rdbuf[i] == success_chars[j]) {
#ifdef DEBUG
                printf("Success prompt detected: char '%c' found in response from %s\n", 
                       success_chars[j], brute->address);
#endif
                return 1;
            }
        }
    }

    // Additional success patterns
    const char *success_patterns[] = {
        "welcome",         // Welcome message
        "logged in",       // Logged in message
        "shell",           // Shell access
        "terminal",        // Terminal access
        "command",         // Command prompt
        "prompt",          // Prompt indicator
        "successful",      // Successful login
        "authenticated",   // Authenticated
        "connected",       // Connected
        "access granted",  // Access granted
        NULL
    };

    // Search for success indicators
    for(int i = 0; success_patterns[i] != NULL; i++) {
        if(strstr(lower_buf, success_patterns[i]) != NULL) {
#ifdef DEBUG
            printf("Success message detected: pattern '%s' found in response from %s\n", 
                   success_patterns[i], brute->address);
#endif
            return 1;
        }
    }

    return 0;
}