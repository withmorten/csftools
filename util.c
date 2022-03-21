#include "csftools.h"

void
printf_help_exit()
{
    printf("%s v%i.%i by withmorten\n\n", TOOLNAME, CSFTOOLS_VERSION_MAJOR, CSFTOOLS_VERSION_MINOR);
    printf("%s supports the following arguments:\n\n", TOOLNAME);
#ifdef CSF2STR
    printf("%s <csf input> <str output> to convert a csf file to a str file\n\n", TOOLNAME);
#elif STR2CSF
    printf("%s <str input> <csf output> <lang> to convert a str file to a csf file\n", TOOLNAME);
    printf("please refer to the readme for the available languages\n\n");
#endif
    printf("%s's source and readme are available at https://github.com/withmorten/csftools\n", TOOLNAME);
    exit(1);
}

void
printf_error_exit(char *message, char *labelname)
{
    printf("Error: %s%s\n", message, labelname);
    exit(1);
}

FILE *
fopen_d(const char *path, const char *mode)
{
    FILE *f;

    f = fopen(path, mode);

    if(f == NULL)
    {
        printf("Error: couldn't fopen() file %s, exiting\n", path);
        exit(1);
    }

    return f;
}

void *
malloc_d(size_t size)
{
    void *m;

    m = malloc(size);

    if(m == NULL && size != 0)
    {
        printf("Error: couldn't malloc() %i bytes of memory, exiting\n", (int)size);
        exit(1);
    }

    return m;
}

void *
calloc_d(size_t nitems, size_t size)
{
    void *m;

    m = calloc(nitems, size);

    if(m == NULL && size != 0)
    {
        printf("Error: couldn't calloc() %i bytes of memory, exiting\n", (int)size);
        exit(1);
    }

    return m;
}

void *
realloc_d(void *ptr, size_t size)
{
    void *m;

    m = realloc(ptr, size);

    if(m == NULL && size != 0)
    {
        printf("Error: couldn't realloc() %i bytes of memory, exiting\n", (int)size);
        exit(1);
    }

    return m;
}

void *
recalloc_d(void *ptr, size_t ptrsize,  size_t nitems, size_t size)
{
    void *m;

    m = calloc_d(nitems, size);

    memcpy(m, ptr, ptrsize);

    free(ptr);

    return m;
}


char *
fgets_t(char *str, int n, FILE *stream)
{
    if(!fgets(str, n, stream))
    {
        return NULL;
    }
    else
    {
        return strtrim(str);
    }
}

char *
strtrim(char *str)
{
    // Adapted from https://stackoverflow.com/a/122974

    size_t len = 0;
    char *frontp = str;
    char *endp = NULL;

    if(str == NULL)
    {
        return NULL;
    }

    if(*str == '\0')
    {
        return str;
    }

    len = strlen(str);
    endp = str + len;

    // Move the front and back pointers to address the first non-whitespace characters from each end.
    while(isspace((unsigned char)*frontp))
    {
        ++frontp;
    }

    if(endp != frontp)
    {
        while(isspace((unsigned char)*(--endp)) && endp != frontp);
    }

    if((str + len - 1) != endp)
    {
        *(endp + 1) = '\0';
    }
    else if(frontp != str &&  endp == frontp)
    {
        *str = '\0';
    }

    // Shift the string so that it starts at str so that if it's dynamically allocated we can still free it on the returned pointer.
    // Note the reuse of endp to mean the front of the string buffer now.
    endp = str;

    if(frontp != str)
    {
        while(*frontp)
        {
            *endp++ = *frontp++;
        }

        *endp = '\0';
    }

    return str;
}

char *
strupr(char* str)
{
    char *tmp = str;

    while(*tmp)
    {
        *tmp = toupper((unsigned char)*tmp);
        tmp++;
    }

    return str;
}
