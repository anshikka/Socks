#ifndef PHASE1_H
#define PHASE1_H

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#define DIRNULL NULL
#define FILENULL NULL

void recursiveTraverseFS(int mappers, char *basePath, FILE *fp[], int *toInsert, int *nFiles);
void traverseFS(int mappers, char *path);
void writeToLog(char * message);

#endif
