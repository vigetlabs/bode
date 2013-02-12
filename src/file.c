#include <file.h>

int is_file(char *filename);
int is_dir(char *filename);
char *path_append(char *path, const char *filename);

FILE *
file_open_from_path(char *filename)
{
    char *full_path = NULL;
    FILE *fd        = NULL;

    if (is_file(filename)) {
        fd = fopen(filename, "r");
    } else if (is_dir(filename)) {
        full_path = path_append(filename, "index.html");
        fd = file_open_from_path(full_path);

        free(full_path);
    }

    return fd;
}

int
ends_with(char *path, char end)
{
    int index = strlen(path);

    return path[index] == end;
}

char *
path_append(char *path, const char *filename)
{
    char *full_path;
    char *format = calloc(6, sizeof(char));

    if (ends_with(path, '/')) {
        strncpy(format, "%s%s", 4);
    } else {
        strncpy(format, "%s/%s", 5);
    }

    asprintf(&full_path, format, path, filename);
    free(format);

    return full_path;
}

int
file_is_type(char *filename, int type)
{
    struct stat fs;

    if (stat(filename, &fs) == 0) {
        return (fs.st_mode & type) == type;
    }

    return 0;
}

int
is_file(char *filename)
{
    return file_is_type(filename, S_IFREG);
}

int
is_dir(char *filename)
{
    return file_is_type(filename, S_IFDIR);
}