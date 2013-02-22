#ifndef FILE_H
#define FILE_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

#include <mime.h>

char *file_path_for(char *requested_filename);
char *file_content_type_for(MimeTypes *mime_types, char *filename);

#endif