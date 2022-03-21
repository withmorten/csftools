#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#ifdef CSF2STR
#define TOOLNAME "csf2str"
#elif STR2CSF
#define TOOLNAME "str2csf"
#endif

#define LINE_MAX 2048

#define CSFTOOLS_VERSION_MAJOR 0
#define CSFTOOLS_VERSION_MINOR 1

#define CSF_MAGIC 0x43534620
#define LBL_MAGIC 0x4C424C20
#define STR_MAGIC 0x53545220
#define STRW_MAGIC 0x53545257

#define CSF_VERSION_2 2
#define CSF_VERSION_3 3

#define CSF_LANGUAGE_STRING_ENUS "en-us"
#define CSF_LANGUAGE_STRING_ENUK "en-uk"
#define CSF_LANGUAGE_STRING_DE    "de"
#define CSF_LANGUAGE_STRING_FRFR "fr-fr"
#define CSF_LANGUAGE_STRING_ES    "es"
#define CSF_LANGUAGE_STRING_IT    "it"
#define CSF_LANGUAGE_STRING_JA    "ja"
#define CSF_LANGUAGE_STRING_JW    "jw"
#define CSF_LANGUAGE_STRING_KO    "ko"
#define CSF_LANGUAGE_STRING_CN    "cn"

enum CSFLanguageIDs
{
    CSF_LANGUAGE_ID_ENUS,
    CSF_LANGUAGE_ID_ENUK,
    CSF_LANGUAGE_ID_DE,
    CSF_LANGUAGE_ID_FRFR,
    CSF_LANGUAGE_ID_ES,
    CSF_LANGUAGE_ID_IT,
    CSF_LANGUAGE_ID_JA,
    CSF_LANGUAGE_ID_JW,
    CSF_LANGUAGE_ID_KO,
    CSF_LANGUAGE_ID_CN,
    CSF_LANGUAGE_NUM,
    CSF_LANGUAGE_UNKNOWN = -1
};

enum STRStates
{
    STR_STATE_LABEL,
    STR_STATE_VALUE,
    STR_STATE_END
};

typedef struct CSFHeader CSFHeader;
typedef struct CSFLabel CSFLabel;
typedef struct CSFString CSFString;

// From https://www.modenc.renegadeprojects.com/CSF_File_Format

struct CSFHeader
{
    uint32_t MagicHeader;
    uint32_t CSFVersion;
    uint32_t NumLabels;
    uint32_t NumStrings;
    uint32_t Unknown;
    uint32_t Language;
    CSFLabel *Label[];
};

struct CSFLabel
{
    uint32_t MagicHeader;
    uint32_t NumStringPairs;
    uint32_t LabelNameLength;
    char *LabelName;
    CSFString *String;
};

struct CSFString
{
    uint32_t MagicHeader;
    uint32_t ValueLength;
    char *Value;
};

// util.c
void printf_help_exit();
void printf_error_exit(char *message, char *labelname);
FILE *fopen_d(const char *path, const char *mode);
void *malloc_d(size_t size);
void *calloc_d(size_t nitems, size_t size);
void *realloc_d(void *ptr, size_t size);
void *recalloc_d(void *ptr, size_t ptrsize,  size_t nitems, size_t size);
char *fgets_t(char *str, int n, FILE *stream);
char *strtrim(char *str);
char *strupr(char *str);

// csf2str.c
void CSFFile_ConvertToSTRFile(char *CSFFile_Path, char *STRFile_Path);
CSFHeader *CSFFileHeader_Parse(FILE *CSFFile_Handle);
char *CSFFile_GetLanguageString(uint32_t LanguageId);

// str2csf.c
void STRFile_ConvertToCSFFile(char *STRFile_Path, char *CSFFile_Path, char *LanguageString);
CSFHeader *CSFFileHeader_Create(FILE *STRFile_Handle, uint32_t LanguageId);
uint32_t CSFFile_GetLanguageId(char *LanguageString);
