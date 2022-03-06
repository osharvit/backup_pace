/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_driver_app_helper.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 12/16/2020
 *
 * DESCRIPTION        : Sync Timing Driver App Helper Functions
 *
 ****************************************************************************************/
 
/****************************************************************************************/
/**                  Copyright (c) 2020, 2021 Skyworks Solution Inc.                   **/
/****************************************************************************************/
/** This software is provided 'as-is', without any express or implied warranty.        **/
/** In no event will the authors be held liable for any damages arising from the use   **/
/** of this software.                                                                  **/
/** Permission is granted to anyone to use this software for any purpose, including    **/
/** commercial applications, and to alter it and redistribute it freely, subject to    **/
/** the following restrictions:                                                        **/
/** 1. The origin of this software must not be misrepresented; you must not claim that **/
/**    you wrote the original software. If you use this software in a product,         **/
/**    an acknowledgment in the product documentation would be appreciated but is not  **/
/**    required.                                                                       **/
/** 2. Altered source versions must be plainly marked as such, and must not be         **/
/**    misrepresented as being the original software.                                  **/
/** 3. This notice may not be removed or altered from any source distribution.         **/
/****************************************************************************************/

/*****************************************************************************************
    Include Header Files
    (No absolute paths - paths will be handled by Makefile)
*****************************************************************************************/

#include "sync_timing_driver_app.h"

/*****************************************************************************************
* Static global variables
****************************************************************************************/

static char         gInputScriptfile[SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ/2];

static char         gOutputfile[SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ/2];

SYNC_TIMING_BOOL_E  bLogScriptCmdOutputToFile   = SYNC_TIMING_FALSE;
FILE                *fpScriptCmdOutput          = NULL;
char                scriptCmd[SYNC_TIMING_DRIVER_APP_MAX_CMD_SIZE]           = {0};


/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_Is_Number
 *
 * DESCRIPTION   : This helper function checks if a string is a number and if so determines 
 *                 the base as well.
 *
 * IN PARAMS     : str    - The string that needs to be processed/checked
 *
 * OUT PARAMS    : base   - If str is a number, store the base (10 or 16) in param base.
 *
 * RETURN VALUE  : Return 1 if str is a number, 0 otherwise.
 *
 ****************************************************************************************/
int32_t Sync_Timing_DriverApp_Is_Number(char *str, uint32_t *base)
{
    // Check for null pointer.
    if (str == NULL)
        return 0;

    int i;
    int len = strlen(str);

    // Single character case.
    if (len == 1)
    {
        *base = 10;
        return isdigit(str[0]);
    }

    // Hexadecimal? At this point, we know length is at least 2.
    if ((str[0] == '0') && (str[1] == 'x'))
    {
        // Check that every character is a digit or a,b,c,d,e, or f.
        for (i = 2; i < len; i++)
        {
            char c = str[i];
            c = tolower(c);
            if (!(
                (c >= '0' && c <= '9') || 
                (c >= 'a' && c <= 'f')))
                return 0;
        }
        *base = 16;
    }
    // It's decimal.
    else
    {
        i = 0;
        // Accept signs.
        if (str[0] == '-' || str[0] == '+')
            i = 1;

        // Check that every character is a digit.
        for (; i < len; i++)
        {
            if (!isdigit(str[i]))
                return 0;
        }
        *base = 10;
    }
    return 1;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_ProcessScript
 *
 * DESCRIPTION   : This function parses the script files and invokes the corresponding 
 *                 command processor (fw or drv)
 *
 * IN PARAMS     : pParTok  - The input string that needs to be processed for scripts
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or SYNC_STATUS_FAILURE
 *
 ****************************************************************************************/
SYNC_STATUS_E   Sync_Timing_DriverApp_ProcessScript(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    SYNC_STATUS_E                       syncStatus          = SYNC_STATUS_SUCCESS;
    char                                *pPar               = NULL;
    FILE                                *fpInput            = NULL;
    char line[SYNC_TIMING_DRIVER_APP_MAX_LINE_LENGTH]       = {0};
    uint32_t                            lineCount           = 0;
    uint32_t                            cmdLen              = 0;
    SYNC_TIMING_OSAL_STR_TOKENIZER_T    cmdTok              = {0};
    char                                *pGenPar            = NULL;
    char                                parDelim[]          = " ";
    uint32_t                            uTimeToSleep        = 0;
    uint32_t                            base                = 10;

    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        Sync_Timing_OSAL_Wrapper_Memset(&gInputScriptfile[0], 0, 
                                        SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ/2);

        Sync_Timing_OSAL_Wrapper_Memcpy(&gInputScriptfile[0], pPar,
                                        Sync_Timing_OSAL_Wrapper_Strlen(pPar));
        printf("Received gInputScriptfile = %s\n", gInputScriptfile);
    }

    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        Sync_Timing_OSAL_Wrapper_Memset(&gOutputfile[0], 0, 
                                        SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ/2);

        Sync_Timing_OSAL_Wrapper_Memcpy(&gOutputfile[0], pPar,
                                        Sync_Timing_OSAL_Wrapper_Strlen(pPar));
        printf("Received gOutputfile = %s\n", gOutputfile);
        bLogScriptCmdOutputToFile = SYNC_TIMING_TRUE;
    }
    else
    {
        bLogScriptCmdOutputToFile = SYNC_TIMING_FALSE;
    }

    fpInput = fopen(gInputScriptfile, "r");
    if (!fpInput)
    {
        printf("Invalid input script file name - unable to open\n");
        bLogScriptCmdOutputToFile = SYNC_TIMING_FALSE;
        return syncStatus;
    }

    if (bLogScriptCmdOutputToFile)
    {
        fpScriptCmdOutput = fopen(gOutputfile, "w");
        if (!fpScriptCmdOutput)
        {
            printf("Invalid output file name - unable to open\n");
            fclose(fpInput);
            bLogScriptCmdOutputToFile = SYNC_TIMING_FALSE;
            return syncStatus;
        }
    }
    
    /* Get each line until there are none left */
    while (fgets(line, SYNC_TIMING_DRIVER_APP_MAX_LINE_LENGTH, fpInput))
    {
        /* Print each line */
        ++lineCount;
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "************************************************************************\n");
            //printf("line[%06d]: %s", lineCount, line);
        }
        
        cmdLen = Sync_Timing_OSAL_Wrapper_Strlen(line);
        
        if (cmdLen > 0 && (line[cmdLen-1] == '\n' || line[cmdLen-1] == '\r'))
        {
            line[cmdLen-1] = '\0';
        }
        
        if (cmdLen > 0 && (line[cmdLen-2] == '\n' || line[cmdLen-2] == '\r'))
        {
            line[cmdLen-2] = '\0';
        }
        
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command: %s\n", line);
        }
        Sync_Timing_OSAL_Wrapper_Memset(&scriptCmd[0], 0, SYNC_TIMING_DRIVER_APP_MAX_CMD_SIZE);

        if (line[0] == '#')
        {
            printf("Commented line - Nothing to do - just skip\n");
            fprintf(fpScriptCmdOutput, "Commented line - Nothing to do - just skip\n");
        }
        else if (Sync_Timing_OSAL_Wrapper_Memcmp(&line[0], "driver", 6) == 0)
        {
            printf("Driver API command.\n");

            Sync_Timing_OSAL_Wrapper_Memcpy(&scriptCmd[0], &line[6],
                                            Sync_Timing_OSAL_Wrapper_Strlen(line) - 6);
            //scriptCmd[Sync_Timing_OSAL_Wrapper_Strlen(scriptCmd)] = '\0';

            //printf("scriptCmd = %s \n", scriptCmd);
            //printf("cmdLen = %u, new scriptcmd len = %u\n", cmdLen, 
            //                            Sync_Timing_OSAL_Wrapper_Strlen(scriptCmd));

            syncStatus = Sync_Timing_DriverApp_ProcessDriverAPICmd(&scriptCmd[0]);
        }   
        else if (Sync_Timing_OSAL_Wrapper_Memcmp(&line[0], "fw", 2) == 0)
        {
            printf("FW API command.\n");
            
            Sync_Timing_OSAL_Wrapper_Memcpy(&scriptCmd[0], &line[2],
                                            Sync_Timing_OSAL_Wrapper_Strlen(line) - 2);
            
            syncStatus = Sync_Timing_DriverApp_ProcessFWAPICmd(&scriptCmd[0]);
        }
        else if (Sync_Timing_OSAL_Wrapper_Memcmp(&line[0], "sleep", 5) == 0)
        {
            printf("General Application command.\n");
            
            Sync_Timing_OSAL_Wrapper_Memcpy(&scriptCmd[0], &line[5],
                                            Sync_Timing_OSAL_Wrapper_Strlen(line) - 5);
            
            Sync_Timing_OSAL_Wrapper_StrTokenizer_Init(&cmdTok, &scriptCmd[0], parDelim, 
                                                       SYNC_TIMING_TRUE);
    
            if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(&cmdTok))
            {
                pGenPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(&cmdTok);
                
                pGenPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pGenPar);

                if (Sync_Timing_DriverApp_Is_Number(pGenPar, &base))
                {
                    uTimeToSleep = Sync_Timing_OSAL_Wrapper_Atoi(pGenPar);
                    printf("Sleep for %u ms\n", uTimeToSleep);
                    if (bLogScriptCmdOutputToFile)
                    {
                        fprintf(fpScriptCmdOutput, "Sleep for %u ms\n", uTimeToSleep);
                    }                    
                    Sync_Timing_OSAL_Wrapper_SleepMS(uTimeToSleep);
                }
                else
                {
                    printf("Invalid argument %s supplied for sleep option\n", pGenPar);
                    if (bLogScriptCmdOutputToFile)
                    {
                        fprintf(fpScriptCmdOutput, "Invalid argument %s supplied for sleep option\n", 
                                pGenPar);
                    } 
                }
            }
            else
            {
                printf("Command Error - sleep needs addl param - milliseconds to sleep.\n");
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, 
                            "Command Error - sleep needs addl param - milliseconds to sleep.\n");
                }
            }
        }
        else
        {
            printf("Unrecognized command format in the file - Ignoring.\n");
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Unrecognized command format in the file - Ignoring.\n");
            }
        }

        /* Add trailing separator and newlines */
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "************************************************************************\n\n");
        }
    }
    
    /* Close file */
    if (fclose(fpInput))
    {
        printf("Unable to close input file pointer.\n");
    }
    if (bLogScriptCmdOutputToFile)
    {
        if (fclose(fpScriptCmdOutput))
        {
            printf("Unable to close cmd output file pointer.\n");
        }
    }
    bLogScriptCmdOutputToFile = SYNC_TIMING_FALSE;
    
    
    return syncStatus;
}


