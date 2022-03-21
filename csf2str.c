#include "csftools.h"

int
main(int argc, char *argv[])
{
    // Not enough args
    if(argc < 3)
    {
        printf_help_exit();
    }

    printf("\n");

    // Exits on failure
    CSFFile_ConvertToSTRFile(argv[1], argv[2]);

    printf("\nSuccessfully converted %s to %s\n", argv[1], argv[2]);
}

void
CSFFile_ConvertToSTRFile(char *CSFFile_Path, char *STRFile_Path)
{
    FILE *CSFFile_Handle, *STRFile_Handle;
    CSFHeader *CSFFile_Header = NULL;
    CSFLabel *Label = NULL;
    char *LanguageString = NULL;
    char *StringValue = NULL;
    int i;

    // Open CSF file
    CSFFile_Handle = fopen_d(CSFFile_Path, "rb");

    // Create CSFFile header from CSF file
    CSFFile_Header = CSFFileHeader_Parse(CSFFile_Handle);

    // Close CSF file
    fclose(CSFFile_Handle);

    // Get LanguageString from CSF header
    LanguageString = CSFFile_GetLanguageString(CSFFile_Header->Language);

    if(!LanguageString)
    {
        printf("Warning: The language of this CSF file was not recognized\n");
    }
    else
    {
        printf("The language of this CSF file is '%s'\n", LanguageString);
    }

    // Open output STR file
    STRFile_Handle = fopen_d(STRFile_Path, "wb");

    // A label in a STR file is a triplet of lines:
    // The first one naming the Label
    // The second one starting and ending with apostrophes containing the String value
    // The third one containing the marker END

    // Iterate through CSFFile header and write each Label to STR file
    for(i = 0; i < CSFFile_Header->NumLabels; i++)
    {
        Label = CSFFile_Header->Label[i];

        // Write LabelName to file
        fprintf(STRFile_Handle, "%s\r\n", Label->LabelName);

        // StringValues in STR files are contained by apostrophes
        // STR files do not contain newline chars in String values, a newline is denoted by the string "\n", so we:
        // Iterate through string char by char, write a "\n" string instead of newline char, but write all other chars normally
        StringValue = Label->String->Value;

        fprintf(STRFile_Handle, "\"");

        while(*StringValue)
        {
            if(*StringValue == '\n')
            {
                fprintf(STRFile_Handle, "\\n");
            }
            else
            {
                fprintf(STRFile_Handle, "%c", *StringValue);
            }

            StringValue++;
        }

        fprintf(STRFile_Handle, "\"\r\n");

        // Write END to file to denote the end of this label with an empty line afterwards
        fprintf(STRFile_Handle, "END\r\n\r\n");
    }

    // Close output STR file, done
    fclose(STRFile_Handle);
}

CSFHeader *
CSFFileHeader_Parse(FILE *CSFFile_Handle)
{
    CSFHeader *CSFFile_Header = NULL;
    CSFLabel *Label = NULL;
    CSFString *String = NULL;
    char itoabuffer[9];
    char EncodedChar;
    uint32_t ExtraValueLength;
    int i, j, k;

    // Initial header malloc
    CSFFile_Header = malloc_d(sizeof(CSFHeader));

    // Read header from CSF file
    fread(CSFFile_Header, sizeof(CSFHeader), 1, CSFFile_Handle);

    // Some sanity checks for header values
    if(CSFFile_Header->MagicHeader != CSF_MAGIC)
    {
        printf_error_exit("Wrong header in CSF file, expected CSF", "");
    }

    if(CSFFile_Header->CSFVersion != CSF_VERSION_2 && CSFFile_Header->CSFVersion != CSF_VERSION_3)
    {
        printf_error_exit("CSF version is not 2 or 3, exiting", "");
    }

    if(CSFFile_Header->NumLabels != CSFFile_Header->NumStrings)
    {
        printf("Warning: Mismatch between Labelcount and Stringcount, %u vs. %u\n", CSFFile_Header->NumLabels, CSFFile_Header->NumStrings);
    }

    // Now realloc memory for all Labels
    CSFFile_Header = realloc_d(CSFFile_Header, sizeof(CSFHeader) + (CSFFile_Header->NumLabels * sizeof(CSFLabel *)));

    // Iterate through all labels
    for(i = 0; i < CSFFile_Header->NumLabels; i++)
    {
        // Label malloc
        Label = malloc_d(sizeof(CSFLabel));

        // Read Label struct members one by one from file due to alignment issues when compiling for x86_64
        fread(&Label->MagicHeader, sizeof(Label->MagicHeader), 1, CSFFile_Handle);
        fread(&Label->NumStringPairs, sizeof(Label->NumStringPairs), 1, CSFFile_Handle);
        fread(&Label->LabelNameLength, sizeof(Label->LabelNameLength), 1, CSFFile_Handle);

        // Sanity check for LBL magic value
        if(Label->MagicHeader != LBL_MAGIC)
        {
            printf_error_exit("Wrong header in Label, expected LBL at 0x", itoa(ftell(CSFFile_Handle), itoabuffer, 16));
        }

        // Calloc memory for non-null-terminated LabelName, + 1 length makes sure it *is* null-terminated
        // Read from file
        Label->LabelName = calloc_d(1, Label->LabelNameLength + 1);
        fread(Label->LabelName, Label->LabelNameLength, 1, CSFFile_Handle);

        // String malloc
        String = malloc_d(sizeof(CSFString));

        // Sometimes a Label can contain no string pair at all (see generals.csf from Zero Hour, for example)
        // So we fill it with some dummy data and just pretend it's an empty string, which is not unusual for STR files
        if(!Label->NumStringPairs)
        {
            printf("Warning: No String associated with Label %s\n", Label->LabelName);

            String->MagicHeader = STR_MAGIC;
            String->ValueLength = 0;
            String->Value = calloc_d(1, String->ValueLength + 1);
        }
        else
        {
            // Read String header from file
            fread(String, sizeof(CSFString) - sizeof(String->Value), 1, CSFFile_Handle);

            // Sanity check for STR or STRW magic value
            if(String->MagicHeader != STR_MAGIC && String->MagicHeader != STRW_MAGIC)
            {
                printf_error_exit("Wrong header in String, expected STR or STRW in Label ", Label->LabelName);
            }

            // Calloc memory for decoded String value, + 1 ensures it's null-terminated
            String->Value = calloc_d(1, String->ValueLength + 1);

            k = 0;

            // Read Encoded (notted) non-null-terminated value from file
            // Since the value is unicode the Value length needs to be multiplied by 2
            // Decode (not) string and skip empty Unicode bytes
            for(j = 0; j < String->ValueLength * 2; j++)
            {
                EncodedChar = fgetc(CSFFile_Handle);

                if(!(j % 2))
                {
                    String->Value[k] = ~EncodedChar;
                    k++;
                }
            }

            // Sometimes a String can have some extra data following it (i.e. generals.csf from Zero Hour), denoted by a STRW header instead of STR
            // Since there is no way to show this data properly in a STR file we'll just discard it
            if(String->MagicHeader == STRW_MAGIC)
            {
                printf("Warning: ExtraValue found in Label %s, discarding\n", Label->LabelName);

                // Read ExtraValueLength (immediately follows after StringValue) and fseek the number it contains forward
                fread(&ExtraValueLength, sizeof(ExtraValueLength), 1, CSFFile_Handle);
                fseek(CSFFile_Handle, (long int)ExtraValueLength, SEEK_CUR);
            }
        }

        // Add String to Label, add Label to CSFHeader
        Label->String = String;
        CSFFile_Header->Label[i] = Label;
    }

    return CSFFile_Header;
}

char *
CSFFile_GetLanguageString(uint32_t LanguageId)
{
    char *CSFLanguages[] =
    {
        CSF_LANGUAGE_STRING_ENUS,
        CSF_LANGUAGE_STRING_ENUK,
        CSF_LANGUAGE_STRING_DE,
        CSF_LANGUAGE_STRING_FRFR,
        CSF_LANGUAGE_STRING_ES,
        CSF_LANGUAGE_STRING_IT,
        CSF_LANGUAGE_STRING_JA,
        CSF_LANGUAGE_STRING_JW,
        CSF_LANGUAGE_STRING_KO,
        CSF_LANGUAGE_STRING_CN
    };

    if(LanguageId < CSF_LANGUAGE_NUM)
    {
        return CSFLanguages[LanguageId];
    }
    else
    {
        return NULL;
    }
}
