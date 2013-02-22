#include <file.h>
#include <mime.h>

int is_file(char *filename);
int is_dir(char *filename);
char *path_append(char *path, const char *filename);
char *file_get_extension(char *filename);

char *
file_path_for(char *requested_filename)
{
    char *filename = NULL;

    if (is_file(requested_filename)) {
        filename = strdup(requested_filename);
    } else if (is_dir(requested_filename)) {
        filename = path_append(requested_filename, "index.html");
    }

    return filename;
}

char *
file_content_type_for(MimeTypes *mime_types, char *filename)
{
    char *ext = file_get_extension(filename);

    return mime_types_find_content_type(mime_types, ext);
}

char *
file_get_extension(char *filename)
{
    const char *d = "."; // split on '.'

    char *token,
         *ext,
         *fp = filename;

    while ((token = strsep(&fp, d)) != NULL) {
        ext = token;
    }

    return ext;
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