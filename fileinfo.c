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
bool isdir(const char *filename);
void show_file_entry(const char *filename);
char *get_permissions(mode_t mode);
void searchdir(const char *dirname, const char *filename, int depth, int *viewed_entries);

char *module;

int main(int argc, char *argv[], char *envp[])
{
    module = basename(argv[0]);

    if (argc < VALID_ARGC) {
        printerr(module, "Missing argument", NULL);
        return 1;
    }
    if (!isdir(argv[1])) {
        return 1;
    }

    int viewed_entries = 1;
    searchdir(argv[1], argv[2], 1, &viewed_entries);
    printf("\nViewed entries: %d\n", viewed_entries);

    return 0;
}

void printerr(const char *module, const char *errmsg, const char *filename)
{
    fprintf(stderr, "%s: %s %s\n", module, errmsg, filename ? filename : "");
}

bool isdir(const char *filename)
{
    struct stat statbuf;

    if (stat(filename, &statbuf) == -1) {
        printerr(module, strerror(errno), filename);
        return false;
    }

    return S_ISDIR(statbuf.st_mode);
}

void show_file_entry(const char *filename)
{
    struct stat statbuf;
    if (stat(filename, &statbuf) == -1) {
        printerr(module, strerror(errno), filename);
        return;
    }

    char *actualpath;
    actualpath = realpath(filename, NULL);
    if (!actualpath) {
        printerr(module, strerror(errno), filename);
        return;
    }

    char *perms;
    perms = get_permissions(statbuf.st_mode);

    printf("For %s:\n",  actualpath);
    printf("\tFile size: %lld bytes\n", (long long) statbuf.st_size);
    printf("\tLast file modification: %s", ctime(&statbuf.st_ctime));
    printf("\tPermissions: %s\n", perms);
    printf("\tI-node number: %ld\n", (long) statbuf.st_ino);

    free(actualpath);
    free(perms);
}

char *get_permissions(mode_t mode)
{
    char *perms;
    perms = (char *) malloc(sizeof(char) * (PERM_BITS_NUM + 1));

    perms[0] = mode & S_IRUSR ? 'r' : '-';
    perms[1] = mode & S_IWUSR ? 'w' : '-';
    perms[2] = mode & S_IXUSR ? 'x' : '-';
    perms[3] = mode & S_IRGRP ? 'r' : '-';
    perms[4] = mode & S_IWGRP ? 'w' : '-';
    perms[5] = mode & S_IXGRP ? 'x' : '-';
    perms[6] = mode & S_IROTH ? 'r' : '-';
    perms[7] = mode & S_IWOTH ? 'w' : '-';
    perms[8] = mode & S_IXOTH ? 'x' : '-';
    perms[9] = '\0';

    return perms;
}

void searchdir(const char *dirname, const char *filename, int depth, int *viewed_entries)
{
    char *bname = basename(filename);

    DIR *currdir;
    if (!(currdir = opendir(dirname))) {
        printerr(module, strerror(errno), dirname);
    }

    struct dirent *cdirent;
    while (cdirent = readdir(currdir)) {
        if (!strcmp(".", cdirent->d_name) || !strcmp("..", cdirent->d_name)) {
            continue;
        }

        (*viewed_entries)++;

        if (depth && isdir(cdirent->d_name)) {
            char new_filename[PATH_MAX];
            strcpy(new_filename, cdirent->d_name);
            strcat(new_filename, "/");
            strcat(new_filename, filename);
            searchdir(cdirent->d_name, new_filename, depth - 1, viewed_entries);
        } else if (!strcmp(cdirent->d_name, bname)) {
            show_file_entry(filename);
        }
    }

    closedir(currdir);
}
