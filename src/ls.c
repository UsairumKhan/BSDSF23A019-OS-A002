#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>

/* Function prototypes */
void do_ls(const char *dirname, int long_format);
void show_details(const char *path, const char *filename);

int main(int argc, char *argv[])
{
    int long_format = 0;

    /* check if -l option is given */
    if (argc > 1 && strcmp(argv[1], "-l") == 0)
        long_format = 1;

    /* if only ./bin/ls -l */
    if (argc == 1 || (argc == 2 && long_format))
        do_ls(".", long_format);
    else
    {
        for (int i = (long_format ? 2 : 1); i < argc; i++)
        {
            printf("Directory listing of %s:\n", argv[i]);
            do_ls(argv[i], long_format);
            puts("");
        }
    }
    return 0;
}

/* List directory contents */
void do_ls(const char *dirname, int long_format)
{
    DIR *dp = opendir(dirname);
    if (!dp)
    {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')  /* skip hidden */
            continue;

        if (long_format)
            show_details(dirname, entry->d_name);
        else
            printf("%s\n", entry->d_name);
    }

    closedir(dp);
}

/* Print file details like ls -l */
void show_details(const char *path, const char *filename)
{
    char fullpath[1024];
    struct stat st;
    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, filename);

    if (lstat(fullpath, &st) == -1)
    {
        perror("lstat");
        return;
    }

    /* Permissions */
    printf((S_ISDIR(st.st_mode)) ? "d" : "-");
    printf((st.st_mode & S_IRUSR) ? "r" : "-");
    printf((st.st_mode & S_IWUSR) ? "w" : "-");
    printf((st.st_mode & S_IXUSR) ? "x" : "-");
    printf((st.st_mode & S_IRGRP) ? "r" : "-");
    printf((st.st_mode & S_IWGRP) ? "w" : "-");
    printf((st.st_mode & S_IXGRP) ? "x" : "-");
    printf((st.st_mode & S_IROTH) ? "r" : "-");
    printf((st.st_mode & S_IWOTH) ? "w" : "-");
    printf((st.st_mode & S_IXOTH) ? "x" : "-");

    /* Owner, group, size, time, name */
    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    char *time_str = ctime(&st.st_mtime);
    time_str[strlen(time_str) - 1] = '\0'; /* remove newline */

    printf(" %ld %s %s %ld %s %s\n",
           (long)st.st_nlink,
           pw ? pw->pw_name : "?",
           gr ? gr->gr_name : "?",
           (long)st.st_size,
           time_str,
           filename);
}
