#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h>
#include <getopt.h>

/* --- Function Prototypes --- */
void do_ls(const char *dir, int long_format, int horizontal);
void print_long_listing(const char *dir);
void print_permissions(mode_t mode);
void print_columns(const char *dir, int horizontal);

/* -------------------- main() -------------------- */
int main(int argc, char *argv[])
{
    int long_format = 0, horizontal = 0;
    int opt;

    /* Parse options: -l and -x */
    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        switch (opt)
        {
        case 'l': long_format = 1; break;
        case 'x': horizontal = 1; break;
        default:
            fprintf(stderr, "Usage: %s [-l] [-x] [directory...]\n", argv[0]);
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

/* -------------------- do_ls() -------------------- */
void do_ls(const char *dir, int long_format, int horizontal)
{
    if (long_format)
        print_long_listing(dir);
    else
        print_columns(dir, horizontal);
}

/* -------------------- print_permissions() -------------------- */
void print_permissions(mode_t mode)
{
    printf("%c", S_ISDIR(mode) ? 'd' : '-');
    printf("%c", (mode & S_IRUSR) ? 'r' : '-');
    printf("%c", (mode & S_IWUSR) ? 'w' : '-');
    printf("%c", (mode & S_IXUSR) ? 'x' : '-');
    printf("%c", (mode & S_IRGRP) ? 'r' : '-');
    printf("%c", (mode & S_IWGRP) ? 'w' : '-');
    printf("%c", (mode & S_IXGRP) ? 'x' : '-');
    printf("%c", (mode & S_IROTH) ? 'r' : '-');
    printf("%c", (mode & S_IWOTH) ? 'w' : '-');
    printf("%c", (mode & S_IXOTH) ? 'x' : '-');
    printf(" ");
}

/* -------------------- print_long_listing() -------------------- */
void print_long_listing(const char *dir)
{
    DIR *dp = opendir(dir);
    if (!dp) { perror("opendir"); return; }

    struct dirent *entry;
    struct stat sb;
    char path[1024];

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.') continue;

        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        if (lstat(path, &sb) == -1) continue;

        print_permissions(sb.st_mode);
        printf("%2ld ", sb.st_nlink);

        struct passwd *pw = getpwuid(sb.st_uid);
        struct group *gr = getgrgid(sb.st_gid);
        printf("%-8s %-8s ", pw ? pw->pw_name : "?", gr ? gr->gr_name : "?");

        printf("%8ld ", sb.st_size);

        char timebuf[64];
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&sb.st_mtime));
        printf("%s %s\n", timebuf, entry->d_name);
    }

    closedir(dp);
}

/* -------------------- print_columns() -------------------- */
void print_columns(const char *dir, int horizontal)
{
    DIR *dp = opendir(dir);
    if (!dp) { perror("opendir"); return; }

    struct dirent *entry;
    char **files = NULL;
    int count = 0, maxlen = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.') continue;
        files = realloc(files, (count + 1) * sizeof(char *));
        files[count] = strdup(entry->d_name);
        int len = strlen(entry->d_name);
        if (len > maxlen) maxlen = len;
        count++;
    }
    closedir(dp);
    if (count == 0) return;

    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        term_width = w.ws_col;

    int spacing = 2;
    int col_width = maxlen + spacing;
    int cols = term_width / col_width;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    /* Sort alphabetically */
    qsort(files, count, sizeof(char *),
          (int (*)(const void *, const void *))strcmp);

    /* Print logic */
    if (horizontal)
    {
        /* Across then down (Feature-4) */
        for (int r = 0; r < rows; r++)
        {
            for (int c = 0; c < cols; c++)
            {
                int idx = r * cols + c;
                if (idx < count)
                    printf("%-*s", col_width, files[idx]);
            }
            printf("\n");
        }
    }
    else
    {
        /* Down then across (Feature-3) */
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
    }

    for (int i = 0; i < count; i++)
        free(files[i]);
    free(files);
}
