/*
* Programming Assignment 02: ls-v1.4.0
* Feature Added: Alphabetical Sort (Case-Insensitive)
* Extends ls-v1.3.0 while retaining -l (long listing),
* column display, and horizontal (-x) features.
*/

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <strings.h>   // for strcasecmp()
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

/* --- Function Prototypes --- */
void do_ls(const char *dir, int long_format, int horizontal);
void print_long_listing(const char *dir);
void print_permissions(mode_t mode);
void print_columns(const char *dir);
void print_horizontal(const char *dir);
int cmpstringp(const void *a, const void *b);

/* -------------------- main() -------------------- */
int main(int argc, char const *argv[])
{
    int long_format = 0;
    int horizontal = 0;
    int opt;

    // Parse -l and -x options
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

    // No directory provided → use current directory
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

/* -------------------- cmpstringp() --------------------
 * Case-insensitive alphabetical comparator for qsort()
 * Also deterministic: prefers lowercase over uppercase if same letters
 */
int cmpstringp(const void *a, const void *b)
{
    const char *pa = *(const char * const *)a;
    const char *pb = *(const char * const *)b;

    int r = strcasecmp(pa, pb);
    if (r != 0)
        return r;

    // Tie-breaker for deterministic order
    return strcmp(pa, pb);
}

/* -------------------- do_ls() -------------------- */
void do_ls(const char *dir, int long_format, int horizontal)
{
    if (long_format)
        print_long_listing(dir);
    else if (horizontal)
        print_horizontal(dir);
    else
        print_columns(dir);
}

/* -------------------- print_permissions() -------------------- */
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

/* -------------------- print_long_listing() -------------------- */
void print_long_listing(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (!dp)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    // Read all filenames first
    char **files = NULL;
    int count = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        files = realloc(files, sizeof(char*) * (count + 1));
        files[count] = strdup(entry->d_name);
        count++;
    }
    closedir(dp);

    // Sort alphabetically (case-insensitive)
    qsort(files, count, sizeof(char*), cmpstringp);

    // Display details
    char path[1024];
    struct stat sb;
    for (int i = 0; i < count; i++)
    {
        snprintf(path, sizeof(path), "%s/%s", dir, files[i]);
        if (lstat(path, &sb) == -1)
        {
            perror("stat");
            continue;
        }

        print_permissions(sb.st_mode);
        printf("%2ld ", sb.st_nlink);

        struct passwd *pw = getpwuid(sb.st_uid);
        struct group *gr = getgrgid(sb.st_gid);
        printf("%-8s %-8s ", pw ? pw->pw_name : "?", gr ? gr->gr_name : "?");

        printf("%8ld ", sb.st_size);

        char timebuf[64];
        struct tm *tm_info = localtime(&sb.st_mtime);
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm_info);
        printf("%s ", timebuf);

        printf("%s\n", files[i]);
    }

    for (int i = 0; i < count; i++) free(files[i]);
    free(files);
}

/* -------------------- print_columns() -------------------- */
void print_columns(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (!dp)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    char **files = NULL;
    int count = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        files = realloc(files, sizeof(char*) * (count + 1));
        files[count] = strdup(entry->d_name);
        count++;
    }
    closedir(dp);

    if (count == 0)
        return;

    // Sort alphabetically
    qsort(files, count, sizeof(char*), cmpstringp);

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width = w.ws_col ? w.ws_col : 80;

    int maxlen = 0;
    for (int i = 0; i < count; i++)
        if ((int)strlen(files[i]) > maxlen) maxlen = strlen(files[i]);

    int spacing = 2;
    int col_width = maxlen + spacing;
    int cols = term_width / col_width;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int idx = r + c * rows;
            if (idx < count)
                printf("%-*s", col_width, files[idx]);
        }
        printf("\n");
    }

    for (int i = 0; i < count; i++) free(files[i]);
    free(files);
}

/* -------------------- print_horizontal() -------------------- */
void print_horizontal(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (!dp)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    char **files = NULL;
    int count = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        files = realloc(files, sizeof(char*) * (count + 1));
        files[count] = strdup(entry->d_name);
        count++;
    }
    closedir(dp);

    if (count == 0)
        return;

    // Sort alphabetically
    qsort(files, count, sizeof(char*), cmpstringp);

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width = w.ws_col ? w.ws_col : 80;

    int maxlen = 0;
    for (int i = 0; i < count; i++)
        if ((int)strlen(files[i]) > maxlen) maxlen = strlen(files[i]);

    int spacing = 2;
    int col_width = maxlen + spacing;

    int current_width = 0;
    for (int i = 0; i < count; i++)
    {
        int next_width = current_width + col_width;
        if (next_width > term_width)
        {
            printf("\n");
            current_width = 0;
        }
        printf("%-*s", col_width, files[i]);
        current_width += col_width;
    }
    printf("\n");

    for (int i = 0; i < count; i++) free(files[i]);
    free(files);
}
