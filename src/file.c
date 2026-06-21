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
    const char *ext = file_get_extension(filename);

    return mime_types_find_content_type(mime_types, ext);
}

char *
file_get_extension(char *filename)
{
    // Locate the last '.' without mutating the caller's string. A name with no
    // dot (or a trailing dot) has no extension; return "" so the MIME lookup
    // falls back to the default type rather than matching on a bogus token.
    char *dot = strrchr(filename, '.');

    if (dot == NULL || *(dot + 1) == '\0') {
        return "";
    }

    return dot + 1;
}

int
ends_with(const char *path, char end)
{
    size_t length = strlen(path);

    if (length == 0) {
        return 0;
    }

    return path[length - 1] == end;
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