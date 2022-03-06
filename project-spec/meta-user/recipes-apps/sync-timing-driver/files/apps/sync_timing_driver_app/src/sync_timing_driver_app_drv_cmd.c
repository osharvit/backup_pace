/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_driver_app_drv_cmd.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 12/14/2020
 *
 * DESCRIPTION        : Sync Timing Driver API Usage Example
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
    Macros
*****************************************************************************************/

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

/*****************************************************************************************
    Global Variable Declarations
*****************************************************************************************/

extern uint8_t              gTimingDevId;
extern char                 syncStatusStr[SYNC_STATUS_MAX][64];
extern void                 *gClientId;
extern SYNC_TIMING_BOOL_E   bLogScriptCmdOutputToFile;
extern FILE                 *fpScriptCmdOutput;

static uint8_t      gSyncPllIdToDriverMap[3] = {SYNC_TIMING_CLOCKADJ_STATUS_PLL_R,
                                                SYNC_TIMING_CLOCKADJ_STATUS_PLL_A,
                                                SYNC_TIMING_CLOCKADJ_STATUS_PLL_B
                                               };

static char gSyncDriverAppPllInputStr[8][64] = {"SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN0",
                                                "",
                                                "SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN1",
                                                "",
                                                "SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN2",
                                                "SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN2B",
                                                "SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN3",
                                                "SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN3B"
                                               };

static char gLogLevelText[SYNC_TIMING_LOG_LEVEL_MAX][16] = 
                                               {
                                                "Critical",
                                                "Error",
                                                "Warning",
                                                "Info1",
                                                "Info2",
                                                "Info3",
                                                "Debug",
                                                "Disabled"
                                               };

static char         gBootfileList[SYNC_TIMING_MAX_DEVICE_DOWNLOAD_BOOTFILES][SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ];
static uint32_t     uNumBootfiles            = 0;

/*****************************************************************************************
    Function Prototypes
 ****************************************************************************************/

static void Sync_Timing_DriverApp_PrintHelpDriverMenu();

static void Sync_Timing_DriverApp_ProcessLogLevel(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_GetVersionInfo();

static void Sync_Timing_DriverApp_GetChipsetMode();

static void Sync_Timing_DriverApp_Reset(SYNC_TIMING_BOOL_E bStayInBlMode);

static void Sync_Timing_DriverApp_ReadDirect (SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_WriteDirect (SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_RawApiCmd (SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_GetPllStatus(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_GetInputStatus(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_SetPllForceHO(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_SetPllInput(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_DownloadImage(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);


/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_PrintHelpDriverMenu
 *
 * DESCRIPTION   : This function is used to print driver commands help menu
 *
 * IN PARAMS     : None 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_PrintHelpDriverMenu()
{
    printf("************************************************************************************\n");
    printf("Driver Commands Help Menu\n");
    printf("************************************************************************************\n");
    printf("help                        - Print this help menu.\n");
    printf("loglevel [logfilter] [level]- Get current loglevel or set new loglevel \n");
    printf("                              Set operation if newlevel is specified otherwise get operation\n");
    printf("                              [logfilter] = 0 (stdout) or 1 (file filter)\n");
    printf("                              <level> = 7 (disable all logs) or 0 (critical) or 1 (error) or 2 (warning) or 3-5 (info) or 6 (debug) \n");
    printf("version                     - Get and print all version info.\n");
    printf("chip_mode                   - Get current mode (appln or bootloader) of chipset\n");
    printf("reset                       - Reset chipset - will load image from NVM if present\n");
    printf("reset_bl                    - Reset chipset and stay in bootloader mode\n");
    printf("reg_read <addr> <num_bytes> - Read num_bytes from reg_addr specified\n");
    printf("reg_write <addr> <d0>..<dn> - Write specified data bytes from reg_addr specified\n");
    printf("raw_api  <c0>..<cn> <rlen>  - Send RAW API command to firmware \n");
    printf("                            - <c0>..<cn> - command data including command; \n");
    printf("                            - <rlen> - cmd resp length excluding cmd status byte\n");
    printf("pll_status  <pllId>         - Get Pll Status \n");
    printf("                              (0 - PLL-R, 1 - PLL-A, 2 - PLL-B)\n");
    printf("input_status <inputId>      - Get Input Status \n");
    printf("                              (0 - IN0, 2 - IN1, 4 - IN2, 5 - IN2B, 6 - IN3, 7 - IN3B)\n");
    printf("pll_ho  <pllId>             - Put PLL in force HO \n");
    printf("                              (0 - PLL-R, 1 - PLL-A, 2 - PLL-B)\n");
    printf("pll_input <pllId> <inputId> - Set PLL input \n");
    printf("                              (0 - PLL-R, 1 - PLL-A, 2 - PLL-B)\n");
    printf("                              (0 - IN0, 2 - IN1, 4 - IN2, 5 - IN2B, 6 - IN3, 7 - IN3B)\n");
    printf("dload_image                 - Download Firmware & FPlan image - file query follows\n");
    printf("main                        - Exit Driver menu and go back to main menu.\n");
    printf("************************************************************************************\n");
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_ProcessLogLevel
 *
 * DESCRIPTION   : This function implements the driver command for getting or setting log level
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_ProcessLogLevel(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    char                                    *pPar               = NULL;
    uint32_t                                uLogLevel           = 0;
    uint32_t                                uLogFilter          = 0;
    uint32_t                                base                = 10;
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;

    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            uLogFilter = Sync_Timing_OSAL_Wrapper_Atoi(pPar);

            if (uLogFilter > 1)
            { 
                printf("Invalid Log Filter %u supplied.\n", uLogFilter);
                Sync_Timing_DriverApp_PrintHelpDriverMenu();
                return;
            }
        }
        else
        {
            printf("Log Filter not specified correctly.\n");
            Sync_Timing_DriverApp_PrintHelpDriverMenu();
            return;
        }
    }
    else
    {
        syncStatus = Sync_Timing_LOG_GetTraceLevel(gClientId, SYNC_TIMING_LOG_CFG_FILTER_STDOUT, 
                                                   &uLogLevel);
        if (syncStatus == SYNC_STATUS_SUCCESS)
        {
            printf("\n**************************************************************************\n");
            printf("----- LOG LEVEL for LOG_CFG_FILTER_STDOUT OBTAINED is %s ----- \n", 
                   gLogLevelText[uLogLevel]);
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "----- LOG LEVEL for LOG_CFG_FILTER_STDOUT OBTAINED is %s ----- \n", 
                        gLogLevelText[uLogLevel]);
            }
            
            printf("\n**************************************************************************\n");
        }
        else
        {
            printf("Sync_Timing_LOG_GetTraceLevel failed syncStatus = %s\n", 
                    syncStatusStr[syncStatus]);
            Sync_Timing_DriverApp_PrintHelpDriverMenu();
            return;
        }

        syncStatus = Sync_Timing_LOG_GetTraceLevel(gClientId, SYNC_TIMING_LOG_CFG_FILTER_FILE, 
                                                   &uLogLevel);
        if (syncStatus == SYNC_STATUS_SUCCESS)
        {
            printf("\n**************************************************************************\n");
            printf("----- LOG LEVEL for LOG_CFG_FILTER_FILE OBTAINED is %s ----- \n", 
                   gLogLevelText[uLogLevel]);
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, 
                        "----- LOG LEVEL for LOG_CFG_FILTER_FILE OBTAINED is %s ----- \n", 
                        gLogLevelText[uLogLevel]);
            }
            
            printf("\n**************************************************************************\n");
        }
        else
        {
            printf("Sync_Timing_LOG_GetTraceLevel failed syncStatus = %s\n", 
                    syncStatusStr[syncStatus]);
            Sync_Timing_DriverApp_PrintHelpDriverMenu();
            return;
        }
        return;
    }            

    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            uLogLevel = Sync_Timing_OSAL_Wrapper_Atoi(pPar);

            if (uLogLevel > 7)
            {
                printf("Invalid Log Level %u supplied.\n", uLogLevel);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Invalid Log Level %u supplied.\n", uLogLevel);
                }
                Sync_Timing_DriverApp_PrintHelpDriverMenu();
                return;
            }
        }
        else
        {
            printf("Log Level not specified correctly.\n");
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Log Level not specified correctly.\n");
            }
            Sync_Timing_DriverApp_PrintHelpDriverMenu();
            return;
        }
    }
    else
    {
        printf("Log Level not specified.\n");
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Log Level not specified.\n");
        }
        Sync_Timing_DriverApp_PrintHelpDriverMenu();
        return;
    }      
    
    syncStatus = Sync_Timing_LOG_SetTraceLevel(gClientId, uLogFilter, uLogLevel);
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf("\n**************************************************************************\n");
        printf("----- LOG LEVEL (%s) for LOG FILTER (%u) SET SUCCESSFULLY ----- \n", 
                gLogLevelText[uLogLevel], uLogFilter);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, 
                    "----- LOG LEVEL (%s) for LOG FILTER (%u) SET SUCCESSFULLY ----- \n", 
                    gLogLevelText[uLogLevel], uLogFilter);
        }
        printf("\n**************************************************************************\n");
    }
    else
    {
        printf("Sync_Timing_LOG_SetTraceLevel failed syncStatus = %s\n", syncStatusStr[syncStatus]);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, 
                    "Sync_Timing_LOG_SetTraceLevel failed syncStatus = %s\n", 
                    syncStatusStr[syncStatus]);
        }
        Sync_Timing_DriverApp_PrintHelpDriverMenu();
        return;
    }
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_GetVersionInfo
 *
 * DESCRIPTION   : This function implements the driver command for obtaining version information
 *
 * IN PARAMS     : None 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_GetVersionInfo()
{
    SYNC_TIMING_DEVICE_VERSION_T    deviceVersion  = {0};
    SYNC_STATUS_E                   syncStatus     = SYNC_STATUS_SUCCESS;

    syncStatus = Sync_Timing_API_Device_GetVersionInfo(gTimingDevId, &deviceVersion);
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf( "\n**************************************************************************\n");
        printf( "Chipset Revision = %s\n", deviceVersion.chipsetRevision);
        printf( "Bootloader Version = %s\n", deviceVersion.blVersion);
        printf( "Firmware Version = %s\n", deviceVersion.fwVersion);
        printf( "CBPro Version = %s\n", deviceVersion.cbproVersion);
        printf( "Fplan Version = %s\n", deviceVersion.fplanVersion);
        printf( "Fplan Design ID = %s\n", deviceVersion.fplanDesignId);
        printf( "Driver Version = %s\n", deviceVersion.driverVersion);
        printf( "Driver Build Info = %s\n", deviceVersion.driverBuildInfo);
        printf( "\n**************************************************************************\n");

        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Chipset Revision = %s\n", deviceVersion.chipsetRevision);
            fprintf(fpScriptCmdOutput, "Bootloader Version = %s\n", deviceVersion.blVersion);
            fprintf(fpScriptCmdOutput, "Firmware Version = %s\n", deviceVersion.fwVersion);
            fprintf(fpScriptCmdOutput, "CBPro Version = %s\n", deviceVersion.cbproVersion);
            fprintf(fpScriptCmdOutput, "Fplan Version = %s\n", deviceVersion.fplanVersion);
            fprintf(fpScriptCmdOutput, "Fplan Design ID = %s\n", deviceVersion.fplanDesignId);
            fprintf(fpScriptCmdOutput, "Driver Version = %s\n", deviceVersion.driverVersion);
            fprintf(fpScriptCmdOutput, "Driver Build Info = %s\n", deviceVersion.driverBuildInfo);
        }
    }
    else
    {
        printf("Sync_Timing_API_Device_GetVersionInfo Failed syncStatus = %s\n", 
                syncStatusStr[syncStatus]);

        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Sync_Timing_API_Device_GetVersionInfo Failed syncStatus = %s\n", 
                        syncStatusStr[syncStatus]);
        }
    }
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_PrinSync_Timing_DriverApp_GetChipsetModetHelpDriverMenu
 *
 * DESCRIPTION   : This fn implements the driver command for obtaining the current chipset mode
 *
 * IN PARAMS     : None 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_GetChipsetMode()
{
    SYNC_STATUS_E                   syncStatus      = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_DEVICE_MODE_E       currDeviceMode  = 0;

    printf("\n**************************************************************************\n");
    syncStatus = Sync_Timing_API_Debug_GetCurrentMode(gTimingDevId, &currDeviceMode);
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf("----- CURRENT DEVICE MODE = %s ----------\n", 
               (currDeviceMode == SYNC_TIMING_DEVICE_MODE_APPLN)? "APPLN":"BOOTLOADER");

        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "----- CURRENT DEVICE MODE = %s ----------\n", 
                    (currDeviceMode == SYNC_TIMING_DEVICE_MODE_APPLN)? "APPLN":"BOOTLOADER");
        }        
    }
    else
    {
        printf("Sync_Timing_API_Debug_GetCurrentMode FAILED syncStatus = %s", 
                syncStatusStr[syncStatus]);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Sync_Timing_API_Debug_GetCurrentMode FAILED syncStatus = %s", 
                                       syncStatusStr[syncStatus]);
        }         
    }
    printf( "\n**************************************************************************\n");
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_Reset
 *
 * DESCRIPTION   : This function implements the driver command for resetting the chipset
 *
 * IN PARAMS     : bStayInBlMode - Indicates where to stay in BL mode or not after reset
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_Reset(SYNC_TIMING_BOOL_E bStayInBlMode)
{
    SYNC_STATUS_E                       syncStatus      = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_DEVICE_RESET_TYPE_E     gResetType      = SYNC_TIMING_DEVICE_RESET_TOGGLE;

    if (bStayInBlMode)
        gResetType = SYNC_TIMING_DEVICE_RESET_BOOTLOADER_MODE;

    printf("\n**************************************************************************\n");
    syncStatus = Sync_Timing_API_Device_Reset(gTimingDevId, gResetType);
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf("----- RESET COMPLETE ----- \n");
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "----- RESET COMPLETE ----- \n");
        }
    }
    else
    {
        printf("----- RESET FAILED ----- syncStatus = %s\n", syncStatusStr[syncStatus]);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "----- RESET FAILED ----- syncStatus = %s\n", 
                    syncStatusStr[syncStatus]);
        }
    }
    printf("\n**************************************************************************\n");
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_ReadDirect
 *
 * DESCRIPTION   : This fn implements the driver command for reading registers back from chipset
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_ReadDirect (SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    SYNC_STATUS_E   syncStatus      = SYNC_STATUS_SUCCESS;
    uint8_t         rdData[32]      = {0},
                    idx             = 0;
    uint16_t        addr            = 0;
    uint32_t        len             = 0;
    uint32_t        temp_val        = 0;
    char            *pPar           = NULL;
    uint32_t        base            = 10;

    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        temp_val = (uint32_t)strtol(pPar, NULL, 16);
        if (temp_val > 0xFFFF)
        {
            printf("Error: Invalid address 0x%X specified (Use 16-bit address)\n", temp_val);
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, 
                        "Error: Invalid address 0x%X specified (Use 16-bit address)\n", temp_val);
            }
            return;
        }
        addr = (uint16_t)temp_val;
    }
    else
    {
        printf("Error: Register Address not specified\n");
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Error: Register Address not specified\n");
        }        
        return;
    }

    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            len = (uint16_t)strtol(pPar, NULL, base);
        }
        else
        {
            printf("Invalid length argument specified\n");
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Invalid length argument specified\n");
            }           
            
            return;
        }

    }
    else
    {
        printf("Error: Length not specified\n");
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Error: Length not specified\n");
        }   
        
        return;
    }

    syncStatus = Sync_Timing_API_Mem_ReadDirect(gTimingDevId, addr, len, &rdData[0]);
  
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf("Data read from rdMemAddr 0x%04X = ", addr);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Data read from rdMemAddr 0x%04X = ", addr);
        }   
        for (idx = 0; idx < len; idx ++)
        {
            printf ("0x%02X ", rdData[idx]);
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "0x%02X ", rdData[idx]);
            }   
        }
        printf("\n");
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "\n");
        }   

    }
    else
    {
        printf("Sync_Timing_API_Mem_ReadDirect failed; SyncStatus = %s\n", 
                syncStatusStr[syncStatus]);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Sync_Timing_API_Mem_ReadDirect failed; SyncStatus = %s\n", 
                                        syncStatusStr[syncStatus]);
        }   
    }
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_WriteDirect
 *
 * DESCRIPTION   : This function implements the driver command for writing registers in the chipset
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_WriteDirect (SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    uint16_t        addr            = 0;
    uint32_t        len             = 0;
    uint32_t        temp_val        = 0;
    char            *pPar           = NULL;
    SYNC_STATUS_E   syncStatus      = SYNC_STATUS_SUCCESS;
    uint8_t         wrData[32]      = {0};

    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        temp_val = (uint32_t)strtol(pPar, NULL, 16);
        if (temp_val > 0xFFFF)
        {
            printf("Error: Invalid address 0x%X specified (Use 16-bit address)\n", temp_val);
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, 
                        "Error: Invalid address 0x%X specified (Use 16-bit address)\n", temp_val);
            }             
            return;
        }
        addr = (uint16_t)temp_val;
    }
    else
    {
        printf("Error: Register Address not specified\n");
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Error: Register Address not specified\n");
        }         
        return;
    }

    while(Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok) && len < 32)
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);
                  
        wrData[len++] = (uint8_t)strtol(pPar, NULL, 16);
    }

    if (len == 0)
    {
        printf("Error: Need atleast one data byte to write\n");
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Error: Need atleast one data byte to write\n");
        } 

        return;
    }
    
    syncStatus = Sync_Timing_API_Mem_WriteDirect(gTimingDevId, addr, len, &wrData[0], 0, 0);
    
    //printf("Sync_Timing_API_Mem_WriteDirect returned SyncStatus = %d\n", syncStatus);
    if (syncStatus != SYNC_STATUS_SUCCESS)
    {
        printf("Sync_Timing_API_Mem_WriteDirect failed; SyncStatus = %s\n", 
                syncStatusStr[syncStatus]);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Sync_Timing_API_Mem_WriteDirect failed; SyncStatus = %s\n", 
                syncStatusStr[syncStatus]);
        } 

    }
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_RawApiCmd
 *
 * DESCRIPTION   : This function implements the driver command for send Raw API commands to FW
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_RawApiCmd(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    uint16_t        uIdx            = 0;
    char            *pPar           = NULL;
    SYNC_STATUS_E   syncStatus      = SYNC_STATUS_SUCCESS;
    uint32_t        inputLen        = 0,
                    cmdLen          = 0;
    uint8_t         cmdInfo[32]     = {0};
    uint32_t        cmdRespLen      = 0;
    uint8_t         cmdRespData[128]= {0};
    uint8_t         cmdRespStatus   = 0;


    while(Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok) && inputLen < 32)
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);
                  
        cmdInfo[inputLen++] = (uint8_t)strtol(pPar, NULL, 16);
    }

    if (inputLen < 2)
    {
        printf("Error: Need atleast 2 bytes of input - "
               "1st command data byte to write and 2nd cmd response length \n");
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Error: Need atleast 2 bytes of input - "
                            "1st command data byte to write and 2nd cmd response length \n");
        } 
        return;
    }

    /* Subtract to account for the command response length which is the last byte in the input
       provided by the user */
    cmdLen = inputLen - 1;

    printf("Cmd inputLen = %u\n", inputLen);
    if (bLogScriptCmdOutputToFile)
    {
        fprintf(fpScriptCmdOutput, "Cmd inputLen = %u\n", inputLen);
    } 

    printf("Command Info: ");
    if (bLogScriptCmdOutputToFile)
    {
        fprintf(fpScriptCmdOutput, "Command Info: ");
    } 
    for (uIdx = 0; uIdx < cmdLen; uIdx++)
    {
        printf("0x%02x ", cmdInfo[uIdx]);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "0x%02x ", cmdInfo[uIdx]);
        } 
    }
    printf("\n");
    if (bLogScriptCmdOutputToFile)
    {
        fprintf(fpScriptCmdOutput, "\n");
    } 

    cmdRespLen = cmdInfo[inputLen-1];
    
    printf("cmdRespLen = %u\n", cmdRespLen);
    if (bLogScriptCmdOutputToFile)
    {
        fprintf(fpScriptCmdOutput, "cmdRespLen = %u\n", cmdRespLen);
    } 
        
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, &cmdInfo[0], cmdLen, 
                                                 cmdRespLen, &cmdRespStatus,
                                                 &cmdRespData[0], 0, 0);
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf("Command Response: (cmdRespStatus = 0x%02x) \n", cmdRespStatus);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command Response: (cmdRespStatus = 0x%02x) \n", 
                    cmdRespStatus);
        } 

        for (uIdx = 0; uIdx < cmdRespLen; uIdx++)
        {
            printf("0x%02x ", cmdRespData[uIdx]);
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "0x%02x ", cmdRespData[uIdx]);
            } 

        }
        printf("\n");
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "\n");
        }         
    }
    else
    {
        printf("Command failed - cmdRespStatus = 0x%02x \n", cmdRespStatus);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command failed - cmdRespStatus = 0x%02x \n", 
                    cmdRespStatus);
        }         
    }

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_GetPllStatus
 *
 * DESCRIPTION   : This function implements the driver command for reading PLL status
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_GetPllStatus(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    char                                    *pPar               = NULL;
    uint8_t                                 uPllId              = 0;
    uint8_t                                 uDriverPllId        = 0;
    uint32_t                                base                = 10;
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CLOCKADJ_STATUS_E           clkAdjStatus        = 0;
    SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_E pllInput            = 0;

        
    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            uPllId = Sync_Timing_OSAL_Wrapper_Atoi(pPar);

            if (uPllId > 2)
            {
                printf("Invalid PLL ID %u supplied for pll_status option\n", uPllId);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Invalid PLL ID %u supplied for pll_status option\n", 
                            uPllId);
                } 
                Sync_Timing_DriverApp_PrintHelpDriverMenu();
                return;
            }
                
            uDriverPllId = gSyncPllIdToDriverMap[uPllId];
            
            syncStatus = Sync_Timing_API_ClockAdj_GetCurrentStatus(gTimingDevId, uDriverPllId, 
                                                                   &clkAdjStatus);
            if (syncStatus == SYNC_STATUS_SUCCESS)
            {
                syncStatus = Sync_Timing_API_ClockAdj_GetPLLInput(gTimingDevId, uDriverPllId, 
                                                                   &pllInput);
                if (syncStatus == SYNC_STATUS_SUCCESS)
                {
                    printf("\n**************************************************************************\n");
                    printf("----- PLL STATUS (0x%02X) OBTAINED SUCCESSFULLY for PLL ID (%u)----- \n", 
                           clkAdjStatus, uPllId);
                    if (bLogScriptCmdOutputToFile)
                    {
                        fprintf(fpScriptCmdOutput, 
                                "----- PLL STATUS (0x%02X) OBTAINED SUCCESSFULLY for PLL ID (%u)----- \n", 
                                clkAdjStatus, uPllId);
                    } 

                    if (clkAdjStatus & SYNC_TIMING_CLOCKADJ_STATUS_PLL_LOCKED)
                    {
                        printf("---------- STATUS_PLL_LOCKED\n");
                        if (bLogScriptCmdOutputToFile)
                        {
                            fprintf(fpScriptCmdOutput, "---------- STATUS_PLL_LOCKED\n");
                        }                         
                    }
                    if (clkAdjStatus & SYNC_TIMING_CLOCKADJ_STATUS_PLL_NOT_LOCKED)
                    {
                        printf("---------- STATUS_PLL_NOT_LOCKED\n");
                        if (bLogScriptCmdOutputToFile)
                        {
                            fprintf(fpScriptCmdOutput, "---------- STATUS_PLL_NOT_LOCKED\n");
                        }   
                    }
                    if (clkAdjStatus & SYNC_TIMING_CLOCKADJ_STATUS_PLL_HOLDOVER)
                    {
                        printf("---------- STATUS_PLL_HOLDOVER\n");
                        if (bLogScriptCmdOutputToFile)
                        {
                            fprintf(fpScriptCmdOutput, "---------- STATUS_PLL_HOLDOVER\n");
                        }
                    }
                    if (clkAdjStatus & SYNC_TIMING_CLOCKADJ_STATUS_PLL_FREQ_ERR)
                    {
                        printf("---------- STATUS_PLL_FREQ_ERR\n");
                        if (bLogScriptCmdOutputToFile)
                        {
                            fprintf(fpScriptCmdOutput, "---------- STATUS_PLL_FREQ_ERR\n");
                        }
                    }
                    if (clkAdjStatus & SYNC_TIMING_CLOCKADJ_STATUS_PLL_PHASE_ERR)
                    {
                        printf("---------- STATUS_PLL_PHASE_ERR\n");
                        if (bLogScriptCmdOutputToFile)
                        {
                            fprintf(fpScriptCmdOutput, "---------- STATUS_PLL_PHASE_ERR\n");
                        }
                    }
                    
                    if (pllInput == SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_RESERVED)
                    {
                        printf("---------- Current PLL Input = REF_XO_INPUT - RESERVED\n");
                        if (bLogScriptCmdOutputToFile)
                        {
                            fprintf(fpScriptCmdOutput, 
                                    "---------- Current PLL Input = REF_XO_INPUT - RESERVED\n");
                        }
                    } 
                    else if (pllInput != SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_NONE)
                    {
                        printf("---------- Current PLL Input = %s\n", 
                                gSyncDriverAppPllInputStr[pllInput]);
                        if (bLogScriptCmdOutputToFile)
                        {
                            fprintf(fpScriptCmdOutput, "---------- Current PLL Input = %s\n", 
                                    gSyncDriverAppPllInputStr[pllInput]);
                        }
                    }
                    else
                    {
                        printf("---------- Current PLL Input = <NOT_APPLICABLE>\n");
                        if (bLogScriptCmdOutputToFile)
                        {
                            fprintf(fpScriptCmdOutput, 
                                    "---------- Current PLL Input = <NOT_APPLICABLE>\n");
                        }
                    }

                    printf("\n**************************************************************************\n");
                }
            }
            else
            {
                printf("----- FAILED TO RETRIEVE PLL STATUS ----- "
                       "syncStatus = %s\n", syncStatusStr[syncStatus]);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "----- FAILED TO RETRIEVE PLL STATUS ----- "
                            "syncStatus = %s\n", syncStatusStr[syncStatus]);
                }
            }               
        }
        else
        {
            printf("Invalid argument %s supplied for pll_status option\n", pPar);
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Invalid argument %s supplied for pll_status option\n", 
                        pPar);
            }            
            Sync_Timing_DriverApp_PrintHelpDriverMenu();
        }
    }
    else
    {
        printf("PLL ID not specified.\n");
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PLL ID not specified.\n");
        }
        Sync_Timing_DriverApp_PrintHelpDriverMenu();
    }
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_GetInputStatus
 *
 * DESCRIPTION   : This function implements the driver command for reading Input Status
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_GetInputStatus(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    char                                    *pPar               = NULL;
    uint8_t                                 uInputId            = 0;
    uint8_t                                 uDriverInputId      = 0;
    uint32_t                                base                = 10;
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_CLOCKADJ_STATUS_E           clkAdjStatus        = 0;
        
    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            uInputId = Sync_Timing_OSAL_Wrapper_Atoi(pPar);

            if (uInputId > 7)
            {
                printf("Invalid Input ID %u supplied.\n", uInputId);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Invalid Input ID %u supplied.\n", uInputId);
                }
                Sync_Timing_DriverApp_PrintHelpDriverMenu();
                return;
            }
                
            uDriverInputId = uInputId + SYNC_TIMING_CLOCKADJ_STATUS_IN0;      
            
            syncStatus = Sync_Timing_API_ClockAdj_GetCurrentStatus(gTimingDevId, uDriverInputId, 
                                                                   &clkAdjStatus );
            if (syncStatus == SYNC_STATUS_SUCCESS)
            {
                printf("\n**************************************************************************\n");
                printf("----- INPUT STATUS (0x%02X) OBTAINED SUCCESSFULLY for INPUT ID (%u)----- \n", 
                        clkAdjStatus, uInputId);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, 
                        "----- INPUT STATUS (0x%02X) OBTAINED SUCCESSFULLY for INPUT ID (%u)----- \n", 
                        clkAdjStatus, uInputId);
                }
                if (clkAdjStatus & SYNC_TIMING_CLOCKADJ_STATUS_INPUT_HAS_SIGNAL)
                {
                    printf("---------- STATUS_INPUT_HAS_SIGNAL\n");
                    if (bLogScriptCmdOutputToFile)
                    {
                        fprintf(fpScriptCmdOutput, "---------- STATUS_INPUT_HAS_SIGNAL\n");
                    }
                }
                if (clkAdjStatus & SYNC_TIMING_CLOCKADJ_STATUS_INPUT_NO_SIGNAL)
                {
                    printf("---------- STATUS_INPUT_NO_SIGNAL\n");
                    if (bLogScriptCmdOutputToFile)
                    {
                        fprintf(fpScriptCmdOutput, "---------- STATUS_INPUT_NO_SIGNAL\n");
                    }
                }
                if (clkAdjStatus & SYNC_TIMING_CLOCKADJ_STATUS_INPUT_OOF)
                {
                    printf("---------- STATUS_INPUT_OOF\n");
                    if (bLogScriptCmdOutputToFile)
                    {
                        fprintf(fpScriptCmdOutput, "---------- STATUS_INPUT_OOF\n");
                    }
                }
                if (clkAdjStatus & SYNC_TIMING_CLOCKADJ_STATUS_INPUT_INVALID)
                {
                    printf("---------- STATUS_INPUT_INVALID\n");
                    if (bLogScriptCmdOutputToFile)
                    {
                        fprintf(fpScriptCmdOutput, "---------- STATUS_INPUT_INVALID\n");
                    }
                }
                printf("\n**************************************************************************\n");                
            }
            else
            {
                printf("----- FAILED TO RETRIEVE INPUT STATUS ----- "
                       "syncStatus = %s\n", syncStatusStr[syncStatus]);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "----- FAILED TO RETRIEVE INPUT STATUS ----- "
                       "syncStatus = %s\n", syncStatusStr[syncStatus]);
                }
            }             
        }
        else
        {
            printf("Invalid argument %s supplied for input_status option\n", pPar);
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Invalid argument %s supplied for input_status option\n", 
                        pPar);
            }
            Sync_Timing_DriverApp_PrintHelpDriverMenu();
        }
    }
    else
    {
        printf("INPUT ID not specified.\n");
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "INPUT ID not specified.\n");
        }
        Sync_Timing_DriverApp_PrintHelpDriverMenu();
    }
    
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_SetPllForceHO
 *
 * DESCRIPTION   : This function implements the driver command for putting PLLs into Force Holdover
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_SetPllForceHO(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    char                                    *pPar               = NULL;
    uint8_t                                 uPllId              = 0;
    uint8_t                                 uDriverPllId        = 0;
    uint32_t                                base                = 10;
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
        
    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            uPllId = Sync_Timing_OSAL_Wrapper_Atoi(pPar);

            if (uPllId > 2)
            {
                printf("Invalid PLL ID %u supplied.\n", uPllId);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Invalid PLL ID %u supplied.\n", uPllId);
                }
                Sync_Timing_DriverApp_PrintHelpDriverMenu();
                return;
            }
                
            uDriverPllId = gSyncPllIdToDriverMap[uPllId];
        
            syncStatus = Sync_Timing_API_ClockAdj_SetPLLInput(gTimingDevId, uDriverPllId, 
                                                              0xFF);
            if (syncStatus == SYNC_STATUS_SUCCESS)
            {
                printf("\n**************************************************************************\n");
                printf("----- PLL (%u) PUT INTO FORCED HO SUCCESSFULLY ----- \n", uPllId);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, 
                            "----- PLL (%u) PUT INTO FORCED HO SUCCESSFULLY ----- \n", uPllId);
                }
                printf("\n**************************************************************************\n");
            }
            else
            {
                printf("Sync_Timing_API_ClockAdj_SetPLLInput Failed syncStatus = %s\n", 
                        syncStatusStr[syncStatus]);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, 
                        "Sync_Timing_API_ClockAdj_SetPLLInput Failed syncStatus = %s\n", 
                        syncStatusStr[syncStatus]);
                }
            }
        }
        else
        {
            printf("Invalid argument %s supplied for pll_status option\n", pPar);
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Invalid argument %s supplied for pll_status option\n", 
                        pPar);
            }
            Sync_Timing_DriverApp_PrintHelpDriverMenu();
        }
    }
    else
    {
        printf("PLL ID not specified.\n");
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PLL ID not specified.\n");
        }
        Sync_Timing_DriverApp_PrintHelpDriverMenu();
    }
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_SetPllInput
 *
 * DESCRIPTION   : This function implements the driver command for setting PLL Input
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_SetPllInput(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    char                                    *pPar               = NULL;
    uint8_t                                 uInputId            = 255;
    uint8_t                                 uDriverInputId      = 255;
    uint8_t                                 uPllId              = 255;
    uint8_t                                 uDriverPllId        = 255;
    uint32_t                                base                = 10;
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
        
    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            uPllId = Sync_Timing_OSAL_Wrapper_Atoi(pPar);

            if (uPllId > 2)
            {
                printf("Invalid PLL ID %u supplied.\n", uPllId);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Invalid PLL ID %u supplied.\n", uPllId);
                }
                Sync_Timing_DriverApp_PrintHelpDriverMenu();
                return;
            }
                
            uDriverPllId = gSyncPllIdToDriverMap[uPllId];

        }
        else
        {
            printf("PLL ID not specified correctly.\n");
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "PLL ID not specified.\n");
            }
            Sync_Timing_DriverApp_PrintHelpDriverMenu();
            return;
        }
    }
    else
    {
        printf("PLL ID not specified.\n");
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PLL ID not specified.\n");
        }
        Sync_Timing_DriverApp_PrintHelpDriverMenu();
        return;
    }            

    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            uInputId = Sync_Timing_OSAL_Wrapper_Atoi(pPar);

            if (uInputId > 7)
            {
                printf("Invalid Input ID %u supplied.\n", uInputId);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Invalid Input ID %u supplied.\n", uInputId);
                }
                Sync_Timing_DriverApp_PrintHelpDriverMenu();
                return;
            }
                
            uDriverInputId = uInputId + SYNC_TIMING_CLOCKADJ_PLL_INPUT_SELECT_IN0;      
        }
        else
        {
            printf("INPUT ID not specified correctly.\n");
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "INPUT ID not specified correctly.\n");
            }
            Sync_Timing_DriverApp_PrintHelpDriverMenu();
            return;
        }
    }
    else
    {
        printf("INPUT ID not specified.\n");
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "INPUT ID not specified.\n");
        }
        Sync_Timing_DriverApp_PrintHelpDriverMenu();
        return;
    }      
    
    syncStatus = Sync_Timing_API_ClockAdj_SetPLLInput(gTimingDevId, uDriverPllId, 
                                                      uDriverInputId);
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf("\n**************************************************************************\n");
        printf("----- PLL (%u) INPUT SET SUCCESSFULLY to %u ----- \n", uPllId, uInputId);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "----- PLL (%u) INPUT SET SUCCESSFULLY to %u ----- \n", 
                    uPllId, uInputId);
        }
        printf("\n**************************************************************************\n");
    }
    else
    {
        printf("Sync_Timing_API_ClockAdj_SetPLLInput Failed syncStatus = %s\n", 
                syncStatusStr[syncStatus]);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Sync_Timing_API_ClockAdj_SetPLLInput Failed syncStatus = %s\n", 
                syncStatusStr[syncStatus]);
        }
    }
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_DownloadImage
 *
 * DESCRIPTION   : This function implements the driver command for downloading images into 
 *                 chipset RAM
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_DownloadImage(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    uint8_t         uIdx                                                    = 0;
    char            updateFile[SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ]  = {0};
    char            cwd[SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ]         = {0};
    SYNC_TIMING_DEVICE_MODE_E       currDeviceMode      = 0;
    SYNC_TIMING_DEVICE_VERSION_T    deviceVersion       = {0};
    SYNC_STATUS_E                   syncStatus          = SYNC_STATUS_SUCCESS;
    char                            *pPar               = NULL;
    int32_t                         retVal              = 0;
    
    Sync_Timing_OSAL_Wrapper_Memset(&(gBootfileList[0][0]), 0, 
                            SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ);
    Sync_Timing_OSAL_Wrapper_Memset(&(gBootfileList[1][0]), 0, 
                            SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ);
    Sync_Timing_OSAL_Wrapper_Memset(&(gBootfileList[2][0]), 0, 
                            SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ);
    Sync_Timing_OSAL_Wrapper_Memset(&(gBootfileList[3][0]), 0, 
                            SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ);

    uNumBootfiles = 0;

    while(Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok) && uNumBootfiles < 4)
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        Sync_Timing_OSAL_Wrapper_Memcpy(&gBootfileList[uNumBootfiles][0], pPar,
                                        Sync_Timing_OSAL_Wrapper_Strlen(pPar));
        printf("Received gBootfileList[%u] = %s\n", uNumBootfiles, gBootfileList[uNumBootfiles]);
        uNumBootfiles++;
    }

    if (uNumBootfiles == 0)
    {
        printf("Enter number of bootfiles to download: ");
        retVal = scanf("%u", &uNumBootfiles);
        if (retVal);

        for (uIdx = 0; uIdx < uNumBootfiles; uIdx++)
        {
            printf("Enter name (absolute path or relative to current folder) of bootfile (%u) : ", uIdx);
            retVal = scanf("%s", gBootfileList[uIdx]);
            if (retVal);
        }
    }
    
    printf("Number of bootfiles to download = %u\n", uNumBootfiles);
    if (bLogScriptCmdOutputToFile)
    {
        fprintf(fpScriptCmdOutput, "Number of bootfiles to download = %u\n", uNumBootfiles);
    }

    for (uIdx = 0; uIdx < uNumBootfiles; uIdx++)
    {
        printf("Bootfile (%u) = %s\n", uIdx, gBootfileList[uIdx]);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Bootfile (%u) = %s\n", uIdx, gBootfileList[uIdx]);
        }

        Sync_Timing_OSAL_Wrapper_Memset(&updateFile[0], 0, 
                                        SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ);     
        
        if (Sync_Timing_OSAL_Wrapper_Getcwd(cwd, sizeof(cwd)) != NULL) 
        {
            ;//printf("Current working dir: %s\n", cwd);
        } 
        else 
        {
            perror("getcwd() error");
            return;
        }

        if (NULL == Sync_Timing_OSAL_Wrapper_Strstr(gBootfileList[uIdx], cwd))
        {
            if (gBootfileList[uIdx][0] != '/')
            {
                // Absolute path not provided - so determine absolute path
                Sync_Timing_OSAL_Wrapper_Strcat(updateFile, cwd);
                Sync_Timing_OSAL_Wrapper_Strcat(updateFile, "/");
                Sync_Timing_OSAL_Wrapper_Strcat(updateFile, gBootfileList[uIdx]);
            }
            else
            {
                Sync_Timing_OSAL_Wrapper_Strcat(updateFile, gBootfileList[uIdx]);
            }
        }
        else
        {
            Sync_Timing_OSAL_Wrapper_Strcat(updateFile, gBootfileList[uIdx]);
        }

        Sync_Timing_OSAL_Wrapper_Memcpy(&gBootfileList[uNumBootfiles][0], 
                                        &updateFile[0], 
                                        Sync_Timing_OSAL_Wrapper_Strlen(&updateFile[0]));

        if (Sync_Timing_OSAL_Wrapper_Access(updateFile, F_OK) != -1)
        {
            ;// file exists
            printf("Update File: %s exists.\n", updateFile);
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Update File: %s exists.\n", updateFile);
            }
        }
        else
        {
            printf("*******************************************************************************\n");
            printf("********** Provided update file %s doesn't exist in the path specified ***********\n", 
                   gBootfileList[uIdx]);
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, 
                    "********** Provided update file %s doesn't exist in the path specified ***********\n", 
                   gBootfileList[uIdx]);
            }
            printf("*******************************************************************************\n");
            return;
        }     
        
    }
    printf( "\n**************************************************************************\n");
    
    printf("+++++ Starting Download +++++ \n");
    if (bLogScriptCmdOutputToFile)
    {
        fprintf(fpScriptCmdOutput, "+++++ Starting Download +++++ \n");
    }
    printf("Updating RAM with %s \n", gBootfileList[0]);
    if (bLogScriptCmdOutputToFile)
    {
        fprintf(fpScriptCmdOutput, "Updating RAM with %s \n", gBootfileList[0]);
    }
    printf("Updating RAM with %s \n", gBootfileList[1]);
    if (bLogScriptCmdOutputToFile)
    {
        fprintf(fpScriptCmdOutput, "Updating RAM with %s \n", gBootfileList[1]);
    }

    syncStatus = Sync_Timing_API_Device_Download(gTimingDevId, uNumBootfiles, 
                                                 gBootfileList);

    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf("----- Download COMPLETE ----- \n");
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "----- Download COMPLETE ----- \n");
        }
        Sync_Timing_OSAL_Wrapper_SleepMS(1000);
        syncStatus = Sync_Timing_API_Debug_GetCurrentMode(gTimingDevId, &currDeviceMode);
        if (syncStatus == SYNC_STATUS_SUCCESS)
        {
            printf("----- CURRENT DEVICE MODE = %s ----------\n", 
                   (currDeviceMode == SYNC_TIMING_DEVICE_MODE_APPLN)? "APPLN":"BOOTLOADER");
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "----- CURRENT DEVICE MODE = %s ----------\n", 
                   (currDeviceMode == SYNC_TIMING_DEVICE_MODE_APPLN)? "APPLN":"BOOTLOADER");
            }
        }
        else
        {
            printf("Sync_Timing_API_Debug_GetCurrentMode FAILED syncStatus = %d", syncStatus);
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, 
                        "Sync_Timing_API_Debug_GetCurrentMode FAILED syncStatus = %d", syncStatus);
            }
        }                    
        
        if (currDeviceMode == SYNC_TIMING_DEVICE_MODE_APPLN)
        {
            syncStatus = Sync_Timing_API_Device_GetVersionInfo(gTimingDevId, &deviceVersion);
            if (syncStatus == SYNC_STATUS_SUCCESS)
            {
                printf("Device Firmware Version = %s\n", deviceVersion.fwVersion);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Device Firmware Version = %s\n", 
                            deviceVersion.fwVersion);
                }
            }
        }
    }
    else
    {
        printf("----- Download FAILED ----- syncStatus = %s\n", syncStatusStr[syncStatus]);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "----- Download FAILED ----- syncStatus = %s\n", 
                    syncStatusStr[syncStatus]);
        }
        return;
    }

    printf( "\n**************************************************************************\n");
    
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_ProcessDriverAPICmd
 *
 * DESCRIPTION   : This function process the user inputs for driver commmands and calls 
 *                 corresponding processing functions
 *
 * IN PARAMS     : prog - Program Name
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_DriverApp_ProcessDriverAPICmd(char *inputCmd)
{
    SYNC_TIMING_OSAL_STR_TOKENIZER_T    cmdTok              = {0};
    char                                *pCmd               = NULL;
    char                                parDelim[]          = " ";
    uint32_t                            cmdLen              = 0;
    SYNC_STATUS_E                       syncStatus          = SYNC_STATUS_SUCCESS;

    cmdLen = Sync_Timing_OSAL_Wrapper_Strlen(&inputCmd[0]);

    if (cmdLen > 0 && inputCmd[cmdLen-1] == '\n')
    {
        inputCmd[cmdLen-1] = '\0';
    }
    
    Sync_Timing_OSAL_Wrapper_StrTokenizer_Init(&cmdTok, &inputCmd[0], parDelim, 
                                               SYNC_TIMING_TRUE);
    
    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(&cmdTok))
    {
        pCmd = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(&cmdTok);

        pCmd = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pCmd);

        printf("Driver API Cmd: %s, len = %u\n", pCmd, 
                                            Sync_Timing_OSAL_Wrapper_Strlen(pCmd)); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Driver API Cmd: %s, len = %u\n", pCmd, 
                                        Sync_Timing_OSAL_Wrapper_Strlen(pCmd));
        }
        
        if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "help") == 0)
        {
            /* help command */
            Sync_Timing_DriverApp_PrintHelpDriverMenu();
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "main") == 0)
        {
            /* exit command */
            printf("Exiting Driver Menu.\n");
            return SYNC_STATUS_TIMEOUT;
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "loglevel") == 0)
        {
            /* Process loglevel command */
            Sync_Timing_DriverApp_ProcessLogLevel(&cmdTok);
        }            
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "version") == 0)
        {
            /* version command */
            Sync_Timing_DriverApp_GetVersionInfo();
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "chip_mode") == 0)
        {
            /* Get chipset mode command */
            Sync_Timing_DriverApp_GetChipsetMode();
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "reset") == 0)
        {
            /* Reset chipset and boot from NVM again */
            Sync_Timing_DriverApp_Reset(SYNC_TIMING_FALSE);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "reset_bl") == 0)
        {
            /* Reset chipset and stay in BL mode */
            Sync_Timing_DriverApp_Reset(SYNC_TIMING_TRUE);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "reg_read") == 0)
        {
            /* register read command */
            Sync_Timing_DriverApp_ReadDirect(&cmdTok);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "reg_write") == 0)
        {
            /* register write command */
            Sync_Timing_DriverApp_WriteDirect(&cmdTok);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "raw_api") == 0)
        {
            /* RAW API command */
            Sync_Timing_DriverApp_RawApiCmd(&cmdTok);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "pll_status") == 0)
        {
            /* pll_status command */
            Sync_Timing_DriverApp_GetPllStatus(&cmdTok);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "input_status") == 0)
        {
            /* input_status command */
            Sync_Timing_DriverApp_GetInputStatus(&cmdTok);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "pll_ho") == 0)
        {
            /* force pll ho command */
            Sync_Timing_DriverApp_SetPllForceHO(&cmdTok);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "pll_input") == 0)
        {
            /* change pll input command */
            Sync_Timing_DriverApp_SetPllInput(&cmdTok);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "dload_image") == 0)
        {
            /* Download Bootfile(s) image command */
            Sync_Timing_DriverApp_DownloadImage(&cmdTok);
        }
        else
        {
            printf("Unrecognized Driver command. Enter valid command.\n");
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Unrecognized Driver command. Enter valid command.\n");
            }
        }
    }
    else
    {
        Sync_Timing_DriverApp_PrintHelpDriverMenu();
    }

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_HandleDriverAPICmd
 *
 * DESCRIPTION   : This function receives the user inputs for driver commmands and calls 
 *                 the main processing function
 *
 * IN PARAMS     : prog - Program Name
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_DriverApp_HandleDriverAPICmd(char *prog)
{
    char                                inputCmd[SYNC_TIMING_DRIVER_APP_MAX_CMD_SIZE] = {0};
    SYNC_STATUS_E                       syncStatus          = SYNC_STATUS_SUCCESS;
    char                                *pRet               = NULL;

    Sync_Timing_DriverApp_PrintHelpDriverMenu();

    while(1)
    {
        printf("\nEnter DRIVER Command (Type 'help' to list available driver commands >> ");
        pRet = fgets(inputCmd, SYNC_TIMING_DRIVER_APP_MAX_CMD_SIZE, stdin);
        if (pRet != NULL)
        {
            printf("Command: %s\n", inputCmd); 

            syncStatus = Sync_Timing_DriverApp_ProcessDriverAPICmd(inputCmd);

            if (SYNC_STATUS_TIMEOUT == syncStatus)
            {
                syncStatus = SYNC_STATUS_SUCCESS;
                break;
            }
        }
    }

    return syncStatus;
}


