/*
 * Programming Assignment 02: ls-v1.5.0 (colorized)
 * Compatible with single-file Makefile (SRC=src/ls.c, OBJ=obj/ls.o, BIN=bin/ls)
 *
 * Features:
 *  - sorted (case-insensitive)
 *  - long listing (-l)
 *  - horizontal across display (-x)
 *  - colorized output (dirs/executables/links)
 */

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <strings.h>   /* strcasecmp */
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <ctype.h>

extern int errno;

/* prototypes */
void do_ls(const char *dir, int long_format, int horizontal);
void print_long_listing(const char *dir);
void print_permissions(mode_t mode);
void print_columns(const char *dir);
void print_horizontal(const char *dir);
int cmpstringp(const void *a, const void *b);
void print_colored_name(const char *path, const char *name);

/* ANSI color codes */
#define COLOR_DIR   "\033[1;34m"
#define COLOR_EXE   "\033[1;32m"
#define COLOR_LINK  "\033[1;36m"
#define COLOR_RESET "\033[0m"

int main(int argc, char const *argv[])
{
    int long_format = 0;
    int horizontal = 0;
    int opt;

    while ((opt = getopt(argc, (char * const *)argv, "lx")) != -1)
    {
        switch (opt)
        {
        case 'l': long_format = 1; break;
        case 'x': horizontal = 1; break;
        default:
            fprintf(stderr, "Usage: %s [-l | -x] [directory...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
        do_ls(".", long_format, horizontal);
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s:\n", argv[i]);
            do_ls(argv[i], long_format, horizontal);
            puts("");
        }
    }

    return 0;
}

/* case-insensitive comparator with deterministic tie-breaker */
int cmpstringp(const void *a, const void *b)
{
    const char *pa = *(const char * const *)a;
    const char *pb = *(const char * const *)b;

    int r = strcasecmp(pa, pb);
    if (r != 0)
        return r;
    return strcmp(pa, pb);
}

/* print colored name based on file type/perm */
void print_colored_name(const char *path, const char *name)
{
    struct stat sb;
    char fullpath[1024];

    /* build path/name */
    if (snprintf(fullpath, sizeof(fullpath), "%s/%s", path, name) >= (int)sizeof(fullpath)) {
        /* path too long: just print name without color */
        printf("%s", name);
        return;
    }

    if (lstat(fullpath, &sb) == -1) {
        /* if lstat fails, print plain */
        printf("%s", name);
        return;
    }

    if (S_ISDIR(sb.st_mode))
        printf(COLOR_DIR "%s" COLOR_RESET, name);
    else if (S_ISLNK(sb.st_mode))
        printf(COLOR_LINK "%s" COLOR_RESET, name);
    else if (sb.st_mode & S_IXUSR)
        printf(COLOR_EXE "%s" COLOR_RESET, name);
    else
        printf("%s", name);
}

/* driver */
void do_ls(const char *dir, int long_format, int horizontal)
{
    if (long_format)
        print_long_listing(dir);
    else if (horizontal)
        print_horizontal(dir);
    else
        print_columns(dir);
}

/* prints perms like ls -l */
void print_permissions(mode_t mode)
{
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

/* ---------------- long listing (sorted) ---------------- */
void print_long_listing(const char *dir)
{
    DIR *dp = opendir(dir);
    if (!dp) { perror("opendir"); return; }

    struct dirent *entry;
    char **files = NULL;
    int count = 0;

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char **tmp = realloc(files, sizeof(char*) * (count + 1));
        if (!tmp) { perror("realloc"); /* cleanup */ for (int i=0;i<count;i++) free(files[i]); free(files); closedir(dp); return; }
        files = tmp;
        files[count] = strdup(entry->d_name);
        if (!files[count]) { perror("strdup"); /* cleanup */ for (int i=0;i<count;i++) free(files[i]); free(files); closedir(dp); return; }
        count++;
    }
    closedir(dp);

    if (count > 0)
        qsort(files, count, sizeof(char*), cmpstringp);

    struct stat sb;
    char path[1024];

    for (int i = 0; i < count; i++) {
        snprintf(path, sizeof(path), "%s/%s", dir, files[i]);
        if (lstat(path, &sb) == -1) { /* if stat fails just show name */ 
            printf("%s\n", files[i]);
            continue;
        }

        print_permissions(sb.st_mode);
        printf("%2ld ", (long)sb.st_nlink);

        struct passwd *pw = getpwuid(sb.st_uid);
        struct group *gr = getgrgid(sb.st_gid);
        printf("%-8s %-8s ", pw ? pw->pw_name : "?", gr ? gr->gr_name : "?");

        printf("%8ld ", (long)sb.st_size);

        char timebuf[64];
        struct tm *tm_info = localtime(&sb.st_mtime);
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm_info);
        printf("%s ", timebuf);

        print_colored_name(dir, files[i]);
        printf("\n");
    }

    for (int i = 0; i < count; i++) free(files[i]);
    free(files);
}

/* ---------------- columns (down-then-across) ---------------- */
void print_columns(const char *dir)
{
    DIR *dp = opendir(dir);
    if (!dp) { perror("opendir"); return; }

    struct dirent *entry;
    char **files = NULL;
    int count = 0, maxlen = 0;

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char **tmp = realloc(files, sizeof(char*) * (count + 1));
        if (!tmp) { perror("realloc"); for (int i=0;i<count;i++) free(files[i]); free(files); closedir(dp); return; }
        files = tmp;
        files[count] = strdup(entry->d_name);
        if (!files[count]) { perror("strdup"); for (int i=0;i<count;i++) free(files[i]); free(files); closedir(dp); return; }
        int len = (int)strlen(files[count]);
        if (len > maxlen) maxlen = len;
        count++;
    }
    closedir(dp);
    if (count == 0) return;

    qsort(files, count, sizeof(char*), cmpstringp);

    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) term_width = w.ws_col;

    int spacing = 2;
    int col_width = maxlen + spacing;
    int cols = term_width / col_width;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    struct stat sb;
    char path[1024];

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = r + c * rows;
            if (idx < count) {
                snprintf(path, sizeof(path), "%s/%s", dir, files[idx]);
                if (lstat(path, &sb) == -1) {
                    /* if stat fails, print plain name */
                    printf("%-*s", col_width, files[idx]);
                } else {
                    /* print colored name then pad */
                    print_colored_name(dir, files[idx]);
                    int len = (int)strlen(files[idx]); /* visible length */
                    int pad = col_width - len;
                    for (int p = 0; p < pad; p++) putchar(' ');
                }
            }
        }
        putchar('\n');
    }

    for (int i = 0; i < count; i++) free(files[i]);
    free(files);
}

/* ---------- horizontal (across-then-down) ---------- */
void print_horizontal(const char *dir)
{
    DIR *dp = opendir(dir);
    if (!dp) { perror("opendir"); return; }

    struct dirent *entry;
    char **files = NULL;
    int count = 0, maxlen = 0;

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        char **tmp = realloc(files, sizeof(char*) * (count + 1));
        if (!tmp) { perror("realloc"); for (int i=0;i<count;i++) free(files[i]); free(files); closedir(dp); return; }
        files = tmp;
        files[count] = strdup(entry->d_name);
        if (!files[count]) { perror("strdup"); for (int i=0;i<count;i++) free(files[i]); free(files); closedir(dp); return; }
        int len = (int)strlen(files[count]);
        if (len > maxlen) maxlen = len;
        count++;
    }
    closedir(dp);
    if (count == 0) return;

    qsort(files, count, sizeof(char*), cmpstringp);

    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) term_width = w.ws_col;

    int spacing = 2;
    int col_width = maxlen + spacing;
    int current_width = 0;

    struct stat sb;
    char path[1024];

    for (int i = 0; i < count; i++) {
        int next_width = current_width + col_width;
        if (next_width > term_width) {
            putchar('\n');
            current_width = 0;
        }
        snprintf(path, sizeof(path), "%s/%s", dir, files[i]);
        if (lstat(path, &sb) == -1) {
            printf("%-*s", col_width, files[i]);
            current_width += col_width;
        } else {
            print_colored_name(dir, files[i]);
            int len = (int)strlen(files[i]);
            int pad = col_width - len;
            for (int p = 0; p < pad; p++) putchar(' ');
            current_width += col_width;
        }
    }
    putchar('\n');

    for (int i = 0; i < count; i++) free(files[i]);
    free(files);
}
