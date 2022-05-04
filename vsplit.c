#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FILE_BUFFER_SIZE 10
#define FILE_BUFFER_INC 10

const char* programName = "vsplit";

int fileCount = 0;
int filesAllocated = 1;
FILE** files;

void closeFiles()
{
    for (int i = 0; i < fileCount; i++) {
        fclose(files[i]);
    }
    free(files);
}

void usage(int status)
{
    if (status == EXIT_FAILURE) {
        fprintf(stderr, "Try '%s -h' for more infomation.\n", programName);
    } else {
        printf("\
Usage: %s [OPTION]... [FILE]...\n\
",
            programName);

        printf("\
Write files to standard output side-by-side.\n\
\n\
  -s [STRING]    use [STRING] as a seperator\n\
");
    }
    exit(status);
}

void skipEscapeSequence(long* fi, char* s)
{
    if(s[*fi] == '\e') {
        *fi += 1;
        if(s[*fi] == '[') {
            *fi += 1;
            while(s[*fi] != 'm' && s[*fi] != 0) {
                *fi += 1;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    char* seperator = " ";

    files = malloc(filesAllocated * sizeof(FILE*));

    int opt;
    while ((opt = getopt(argc, argv, "--hs:")) != -1) {
        long optArgLen;
        switch (opt) {
        case 'h':
            closeFiles();
            usage(EXIT_SUCCESS);
        case 1:
            if (fileCount >= filesAllocated) {
                filesAllocated = fileCount + 5;
                files = realloc(files, filesAllocated * sizeof(FILE*));
            }
            files[fileCount] = fopen(optarg, "r");
            if (files[fileCount] == NULL) {
                fprintf(stderr, "failed to open file '%s'.", optarg);
                closeFiles();
                usage(EXIT_FAILURE);
            }
            fileCount += 1;
            break;
        case 's':
            optArgLen = strlen(optarg);
            seperator = malloc(optArgLen);
            strcpy(seperator, optarg);
            break;
        default:
            closeFiles();
            usage(EXIT_FAILURE);
        }
    }

    /* READ FILES INTO BUFFER */
    char** fileContents = malloc(fileCount * sizeof(char*));
    for (int i = 0; i < fileCount; i++) {
        int c, fi = 0, allocatedSpace = FILE_BUFFER_SIZE;
        fileContents[i] = malloc(allocatedSpace);
        while ((c = getc(files[i])) != EOF) {
            if(fi + 1 >= allocatedSpace) {
                allocatedSpace += FILE_BUFFER_INC;
                fileContents[i] = realloc(fileContents[i], allocatedSpace);
            }
            if(c == 0) {
                c = ' ';
            }
            fileContents[i][fi] = (char)c;
            fi += 1;
        }
        fileContents[i][fi] = 0;
    }
    closeFiles();

    int* maxWidths = malloc(fileCount * sizeof(int));
    memset(maxWidths, 0, fileCount * sizeof(int));
    for (int i = 0; i < fileCount; i++) {
        long fileProgress = 0;
        int w = 0;
        char c;
        while ((c = fileContents[i][fileProgress]) != 0) {
            fileProgress += 1;
            if (c == '\n') {
                if (w > maxWidths[i])
                    maxWidths[i] = w;
                w = 0;
            } else if(c == '\e') {
                fileProgress -= 1;
                skipEscapeSequence(&fileProgress, fileContents[i]);
            } else {
                w += 1;
            }
        }
    }

    long* fileProgress = malloc(fileCount * sizeof(long));
    memset(fileProgress, 0, fileCount * sizeof(long));
    bool end = false;
    while (!end) {
        end = true;
        for (int i = 0; i < fileCount; i++) {
            int w = 0;
            int c;
            while ((c = fileContents[i][fileProgress[i]]) != 0) {
                fileProgress[i] += 1;
                if (c == '\n') {
                    if(fileContents[i][fileProgress[i]] != 0)
                        end = false;
                    break;
                }
                putchar(c);
                if(c == '\e') {
                    long beforeSkipProg = fileProgress[i];
                    fileProgress[i] -= 1;
                    skipEscapeSequence(&fileProgress[i], fileContents[i]);
                    while(beforeSkipProg < fileProgress[i]) {
                        c = fileContents[i][beforeSkipProg];
                        beforeSkipProg += 1;
                        putchar(c);
                    }
                } else {
                    w += 1;
                }
            }
            while (w < maxWidths[i]) {
                putchar(' ');
                w += 1;
            }
            printf("%s", seperator);
        }
        putchar('\n');
    }

    free(maxWidths);
    return 0;
}
