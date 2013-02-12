#ifndef FILE_H
#define FILE_H

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

FILE *file_open_from_path(char *filename);

#endif