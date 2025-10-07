#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

/* Function prototypes */
void do_ls(const char *dirname);
void print_in_columns(char **names, int count);

int main(int argc, char *argv[])
{
    if (argc == 1)
        do_ls(".");
    else
    {
        for (int i = 1; i < argc; i++)
        {
            printf("Directory listing of %s:\n", argv[i]);
            do_ls(argv[i]);
            puts("");
        }
    }
    return 0;
}

/* Read directory and store all file names */
void do_ls(const char *dirname)
{
    DIR *dp = opendir(dirname);
    if (!dp)
    {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    char **names = NULL;
    int count = 0;

    /* read all names into array */
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        names = realloc(names, sizeof(char *) * (count + 1));
        names[count] = strdup(entry->d_name);
        count++;
    }

    closedir(dp);

    /* print them nicely */
    if (count > 0)
        print_in_columns(names, count);

    /* free memory */
    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

/* Print filenames in multiple columns (down then across) */
void print_in_columns(char **names, int count)
{
    int maxlen = 0;
    for (int i = 0; i < count; i++)
    {
        int len = strlen(names[i]);
        if (len > maxlen)
            maxlen = len;
    }

    int term_width = 80;                 /* fixed width for simplicity */
    int spacing = 2;
    int col_width = maxlen + spacing;
    int cols = term_width / col_width;
    if (cols < 1) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int idx = c * rows + r;     /* down then across logic */
            if (idx < count)
                printf("%-*s", col_width, names[idx]);
        }
        printf("\n");
    }
}
