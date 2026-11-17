#ifndef VALIDATION_H
#define VALIDATION_H

int valid_uid(const char *uid);

int valid_password(const char *password);

int valid_filename(const char *filename);

int valid_label(const char *label);

bool is_file_published(const char *filename, const char *uid, const char *dsip, const char *dsport, int *fd);

#endif // VALIDATION_H