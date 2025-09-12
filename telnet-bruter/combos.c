
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headers/combos.h"

int cindex = 0;
Combo *combos = NULL;

void combo_add(char *username, char *password)
{
    if (combos == NULL) {
        combos = calloc(1, sizeof(Combo));
        if (combos == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
    } else {
        Combo *temp = realloc(combos, (cindex + 1) * sizeof(Combo));
        if (temp == NULL) {
            fprintf(stderr, "Memory reallocation failed\n");
            exit(1);
        }
        combos = temp;
    }

    // Duplicate strings to avoid issues with string literals
    combos[cindex].username = malloc(strlen(username) + 1);
    combos[cindex].password = malloc(strlen(password) + 1);

    if (combos[cindex].username == NULL || combos[cindex].password == NULL) {
        fprintf(stderr, "String duplication failed\n");
        exit(1);
    }

    strcpy(combos[cindex].username, username);
    strcpy(combos[cindex].password, password);

    combos[cindex].username_len = strlen(username);
    combos[cindex].password_len = strlen(password);
    cindex++;
}

void combos_cleanup(void)
{
    if (combos != NULL) {
        for (int i = 0; i < cindex; i++) {
            free(combos[i].username);
            free(combos[i].password);
        }
        free(combos);
        combos = NULL;
        cindex = 0;
    }
}

void combos_init(void)
{
    // Default/empty credentials
    combo_add("", "");
    combo_add("root", "");
    combo_add("admin", "");
    combo_add("default", "");

    // Common default accounts
    combo_add("root", "root");
    combo_add("admin", "admin");
    combo_add("user", "user");
    combo_add("guest", "guest");
    combo_add("support", "support");
    combo_add("test", "test");
    combo_add("demo", "demo");
    combo_add("oracle", "oracle");
    combo_add("postgres", "postgres");
    combo_add("mysql", "mysql");

    // Cross-combinations
    combo_add("root", "admin");
    combo_add("admin", "root");
    combo_add("root", "toor");
    combo_add("admin", "password");
    combo_add("root", "password");

    // Common numeric passwords
    combo_add("root", "123");
    combo_add("root", "1234");
    combo_add("root", "12345");
    combo_add("root", "123456");
    combo_add("root", "1234567");
    combo_add("root", "12345678");
    combo_add("root", "888888");
    combo_add("root", "000000");
    combo_add("root", "111111");
    combo_add("admin", "123");
    combo_add("admin", "1234");
    combo_add("admin", "12345");
    combo_add("admin", "123456");
    combo_add("admin", "1234567");
    combo_add("admin", "12345678");
    combo_add("guest", "123");
    combo_add("guest", "1234");
    combo_add("guest", "12345");
    combo_add("guest", "123456");

    // Common weak passwords
    combo_add("root", "changeme");
    combo_add("admin", "changeme");
    combo_add("root", "qwerty");
    combo_add("admin", "qwerty");
    combo_add("root", "letmein");
    combo_add("admin", "letmein");
    combo_add("root", "welcome");
    combo_add("admin", "welcome");

    // IoT device defaults
    combo_add("ubnt", "ubnt");
    combo_add("ubuntu", "ubuntu");
    combo_add("hikvision", "hikvision");
    combo_add("dahua", "dahua");
    combo_add("dvr", "dvr");
    combo_add("camera", "camera");
    combo_add("pi", "raspberry");
    combo_add("raspberry", "raspberry");

    // Service accounts
    combo_add("operator", "operator");
    combo_add("ftp", "ftp");
    combo_add("telnet", "telnet");
    combo_add("ssh", "ssh");
    combo_add("service", "service");
    combo_add("maintenance", "maintenance");

    // Network equipment
    combo_add("cisco", "cisco");
    combo_add("netgear", "netgear");
    combo_add("linksys", "linksys");
    combo_add("dlink", "dlink");
    combo_add("tplink", "tplink");

    // Fake/test accounts
    combo_add("fake", "fake");
    combo_add("1234", "1234");
    combo_add("5678", "5678");
    combo_add("0000", "0000");

    // Year-based passwords (common in IoT)
    combo_add("admin", "2023");
    combo_add("admin", "2024");
    combo_add("admin", "2025");
    combo_add("root", "2023");
    combo_add("root", "2024");
    combo_add("root", "2025");
}
