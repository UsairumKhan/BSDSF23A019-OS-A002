/*
 * Programming Assignment 02: ls-v1.6.0
 * Feature: Recursive Listing (-R)
 * Also includes -l, -x, sorting, and color output.
 */

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <getopt.h>
#include <sys/ioctl.h>

void do_ls(const char *dir, int long_format, int horizontal, int recursive);
void print_permissions(mode_t mode);
void print_colored_name(const char *path, const char *name);
int cmpstringp(const void *a, const void *b);

/* -------------------- main() -------------------- */
int main(int argc, char *argv[]) {
    int long_format = 0, horizontal = 0, recursive = 0, opt;

    while ((opt = getopt(argc, argv, "lxR")) != -1) {
        if (opt == 'l') long_format = 1;
        else if (opt == 'x') horizontal = 1;
        else if (opt == 'R') recursive = 1;
        else {
            fprintf(stderr, "Usage: %s [-l | -x | -R] [directory...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
        do_ls(".", long_format, horizontal, recursive);
    else
        for (int i = optind; i < argc; i++) {
            printf("%s:\n", argv[i]);
            do_ls(argv[i], long_format, horizontal, recursive);
            puts("");
        }

    return 0;
}

/* -------------------- compare for sorting -------------------- */
int cmpstringp(const void *a, const void *b) {
    const char *pa = *(const char **)a;
    const char *pb = *(const char **)b;
    int r = strcasecmp(pa, pb);
    return (r == 0) ? strcmp(pa, pb) : r;
}

/* -------------------- print permissions -------------------- */
void print_permissions(mode_t mode) {
    char perms[11];
    perms[0] = S_ISDIR(mode) ? 'd' : '-';
    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? 'x' : '-';
    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? 'x' : '-';
    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? 'x' : '-';
    perms[10] = '\0';
    printf("%s ", perms);
}

/* -------------------- color printing -------------------- */
void print_colored_name(const char *path, const char *name) {
    struct stat sb;
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, name);

    if (lstat(fullpath, &sb) == -1) {
        printf("%s", name);
        return;
    }

    if (S_ISDIR(sb.st_mode))
        printf("\033[1;34m%s\033[0m", name); // Blue
    else if (S_ISLNK(sb.st_mode))
        printf("\033[1;36m%s\033[0m", name); // Cyan
    else if (sb.st_mode & S_IXUSR)
        printf("\033[1;32m%s\033[0m", name); // Green
    else
        printf("%s", name);                  // Default
}

/* -------------------- main listing logic -------------------- */
void do_ls(const char *dir, int long_format, int horizontal, int recursive) {
    DIR *dp = opendir(dir);
    if (!dp) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    struct dirent *entry;
    char **files = NULL;
    int count = 0;

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;
        files = realloc(files, sizeof(char*) * (count + 1));
        files[count++] = strdup(entry->d_name);
    }
    closedir(dp);

    qsort(files, count, sizeof(char*), cmpstringp);

    struct stat sb;
    char path[1024];

    /* Print files */
    for (int i = 0; i < count; i++) {
        snprintf(path, sizeof(path), "%s/%s", dir, files[i]);
        if (lstat(path, &sb) == -1) continue;

        if (long_format) {
            print_permissions(sb.st_mode);
            printf("%2ld %8ld ", sb.st_nlink, sb.st_size);

            char timebuf[32];
            strftime(timebuf, sizeof(timebuf), "%b %d %H:%M",
                     localtime(&sb.st_mtime));
            printf("%s ", timebuf);
        }

        print_colored_name(dir, files[i]);

        if (horizontal) printf("  ");
        else printf("\n");
    }

    if (horizontal) printf("\n");

    /* Recursion */
    if (recursive) {
        for (int i = 0; i < count; i++) {
            snprintf(path, sizeof(path), "%s/%s", dir, files[i]);
            if (lstat(path, &sb) == -1) continue;

            if (S_ISDIR(sb.st_mode) &&
                strcmp(files[i], ".") != 0 &&
                strcmp(files[i], "..") != 0) {
                printf("\n%s:\n", path);
                do_ls(path, long_format, horizontal, recursive);
            }
        }
    }

    for (int i = 0; i < count; i++) free(files[i]);
    free(files);
}
