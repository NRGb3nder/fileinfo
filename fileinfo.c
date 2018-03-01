#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#define VALID_ARGC 3
#define PERM_BITS_NUM 9

void printerr(const char *module_name, const char *errmsg, const char *filename);
bool isdir(const char *path);
void show_file_entry(const char *filepath);
void write_permissions_to_buf(mode_t mode, char *buf);
void searchdir(const char *dirpath, const char *filename, int depth, int *scanned_entries);

char *module;

int main(int argc, char *argv[], char *envp[])
{
    module = basename(argv[0]);

    if (argc < VALID_ARGC) {
        printerr(module, "Too few arguments", NULL);
        return 1;
    }
    if (!isdir(argv[1])) {
        return 1;
    }

    int scanned_entries = 1;
    searchdir(argv[1], argv[2], 1, &scanned_entries);
    printf("Scanned entries: %d\n", scanned_entries);

    return 0;
}

void printerr(const char *module, const char *errmsg, const char *filename)
{
    fprintf(stderr, "%s: %s %s\n", module, errmsg, filename ? filename : "");
}

bool isdir(const char *path)
{
    struct stat statbuf;

    if (stat(path, &statbuf) == -1) {
        printerr(module, strerror(errno), path);
        return false;
    }

    return S_ISDIR(statbuf.st_mode);
}

void show_file_entry(const char *filepath)
{
    struct stat statbuf;
    if (stat(filepath, &statbuf) == -1) {
        printerr(module, strerror(errno), filepath);
        return;
    }

    char actualpath[PATH_MAX];
    if (!realpath(filepath, actualpath)) {
        printerr(module, strerror(errno), filepath);
        return;
    }

    char perms[PERM_BITS_NUM + 1];
    write_permissions_to_buf(statbuf.st_mode, perms);

    printf("For %s:\n",  actualpath);
    printf("\tFile size: %lld bytes\n", (long long) statbuf.st_size);
    printf("\tLast file modification: %s", ctime(&statbuf.st_ctime));
    printf("\tPermissions: %s\n", perms);
    printf("\tI-node number: %ld\n", (long) statbuf.st_ino);
    printf("\n");
}

void write_permissions_to_buf(mode_t mode, char *buf)
{
    buf[0] = mode & S_IRUSR ? 'r' : '-';
    buf[1] = mode & S_IWUSR ? 'w' : '-';
    buf[2] = mode & S_IXUSR ? 'x' : '-';
    buf[3] = mode & S_IRGRP ? 'r' : '-';
    buf[4] = mode & S_IWGRP ? 'w' : '-';
    buf[5] = mode & S_IXGRP ? 'x' : '-';
    buf[6] = mode & S_IROTH ? 'r' : '-';
    buf[7] = mode & S_IWOTH ? 'w' : '-';
    buf[8] = mode & S_IXOTH ? 'x' : '-';
    buf[9] = '\0';
}

void searchdir(const char *dirpath, const char *filename, int depth, int *scanned_entries)
{
    char *bname = basename(filename);
    char filepath[PATH_MAX];
    strcpy(filepath, dirpath);
    strcat(filepath, "/");
    strcat(filepath, filename);

    DIR *currdir;
    if (!(currdir = opendir(dirpath))) {
        printerr(module, strerror(errno), dirpath);
    }

    struct dirent *cdirent;
    while (cdirent = readdir(currdir)) {
        if (!strcmp(".", cdirent->d_name) || !strcmp("..", cdirent->d_name)) {
            continue;
        }

        (*scanned_entries)++;

        char new_dirpath[PATH_MAX];
        strcpy(new_dirpath, dirpath);
        strcat(new_dirpath, "/");
        strcat(new_dirpath, cdirent->d_name);
        if (depth && isdir(new_dirpath)) {
            searchdir(new_dirpath, filename, depth - 1, scanned_entries);
        } else if (!strcmp(cdirent->d_name, bname)) {
            show_file_entry(filepath);
        }
    }

    closedir(currdir);
}
