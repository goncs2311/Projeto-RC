#include "validation.h"

#include <ctype.h>
#include <string.h>

int valid_filename(const char *filename) {
    int len = strlen(filename);

    // Maximum 24 characters
    if (len > 24) {
        return 0;
    }

    // Find the dot
    const char *dot = strrchr(filename, '.');
    if (dot == NULL) {
        return 0;
    }
    if (dot == filename) {
        return 0;
    }

    // Extension must have exactly 3 characters
    if (strlen(dot + 1) != 3) {
        return 0;
    }

    // Check filename base
    for (const char *p = filename; p < dot; p++) {
        if (!isalnum(*p) && *p != '-' && *p != '_') {
            return 0;
        }
    }

    // Check extension
    for (const char *p = dot + 1; *p != '\0'; p++) {
        if (!isalnum(*p)) {
            return 0;
        }
    }

    return 1;
}

int valid_label(const char *label) {
    int len = strlen(label);

    // Length must be between 1 and 20
    if (len < 1 || len > 20) {
        return 0;
    }

    // Only letters, digits, '-' and '_'
    for (int i = 0; i < len; i++) {
        if (!isalnum(label[i]) && label[i] != '-' && label[i] != '_') {
            return 0;
        }
    }

    return 1;
}