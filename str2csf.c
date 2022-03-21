#include "csftools.h"

int
main(int argc, char *argv[])
{
    // Not enough args
    if(argc < 4)
    {
        printf_help_exit();
    }

    printf("\n");

    // Exits on failure
    STRFile_ConvertToCSFFile(argv[1], argv[2], argv[3]);

    printf("\nSuccessfully converted %s to %s\n", argv[1], argv[2]);

    return 0;
}

void
STRFile_ConvertToCSFFile(char *STRFile_Path, char *CSFFile_Path, char *LanguageString)
{
    FILE *STRFile_Handle, *CSFFile_Handle;
    CSFHeader *CSFFile_Header = NULL;
    CSFLabel *Label = NULL;
    uint32_t LanguageId;
    int i;

    // Get LanguageId from argument
    LanguageId = CSFFile_GetLanguageId(LanguageString);

    if((int32_t)LanguageId == CSF_LANGUAGE_UNKNOWN)
    {
        printf_error_exit("Unsupported language string, please refer to the readme for the available languages", "");
    }

    printf("Creating CSF file with language '%s'\n", LanguageString);

    // Open STR file
    STRFile_Handle = fopen_d(STRFile_Path, "rb");

    // Create CSFFile header from STR file
    CSFFile_Header = CSFFileHeader_Create(STRFile_Handle, LanguageId);

    // Close STR file
    fclose(STRFile_Handle);

    // Open output CSF file
    CSFFile_Handle = fopen_d(CSFFile_Path, "wb");

    // Write CSFHeader to file
    fwrite(CSFFile_Header, sizeof(CSFHeader), 1, CSFFile_Handle);

    // Iterate through labels
    for(i = 0; i < CSFFile_Header->NumLabels; i++)
    {
        Label = CSFFile_Header->Label[i];

        // Write Label struct members one by one to file due to alignment issues when compiling for x86_64
        fwrite(&Label->MagicHeader, sizeof(Label->MagicHeader), 1, CSFFile_Handle);
        fwrite(&Label->NumStringPairs, sizeof(Label->NumStringPairs), 1, CSFFile_Handle);
        fwrite(&Label->LabelNameLength, sizeof(Label->LabelNameLength), 1, CSFFile_Handle);

        // Write non-null-terminated LabelName to file
        fwrite(Label->LabelName, Label->LabelNameLength, 1, CSFFile_Handle);

        // Write String header to file
        fwrite(Label->String, sizeof(CSFString) - sizeof(Label->String->Value), 1, CSFFile_Handle);

        // Write non-null-terminated String value to file, in the file it is a notted unicode string
        // So not every char and add a notted '\0' after
        while(*Label->String->Value)
        {
            fputc(~(*Label->String->Value), CSFFile_Handle);
            fputc(~('\0'), CSFFile_Handle);

            Label->String->Value++;
        }
    }

    // Close output CSF file, done
    fclose(CSFFile_Handle);
}

CSFHeader *
CSFFileHeader_Create(FILE *STRFile_Handle, uint32_t LanguageId)
{
    CSFHeader *CSFFile_Header = NULL;
    CSFLabel *Label = NULL;
    CSFString *String = NULL;
    char STRFile_Line[LINE_MAX];
    char *StringValue = NULL;
    char *StringValueLF = NULL;
    int STRFile_Line_Len, STRState;
    int CSFFile_Header_AllocSize = 10000;

    // Initial header malloc
    CSFFile_Header = malloc_d(sizeof(CSFHeader) + (CSFFile_Header_AllocSize * sizeof(CSFLabel *)));

    // Set some default values, NumLabels and NumStrings will be updated while reading STR file
    CSFFile_Header->MagicHeader = CSF_MAGIC;
    CSFFile_Header->CSFVersion = CSF_VERSION_3;
    CSFFile_Header->NumLabels = 0;
    CSFFile_Header->NumStrings = 0;
    CSFFile_Header->Unknown = 0;
    CSFFile_Header->Language = LanguageId;

    // Iterate through STR file
    STRState = STR_STATE_LABEL;

    while(fgets_t(STRFile_Line, LINE_MAX, STRFile_Handle))
    {
        // STRFile_Line is trimmed of leading and trailing whitespaces
        STRFile_Line_Len = strlen(STRFile_Line);

        // A label in a STR file is a triplet of lines:
        // The first one naming the Label
        // The second one starting and ending with apostrophes containing the String value
        // The third one containing the case insensitive marker END

        // Only parse line if it actually contains something other than whitespaces
        // Also skip if it starts with "//"
        if(STRFile_Line_Len > 0 && (STRFile_Line[0] != '/' && STRFile_Line[1] != '/'))
        {
            switch(STRState)
            {
            case STR_STATE_LABEL:
                // Create Label, fill with data and copy Line contents into LabelName
                Label = malloc_d(sizeof(CSFLabel));

                Label->MagicHeader = LBL_MAGIC;
                Label->NumStringPairs = 1;

                Label->LabelNameLength = STRFile_Line_Len;
                Label->LabelName = malloc_d(STRFile_Line_Len + 1);

                strcpy(Label->LabelName, STRFile_Line);

                STRState = STR_STATE_VALUE;

                break;
            case STR_STATE_VALUE:
                // If this line doesn't start or end with apostrophes the STR file is malformed
                // STR files do not contain newline chars in String values, a newline is denoted by the string "\n"
                if(STRFile_Line[0] != '"' || STRFile_Line[STRFile_Line_Len - 1] != '"')
                {
                    printf_error_exit("Malformed STR file, expected Value at Label ", Label->LabelName);
                }
                else
                {
                    // Create String for Label, fill with data
                    // Take care of newline strings, then trim apostrophes from front and end, then finally add new values to String
                    // Finally add String to Label
                    String = malloc_d(sizeof(CSFString));

                    String->MagicHeader = STR_MAGIC;

                    StringValue = malloc_d(STRFile_Line_Len + 1);
                    strcpy(StringValue, STRFile_Line);

                    while((StringValueLF = strstr(StringValue, "\\n")))
                    {
                        *StringValueLF = '\0';
                        *(++StringValueLF) = '\n';

                        strcat(StringValue, StringValueLF);

                        STRFile_Line_Len--;
                    }

                    StringValue[STRFile_Line_Len - 1] = '\0';
                    StringValue++;

                    String->ValueLength = STRFile_Line_Len - 1 - 1;
                    String->Value = StringValue;

                    Label->String = String;

                    STRState = STR_STATE_END;
                }

                break;
            case STR_STATE_END:
                // If this line isn't END, my only friend the End, the STR file is malformed
                if(strcmp(strupr(STRFile_Line), "END"))
                {
                    printf_error_exit("Malformed STR file, expected END at Label ", Label->LabelName);
                }
                else
                {
                    // Check if the alloc'd memory is still big enough, if not allocate twice as much
                    if(CSFFile_Header->NumLabels == CSFFile_Header_AllocSize)
                    {
                        CSFFile_Header_AllocSize *= 2;
                        CSFFile_Header = realloc_d(CSFFile_Header, sizeof(CSFHeader) + (CSFFile_Header_AllocSize * sizeof(CSFLabel *)));
                    }

                    // Add Label to CSFHeader, increase NumLabels and NumStrings
                    CSFFile_Header->Label[CSFFile_Header->NumLabels] = Label;

                    CSFFile_Header->NumLabels++;
                    CSFFile_Header->NumStrings++;

                    STRState = STR_STATE_LABEL;
                }

                break;
            }
        }
    }

    return CSFFile_Header;
}

uint32_t
CSFFile_GetLanguageId(char *LanguageString)
{
    if(!strcmp(CSF_LANGUAGE_STRING_ENUS, LanguageString))
    {
        return CSF_LANGUAGE_ID_ENUS;
    }
    else if(!strcmp(CSF_LANGUAGE_STRING_ENUK, LanguageString))
    {
        return CSF_LANGUAGE_ID_ENUK;
    }
    else if(!strcmp(CSF_LANGUAGE_STRING_DE, LanguageString))
    {
        return CSF_LANGUAGE_ID_DE;
    }
    else if(!strcmp(CSF_LANGUAGE_STRING_FRFR, LanguageString))
    {
        return CSF_LANGUAGE_ID_FRFR;
    }
    else if(!strcmp(CSF_LANGUAGE_STRING_ES, LanguageString))
    {
        return CSF_LANGUAGE_ID_ES;
    }
    else if(!strcmp(CSF_LANGUAGE_STRING_IT, LanguageString))
    {
        return CSF_LANGUAGE_ID_IT;
    }
    else if(!strcmp(CSF_LANGUAGE_STRING_JA, LanguageString))
    {
        return CSF_LANGUAGE_ID_JA;
    }
    else if(!strcmp(CSF_LANGUAGE_STRING_JW, LanguageString))
    {
        return CSF_LANGUAGE_ID_JW;
    }
    else if(!strcmp(CSF_LANGUAGE_STRING_KO, LanguageString))
    {
        return CSF_LANGUAGE_ID_KO;
    }
    else if(!strcmp(CSF_LANGUAGE_STRING_CN, LanguageString))
    {
        return CSF_LANGUAGE_ID_CN;
    }
    else
    {
        return CSF_LANGUAGE_UNKNOWN;
    }
}
