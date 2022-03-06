/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_driver_app_fw_cmd.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 12/14/2020
 *
 * DESCRIPTION        : Sync Timing Driver FW Commands Usage Example
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
#define SYNC_TIMING_PLL_REFCLOCK_BASE 0x20

/*****************************************************************************************
    User-Defined Types (Typedefs)
 ****************************************************************************************/

/*****************************************************************************************
    Global Variable Declarations
*****************************************************************************************/

extern uint8_t              gTimingDevId;
extern char                 syncStatusStr[SYNC_STATUS_MAX][64];
extern SYNC_TIMING_BOOL_E   bLogScriptCmdOutputToFile;
extern FILE                 *fpScriptCmdOutput;

static char         gBootfile[SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ];

static char         gRefClockToStr[8][4] = {"IN0", "INV", "IN1", "INV", "IN2", "IN2B", "IN3", "IN3B"};

static uint8_t      gPllIdToFWMap[4] = {0x1, 0x2, 0x4, 0x80};

/*****************************************************************************************
    Function Prototypes
 ****************************************************************************************/

static void Sync_Timing_DriverApp_PrintHelpFWMenu();

static void Sync_Timing_DriverApp_FWAPIDeviceInfo();

static void Sync_Timing_DriverApp_FWAPIRestart(SYNC_TIMING_BOOL_E bStayInBlMode);

static void Sync_Timing_DriverApp_FWAPIHostLoad(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_FWAPIBoot();

static void Sync_Timing_DriverApp_FWAPIAppInfo();

static void Sync_Timing_DriverApp_FWAPIMetadata();

static void Sync_Timing_DriverApp_FWAPIPllStatus(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_FWAPIInputStatus(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_FWAPIPllForceHoldover(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_FWAPIManualInputClockSelect(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_FWAPIPllActiveRefclock(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_FWAPIReferenceStatus();

static void Sync_Timing_DriverApp_FWAPIInterruptStatus();

static void Sync_Timing_DriverApp_FWAPIClearStatus();

static void Sync_Timing_DriverApp_FWAPIVariableOffsetDco(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_FWAPIPhaseJam1PPS(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_FWAPITempRead();

static void Sync_Timing_DriverApp_FWAPIFreqRead(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

static void Sync_Timing_DriverApp_FWAPIPhaseRead(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok);

/*****************************************************************************************
    Functions
 ****************************************************************************************/

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_PrintHelpFWMenu
 *
 * DESCRIPTION   : This function is used to print FW commands help menu
 *
 * IN PARAMS     : None 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
void Sync_Timing_DriverApp_PrintHelpFWMenu()
{
    printf("************************************\n");
    printf("FW API Commands Help Menu\n");
    printf("************************************\n");
    printf("help                        - Print this help menu.\n");
    printf("dev_info                    - Get and print device info.\n");
    printf("app_info                    - Get and print application info.\n");
    printf("restart                     - Restart chipset - will load image from NVM if present\n");
    printf("restart_bl                  - Restart chipset and stay in bootloader mode\n");
    printf("host_load <bootfile>        - Download bootfile\n");
    printf("boot                        - Send Boot command to FW allowing it to continue booting\n");
    printf("metadata                    - Retrieve Metadata information from FW\n");
    printf("pll_status  <pllId>         - Get Pll Status \n");
    printf("                            - (0 - PLL-R, 1 - PLL-A, 2 - PLL-B)\n");
    printf("input_status <inputId>      - Get Input Status \n");
    printf("                            - (0 - IN0, 2 - IN1, 4 - IN2, 5 - IN2B, 6 - IN3, 7 - IN3B)\n");
    printf("pll_ho  <pllId> <0|1>       - Put or bring PLL into/from force HO \n");
    printf("                            - (0 - PLL-R, 1 - PLL-A, 2 - PLL-B)\n");
    printf("                            - (0 - No Holdover, 1 - Force Holdover)\n");
    printf("pll_input <pllId> <inputId> - Set PLL input \n");
    printf("                            - (0 - PLL-R, 1 - PLL-A, 2 - PLL-B)\n");
    printf("                            - (0 - IN0, 2 - IN1, 4 - IN2, 5 - IN2B, 6 - IN3, 7 - IN3B)\n");
    printf("pll_active_ref <pllId>      - Get Pll's current active refclk \n");
    printf("                            - (0 - PLL-R, 1 - PLL-A, 2 - PLL-B)\n");
    printf("intr_status                 - Get current interrupt status information \n");
    printf("ref_status                  - Get current status of reference input\n");
    printf("clear_status                - Clear all status - pll, input, reference and interrupt\n");
    printf("var_dco <div> <steps>       - Perform Variable DCO for num_steps for specified divider\n");
    printf("                            - 0x1 - MR, 0x2 - NA, 0x4 - MA, 0x8 - NB, 0x10 - MB\n");
    printf("ph_jam_1pps <steps>         - Perform a phase JAM operation on the 1PPS outputs\n");
    printf("temp_read                   - Read the current chipset temperature \n");
    printf("in_period_read <Id1> <Id2>  - Read the period between selected inputs \n");
    printf("                            - (0 - IN0, 2 - IN1, 4 - IN2, 5 - IN2B, 6 - IN3, 7 - IN3B)\n");
    printf("phase_read <grp>            - Read the phase difference between 2 signals; Phase grp setup in fplan\n");
    printf("main                        - Exit FW API Command menu and go back to main menu.\n");
    printf("************************************************************************************\n");
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIDeviceInfo
 *
 * DESCRIPTION   : This function implements the driver command for obtaining Device info
 *
 * IN PARAMS     : None 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIDeviceInfo()
{
    SYNC_STATUS_E                   syncStatus      = SYNC_STATUS_SUCCESS;
    union cmd_arg_union             cmd             = {0};
    union cmd_reply_union           reply           = {0};
    uint32_t                        cmdRespLength   = 0;
    uint8_t                         cmdStatus       = 0;
    uint8_t                         uIdx            = 0;

    cmd.DEVICE_INFO.CMD = cmd_ID_DEVICE_INFO;
    
    cmdRespLength = (uint32_t)sizeof(reply.DEVICE_INFO);
    
    printf("cmd_len = %lu; resp_len = %lu\n", sizeof(cmd.DEVICE_INFO), sizeof(reply.DEVICE_INFO));
    
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                 (uint32_t)sizeof(cmd.DEVICE_INFO), 
                                                 cmdRespLength, 
                                                 &cmdStatus,
                                                 (uint8_t *)&reply, 0, 0);
    
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf("Command Response: (cmd_resp_status = 0x%02x) \n", cmdStatus);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x) \n", 
                    cmdStatus);
        }
        for (uIdx = 0; uIdx < cmdRespLength; uIdx++)
        {
            //printf("0x%02x ", (uint8_t)reply[uIdx]);
        }
        printf("\n");

        printf("Part Number     = Si%04X\n", reply.DEVICE_INFO.PN); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Part Number     = Si%04X\n", reply.DEVICE_INFO.PN); 
        }
        printf("Device Grade    = 0x%x\n", reply.DEVICE_INFO.DG); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Device Grade    = 0x%x\n", reply.DEVICE_INFO.DG); 
        }
        printf("Device Revision = 0x%x\n", reply.DEVICE_INFO.REV); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Device Revision = 0x%x\n", reply.DEVICE_INFO.REV); 
        }
        printf("ROM REV         = %u\n", reply.DEVICE_INFO.ROM); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "ROM REV         = %u\n", reply.DEVICE_INFO.ROM); 
        }
    }
    else
    {
        printf("Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        }
    }
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIRestart
 *
 * DESCRIPTION   : This function implements the FW API command for restarting the chipset
 *
 * IN PARAMS     : bStayInBlMode - Indicates where to stay in BL mode or not after restart
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIRestart(SYNC_TIMING_BOOL_E bStayInBlMode)
{
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    union cmd_arg_union                     cmd                 = {0};
    uint8_t                                 cmdStatus           = 0;

    printf("\n**************************************************************************\n");

    cmd.RESTART.CMD = cmd_ID_RESTART;
    cmd.RESTART.OPTIONS = 0x80;

    if (bStayInBlMode == SYNC_TIMING_TRUE)
    {
        cmd.RESTART.OPTIONS = 0x01;
    }
    
    printf("cmd_len = %lu; \n", sizeof(cmd.RESTART));
    
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                 (uint32_t)sizeof(cmd.RESTART), 
                                                 0, 
                                                 &cmdStatus,
                                                 NULL, 0, 0); 

    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf("----- RESTART COMPLETE ----- \n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "----- RESTART COMPLETE ----- \n"); 
        }
    }
    else
    {
        printf("----- RESTART FAILED ----- syncStatus = %s\n", syncStatusStr[syncStatus]); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "----- RESTART FAILED ----- syncStatus = %s\n", 
                    syncStatusStr[syncStatus]); 
        }
    }
    printf("\n**************************************************************************\n");
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIHostLoad
 *
 * DESCRIPTION   : This function implements the FW API command for host loading bootfile into RAM 
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIHostLoad(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    char                                *pPar               = NULL;
    SYNC_STATUS_E                       syncStatus          = SYNC_STATUS_SUCCESS;
    union cmd_arg_union                 cmd                 = {0};
    union cmd_reply_union               reply               = {0};
    uint32_t                            cmdRespLength       = 0;
    uint8_t                             cmdStatus           = 0;
    uint8_t                             uIdx                = 0;
    uint32_t                            count               = 0;
    uint32_t                            index               = 0;
    uint32_t                            uFileSize           = 0;
    FILE                                *fpBootFD           = NULL;
    uint32_t                            uFilePartitionSize  = SYNC_TIMING_MAX_CMD_DATA_TRANSFER_SIZE;
    uint32_t                            uBytesRead          = 0;

    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        Sync_Timing_OSAL_Wrapper_Memset(&gBootfile[0], 0, SYNC_TIMING_MAX_DEVICE_UPDATE_FILE_NAME_SZ);

        Sync_Timing_OSAL_Wrapper_Memcpy(&gBootfile[0], pPar,
                                        Sync_Timing_OSAL_Wrapper_Strlen(pPar));
        printf("Received Bootfile = %s\n", gBootfile);
 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Received Bootfile = %s\n", gBootfile);
        }

        //Find command buffer size
        cmd.SIO_INFO.CMD = cmd_ID_SIO_INFO;

        cmdRespLength = (uint32_t)sizeof(reply.SIO_INFO);
    
        printf("cmd_len = %lu; resp_len = %lu\n", sizeof(cmd.SIO_INFO), sizeof(reply.SIO_INFO));
        
        syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                     (uint32_t)sizeof(cmd.SIO_INFO), 
                                                     cmdRespLength, 
                                                     &cmdStatus,
                                                     (uint8_t *)&reply, 0, 0);
        
        if (syncStatus == SYNC_STATUS_SUCCESS)
        {
            printf( "Command Response: (cmd_resp_status = 0x%02x) \n", cmdStatus);
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x) \n", 
                        cmdStatus);
            }
            for (uIdx = 0; uIdx < cmdRespLength; uIdx++)
            {
                //printf("0x%02x ", (uint8_t)reply[uIdx]);
            }
            printf("\n");

            printf("CBS     = %u\n", reply.SIO_INFO.CBS); 
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "CBS     = %u\n", reply.SIO_INFO.CBS); 
            }
            printf("RBS     = %u\n", reply.SIO_INFO.RBS); 
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "RBS     = %u\n", reply.SIO_INFO.RBS);
            }

            fpBootFD = fopen(gBootfile, "rb");
            if (fpBootFD == NULL) 
            {
                printf("Error: unable to open %s\n", gBootfile);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Error: unable to open %s\n", gBootfile);
                }                
                return;
            }

            fseek(fpBootFD, 0, SEEK_END); // seek to end of file
            uFileSize = ftell(fpBootFD); // get current file pointer
            fseek(fpBootFD, 0, SEEK_SET); // seek back to beginning of file

            printf("Boot File = %s; uFileSize = %u\n", gBootfile, uFileSize); 
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Boot File = %s; uFileSize = %u\n", 
                        gBootfile, uFileSize); 
            }
                
            while(index < uFileSize)
            {
                count = uFilePartitionSize;
                if ((index + count) > uFileSize)
                {
                    count = uFileSize - index;
                }          

                cmd.HOST_LOAD.CMD = cmd_ID_HOST_LOAD;
                Sync_Timing_OSAL_Wrapper_Memset(&(cmd.HOST_LOAD.DATA[0]), 0, 255);
                
                uBytesRead = fread(&(cmd.HOST_LOAD.DATA[0]), 1, count, fpBootFD);
                
                printf("Boot File = %s; uBytesRead = %u\n", gBootfile, uBytesRead);   
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Boot File = %s; uBytesRead = %u\n", 
                            gBootfile, uBytesRead); 
                } 
                
                syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                             (uint32_t)sizeof(cmd.HOST_LOAD), 
                                                             0, 
                                                             &cmdStatus,
                                                             NULL, 0, 0);                
                
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    printf("Host Load command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
                    if (bLogScriptCmdOutputToFile)
                    {
                        fprintf(fpScriptCmdOutput,
                                "Host Load command failed - cmd_resp_status = 0x%02x \n", 
                                cmdStatus); 
                    }
                    fclose(fpBootFD);
                    return;
                }
                
                index = index + count;
            }
            
            fclose(fpBootFD);
        }
        else
        {
            printf("SIO_INFO Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "SIO_INFO Command failed - cmd_resp_status = 0x%02x \n", 
                        cmdStatus); 
            }
        }
        
    }
    else
    {
        printf("Boot file not specified.\n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Boot file not specified.\n"); 
        }
        Sync_Timing_DriverApp_PrintHelpFWMenu();
        return;
    }            
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIBoot
 *
 * DESCRIPTION   : This function implements the FW API command for booting the chipset; Command must
 *                 be issued when chipset is in bootloader mode and app images are available in NVM 
 *                 or have been downloaded to RAM using host load command.
 *
 * IN PARAMS     : None
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIBoot()
{
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    union cmd_arg_union                     cmd                 = {0};
    uint8_t                                 cmdStatus           = 0;

    printf("\n**************************************************************************\n");

    cmd.BOOT.CMD = cmd_ID_BOOT;
    
    printf("cmd_len = %lu; \n", sizeof(cmd.BOOT));
    
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                 (uint32_t)sizeof(cmd.BOOT), 
                                                 0, 
                                                 &cmdStatus,
                                                 NULL, 0, 0); 

    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf("----- BOOT COMPLETE ----- \n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "----- BOOT COMPLETE ----- \n"); 
        }
    }
    else
    {
        printf("----- BOOT FAILED ----- syncStatus = %s\n", syncStatusStr[syncStatus]); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "----- BOOT FAILED ----- syncStatus = %s\n", 
                    syncStatusStr[syncStatus]); 
        }
    }
    printf("\n**************************************************************************\n");
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIAppInfo
 *
 * DESCRIPTION   : This function implements the FW API command for obtaining Application info
 *
 * IN PARAMS     : None 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIAppInfo()
{
    SYNC_STATUS_E                   syncStatus      = SYNC_STATUS_SUCCESS;
    union cmd_arg_union             cmd             = {0};
    union cmd_reply_union           reply           = {0};
    uint32_t                        cmdRespLength   = 0;
    uint8_t                         cmdStatus       = 0;
    uint8_t                         uIdx            = 0;

    cmd.APP_INFO.CMD = cmd_ID_APP_INFO;
    
    cmdRespLength = (uint32_t)sizeof(reply.APP_INFO);
    
    printf("cmd_len = %lu; resp_len = %lu\n", sizeof(cmd.APP_INFO), sizeof(reply.APP_INFO));
    
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                 (uint32_t)sizeof(cmd.APP_INFO), 
                                                 cmdRespLength, 
                                                 &cmdStatus,
                                                 (uint8_t *)&reply, 0, 0);
    
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf("Command Response: (cmd_resp_status = 0x%02x) \n", cmdStatus);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x) \n", 
                    cmdStatus);
        }
        for (uIdx = 0; uIdx < cmdRespLength; uIdx++)
        {
            //printf("0x%02x ", (uint8_t)reply.APP_INFO[uIdx]);
        }
        printf("\n");
        
        printf("APP Version     = %u.%u.%u_svn_%u\n", reply.APP_INFO.A_MAJOR, 
                                                      reply.APP_INFO.A_MINOR,
                                                      reply.APP_INFO.A_BRANCH, 
                                                      reply.APP_INFO.A_BUILD); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "APP Version     = %u.%u.%u_svn_%u\n", reply.APP_INFO.A_MAJOR, 
                                                      reply.APP_INFO.A_MINOR,
                                                      reply.APP_INFO.A_BRANCH, 
                                                      reply.APP_INFO.A_BUILD);
        }
        printf("PLANNER Version = %u.%u.%u_svn_%u\n", reply.APP_INFO.P_MAJOR, 
                                                      reply.APP_INFO.P_MINOR,
                                                      reply.APP_INFO.P_BRANCH,
                                                      reply.APP_INFO.P_BUILD); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PLANNER Version = %u.%u.%u_svn_%u\n", reply.APP_INFO.P_MAJOR, 
                                                      reply.APP_INFO.P_MINOR,
                                                      reply.APP_INFO.P_BRANCH,
                                                      reply.APP_INFO.P_BUILD); 
        }
        printf("Design ID       = %c%c%c%c%c%c%c%c\n", reply.APP_INFO.DESIGN_ID[0], 
                                                       reply.APP_INFO.DESIGN_ID[1],
                                                       reply.APP_INFO.DESIGN_ID[2], 
                                                       reply.APP_INFO.DESIGN_ID[3],
                                                       reply.APP_INFO.DESIGN_ID[4], 
                                                       reply.APP_INFO.DESIGN_ID[5],
                                                       reply.APP_INFO.DESIGN_ID[6], 
                                                       reply.APP_INFO.DESIGN_ID[7]); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Design ID       = %c%c%c%c%c%c%c%c\n", 
                                                       reply.APP_INFO.DESIGN_ID[0], 
                                                       reply.APP_INFO.DESIGN_ID[1],
                                                       reply.APP_INFO.DESIGN_ID[2], 
                                                       reply.APP_INFO.DESIGN_ID[3],
                                                       reply.APP_INFO.DESIGN_ID[4], 
                                                       reply.APP_INFO.DESIGN_ID[5],
                                                       reply.APP_INFO.DESIGN_ID[6], 
                                                       reply.APP_INFO.DESIGN_ID[7]);
        }
    }
    else
    {
        printf("Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        }
    }
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIMetadata
 *
 * DESCRIPTION   : This function implements the FW API command for obtaining the Metadata
 *
 * IN PARAMS     : None 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIMetadata()
{
    SYNC_STATUS_E                   syncStatus      = SYNC_STATUS_SUCCESS;
    union cmd_arg_union             cmd             = {0};
    union cmd_reply_union           reply           = {0};
    uint32_t                        cmdRespLength   = 0;
    uint8_t                         cmdStatus       = 0;
    uint8_t                         uIdx            = 0;
    double                          ppsStepSize     = 0.0;
    
    cmd.METADATA.CMD = cmd_ID_METADATA;
    
    cmdRespLength = (uint32_t)sizeof(reply.METADATA);
    
    printf("cmd_len = %lu; resp_len = %lu\n", sizeof(cmd.METADATA), sizeof(reply.METADATA));
    
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                 (uint32_t)sizeof(cmd.METADATA), 
                                                 cmdRespLength, 
                                                 &cmdStatus,
                                                 (uint8_t *)&reply, 0, 0);
    
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf( "Command Response: (cmd_resp_status = 0x%02x) \n", cmdStatus);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x) \n", 
                    cmdStatus);
        }
        for (uIdx = 0; uIdx < cmdRespLength; uIdx++)
        {
            //printf("0x%02x ", (uint8_t)reply[uIdx]);
        }
        printf("\n");

        printf("DCO_MR_STEP_SIZE = %u ppt\n", reply.METADATA.DCO_MR_STEP_SIZE); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "DCO_MR_STEP_SIZE = %u ppt\n", 
                                        reply.METADATA.DCO_MR_STEP_SIZE); 
        }
        printf("DCO_NA_STEP_SIZE = %u ppt\n", reply.METADATA.DCO_NA_STEP_SIZE); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "DCO_NA_STEP_SIZE = %u ppt\n", 
                                        reply.METADATA.DCO_NA_STEP_SIZE); 
        }
        printf("DCO_MA_STEP_SIZE = %u ppt\n", reply.METADATA.DCO_MA_STEP_SIZE); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "DCO_MA_STEP_SIZE = %u ppt\n", 
                                        reply.METADATA.DCO_MA_STEP_SIZE); 
        }
        printf("PHASE_JAM_PPS_OUT_RANGE_HIGH = 0x%x\n", 
                    reply.METADATA.PHASE_JAM_PPS_OUT_RANGE_HIGH); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PHASE_JAM_PPS_OUT_RANGE_HIGH = 0x%x\n", 
                    reply.METADATA.PHASE_JAM_PPS_OUT_RANGE_HIGH); 
        }
        printf("PHASE_JAM_PPS_OUT_RANGE_LOW = 0x%x\n", 
                    reply.METADATA.PHASE_JAM_PPS_OUT_RANGE_LOW); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PHASE_JAM_PPS_OUT_RANGE_LOW = %x\n", 
                    reply.METADATA.PHASE_JAM_PPS_OUT_RANGE_LOW); 
        }
        printf("PHASE_JAM_PPS_OUT_STEP_SIZE (U18.46 format) = %lu \n", 
                    reply.METADATA.PHASE_JAM_PPS_OUT_STEP_SIZE); 

        ppsStepSize = (double)((reply.METADATA.PHASE_JAM_PPS_OUT_STEP_SIZE & 0xFFFFC00000000000) >> 46) + \
                       (double)((reply.METADATA.PHASE_JAM_PPS_OUT_STEP_SIZE & 0x3FFFFFFFFFFF)/1e-46);
        
        printf("PHASE_JAM_PPS_OUT_STEP_SIZE (converted) = %0.46lf ns/step \n", ppsStepSize); 
        
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PHASE_JAM_PPS_OUT_STEP_SIZE (U18.46 format) = %lu \n", 
                    reply.METADATA.PHASE_JAM_PPS_OUT_STEP_SIZE); 
        }
        printf("PLAN_OPTIONS = %u\n", 
                    reply.METADATA.PLAN_OPTIONS); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PLAN_OPTIONS = %u\n", 
                    reply.METADATA.PLAN_OPTIONS); 
        }
    }
    else
    {
        printf("Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        }
    }
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIPllStatus
 *
 * DESCRIPTION   : This function implements the FW API command for reading PLL status
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIPllStatus(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    char                                    *pPar               = NULL;
    uint8_t                                 uPllId              = 0;
    uint32_t                                base                = 10;
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    union cmd_arg_union                     cmd                 = {0};
    union cmd_reply_union                   reply               = {0};
    uint32_t                                cmdRespLength       = 0;
    uint8_t                                 cmdStatus           = 0;
    uint8_t                                 uIdx                = 0;

    // Extract specified PLL ID before sending command to FW
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
                Sync_Timing_DriverApp_PrintHelpFWMenu();
                return;
            }

            cmd.PLL_STATUS.CMD = cmd_ID_PLL_STATUS;
            cmd.PLL_STATUS.PLL_SELECT = gPllIdToFWMap[uPllId];
            
            cmdRespLength = (uint32_t)sizeof(reply.PLL_STATUS);
            
            printf("cmd_len = %lu; resp_len = %lu\n", sizeof(cmd.PLL_STATUS), sizeof(reply.PLL_STATUS));

            syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                         (uint32_t)sizeof(cmd.PLL_STATUS), 
                                                         cmdRespLength, 
                                                         &cmdStatus,
                                                         (uint8_t *)&reply, 0, 0);
            
            if (syncStatus == SYNC_STATUS_SUCCESS)
            {
                printf("Command Response: (cmd_resp_status = 0x%02x) \n", cmdStatus);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x) \n", 
                            cmdStatus);
                }
                for (uIdx = 0; uIdx < cmdRespLength; uIdx++)
                {
                    //printf("0x%02x ", (uint8_t)reply[uIdx]);
                }
                printf("\n");

                printf("PLL_LOSS_OF_LOCK_MISC   = 0x%x\n", reply.PLL_STATUS.PLL_LOSS_OF_LOCK_MISC); 
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "PLL_LOSS_OF_LOCK_MISC   = 0x%x\n", 
                            reply.PLL_STATUS.PLL_LOSS_OF_LOCK_MISC);
                }
                printf("PLL_HOLDOVER            = 0x%x\n", reply.PLL_STATUS.PLL_HOLDOVER); 
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "PLL_HOLDOVER            = 0x%x\n", 
                            reply.PLL_STATUS.PLL_HOLDOVER);
                }
            }
            else
            {
                printf( "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Command failed - cmd_resp_status = 0x%02x \n", 
                                        cmdStatus); 
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
            Sync_Timing_DriverApp_PrintHelpFWMenu();
        }
    }
    else
    {
        printf("PLL ID not specified.\n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PLL ID not specified.\n"); 
        }
        Sync_Timing_DriverApp_PrintHelpFWMenu();
    }
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIInputStatus
 *
 * DESCRIPTION   : This function implements the FW API command for reading Input Status
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIInputStatus(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    char                                    *pPar               = NULL;
    uint8_t                                 uInputId            = 0;
    uint32_t                                base                = 10;
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    union cmd_arg_union                     cmd                 = {0};
    union cmd_reply_union                   reply               = {0};
    uint32_t                                cmdRespLength       = 0;
    uint8_t                                 cmdStatus           = 0;
    uint8_t                                 uIdx                = 0;

    // Extract specified Input Reference ID before sending command to FW        
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
                Sync_Timing_DriverApp_PrintHelpFWMenu();
                return;
            }     

            cmd.INPUT_STATUS.CMD = cmd_ID_INPUT_STATUS;
            cmd.INPUT_STATUS.INPUT_SELECT = uInputId;
            
            cmdRespLength = (uint32_t)sizeof(reply.INPUT_STATUS);
            
            printf("cmd_len = %lu; resp_len = %lu\n", sizeof(cmd.INPUT_STATUS), sizeof(reply.INPUT_STATUS));
            
            syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                         (uint32_t)sizeof(cmd.INPUT_STATUS), 
                                                         cmdRespLength, 
                                                         &cmdStatus,
                                                         (uint8_t *)&reply, 0, 0);
            
            if (syncStatus == SYNC_STATUS_SUCCESS)
            {
                printf( "Command Response: (cmd_resp_status = 0x%02x) \n", cmdStatus);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x) \n", 
                            cmdStatus);
                }
                for (uIdx = 0; uIdx < cmdRespLength; uIdx++)
                {
                    //printf("0x%02x ", (uint8_t)reply[uIdx]);
                }
                printf("\n");

                printf("INPUT_CLOCK_VALIDATION  = 0x%x\n", reply.INPUT_STATUS.INPUT_CLOCK_VALIDATION); 
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "INPUT_CLOCK_VALIDATION  = 0x%x\n", 
                            reply.INPUT_STATUS.INPUT_CLOCK_VALIDATION);
                }
                printf("LOSS_OF_SIGNAL          = 0x%x\n", reply.INPUT_STATUS.LOSS_OF_SIGNAL); 
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "LOSS_OF_SIGNAL          = 0x%x\n", 
                            reply.INPUT_STATUS.LOSS_OF_SIGNAL);
                }
                printf("OUT_OF_FREQUENCY        = 0x%x\n", reply.INPUT_STATUS.OUT_OF_FREQUENCY); 
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "OUT_OF_FREQUENCY        = 0x%x\n", 
                            reply.INPUT_STATUS.OUT_OF_FREQUENCY);
                }
                printf("PHASE_MONITOR           = 0x%x\n", reply.INPUT_STATUS.PHASE_MONITOR); 
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "PHASE_MONITOR           = 0x%x\n", 
                            reply.INPUT_STATUS.PHASE_MONITOR);  
                }
            }
            else
            {
                printf( "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Command failed - cmd_resp_status = 0x%02x \n", 
                            cmdStatus);
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
            Sync_Timing_DriverApp_PrintHelpFWMenu();
        }
    }
    else
    {
        printf("INPUT ID not specified.\n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "INPUT ID not specified.\n");  
        }
        Sync_Timing_DriverApp_PrintHelpFWMenu();
    }
    
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIPllForceHoldover
 *
 * DESCRIPTION   : This function implements the FW API command for putting PLLs into Force Holdover
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIPllForceHoldover(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    char                                    *pPar               = NULL;
    uint8_t                                 uPllId              = 0;
    union cmd_arg_union                     cmd                 = {0};
    uint8_t                                 cmdStatus           = 0;
    uint32_t                                base                = 10;
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    uint8_t                                 holdover            = 0xFF;

    // Extract specified PLL ID before sending command to FW        
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
                Sync_Timing_DriverApp_PrintHelpFWMenu();
                return;
            }
        }
        else
        {
            printf("Invalid argument %s supplied for pll_ho option\n", pPar); 
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Invalid argument %s supplied for pll_ho option\n", 
                        pPar); 
            }
            Sync_Timing_DriverApp_PrintHelpFWMenu();
        }
    }
    else
    {
        printf("PLL ID not specified.\n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PLL ID not specified.\n");
        }
        Sync_Timing_DriverApp_PrintHelpFWMenu();
    }

    // Extract specified holdover status before sending command to FW    
    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);
    
        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            holdover = Sync_Timing_OSAL_Wrapper_Atoi(pPar);
    
            if (holdover > 1)
            {
                printf("Invalid holdover option %u supplied.\n", holdover); 
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Invalid holdover option %u supplied.\n", holdover); 
                }
                Sync_Timing_DriverApp_PrintHelpFWMenu();
                return;
            }
        }
        else
        {
            printf("Invalid argument %s supplied for pll_ho option\n", pPar); 
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Invalid argument %s supplied for pll_ho option\n", pPar);
            }
            Sync_Timing_DriverApp_PrintHelpFWMenu();
        }
    }
    else
    {
        printf("PLL ID not specified.\n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PLL ID not specified.\n");
        }
        Sync_Timing_DriverApp_PrintHelpFWMenu();
    }

    
    cmd.FORCE_HOLDOVER.CMD = cmd_ID_FORCE_HOLDOVER;
    cmd.FORCE_HOLDOVER.PLLX = gPllIdToFWMap[uPllId];
    cmd.FORCE_HOLDOVER.HOLDOVER = holdover;
    
    printf("cmd_len = %lu;\n", sizeof(cmd.PLL_STATUS));
    
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                 (uint32_t)sizeof(cmd.FORCE_HOLDOVER), 
                                                 0, 
                                                 &cmdStatus,
                                                 NULL, 0, 0);
    
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf( "Command Response: (cmd_resp_status = 0x%02x) \n", cmdStatus);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x) \n", 
                    cmdStatus);
        }
    }
    else
    {
        printf( "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus);
        }
    }

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIManualInputClockSelect
 *
 * DESCRIPTION   : This function implements the FW API command for setting PLL Input
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIManualInputClockSelect(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    char                                    *pPar               = NULL;
    uint8_t                                 uInputId            = 255;
    uint8_t                                 uPllId              = 255;
    uint32_t                                base                = 10;
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    union cmd_arg_union                     cmd                 = {0};
    uint8_t                                 cmdStatus           = 0;

    // Extract specified PLL ID    
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
                Sync_Timing_DriverApp_PrintHelpFWMenu();
                return;
            }
        }
        else
        {
            printf("PLL ID not specified correctly.\n"); 
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "PLL ID not specified correctly.\n"); 
            }
            Sync_Timing_DriverApp_PrintHelpFWMenu();
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
        Sync_Timing_DriverApp_PrintHelpFWMenu();
        return;
    }            

    // Extract specified Input Reference ID before sending command to FW
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
                Sync_Timing_DriverApp_PrintHelpFWMenu();
                return;
            }
        }
        else
        {
            printf("INPUT ID not specified correctly.\n"); 
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "INPUT ID not specified correctly.\n"); 
            }
            Sync_Timing_DriverApp_PrintHelpFWMenu();
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
        Sync_Timing_DriverApp_PrintHelpFWMenu();
        return;
    }      
    
    cmd.MANUAL_INPUT_CLOCK_SELECT.CMD = cmd_ID_MANUAL_INPUT_CLOCK_SELECT;
    cmd.MANUAL_INPUT_CLOCK_SELECT.PLLX = gPllIdToFWMap[uPllId];
    cmd.MANUAL_INPUT_CLOCK_SELECT.CLOCK_SELECT = uInputId;
    
    printf("cmd_len = %lu;\n", sizeof(cmd.MANUAL_INPUT_CLOCK_SELECT));
    
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                 (uint32_t)sizeof(cmd.MANUAL_INPUT_CLOCK_SELECT), 
                                                 0, 
                                                 &cmdStatus,
                                                 NULL, 0, 0);
    
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf( "Command Response: (cmd_resp_status = 0x%02x) \n", cmdStatus);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x) \n", 
                    cmdStatus);
        }
    }
    else
    {
        printf( "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        }
    }

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIPllActiveRef
 *
 * DESCRIPTION   : This function implements the FW API command for reading PLL Active Ref Clock
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIPllActiveRefclock(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    char                                    *pPar               = NULL;
    uint8_t                                 uPllId              = 0;
    uint32_t                                base                = 10;
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    union cmd_arg_union                     cmd                 = {0};
    union cmd_reply_union                   reply               = {0};
    uint32_t                                cmdRespLength       = 0;
    uint8_t                                 cmdStatus           = 0;
    uint8_t                                 uIdx                = 0;

    // Extract specified PLL ID before sending command to FW    
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
                Sync_Timing_DriverApp_PrintHelpFWMenu();
                return;
            }

            cmd.PLL_ACTIVE_REFCLOCK.CMD = cmd_ID_PLL_ACTIVE_REFCLOCK;
            cmd.PLL_ACTIVE_REFCLOCK.PLLX = gPllIdToFWMap[uPllId];
            
            cmdRespLength = (uint32_t)sizeof(reply.PLL_STATUS);
            
            printf("cmd_len = %lu; resp_len = %lu\n", sizeof(cmd.PLL_ACTIVE_REFCLOCK), 
                                                      sizeof(reply.PLL_ACTIVE_REFCLOCK));
            
            syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                         (uint32_t)sizeof(cmd.PLL_ACTIVE_REFCLOCK), 
                                                         cmdRespLength, 
                                                         &cmdStatus,
                                                         (uint8_t *)&reply, 0, 0);
            
            if (syncStatus == SYNC_STATUS_SUCCESS)
            {
                printf( "Command Response: (cmd_resp_status = 0x%02x) \n", cmdStatus);
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x) \n", 
                            cmdStatus);
                }
                for (uIdx = 0; uIdx < cmdRespLength; uIdx++)
                {
                    //printf("0x%02x ", (uint8_t)reply[uIdx]);
                }
                printf("\n");

                printf("ACTIVE REFCLOCK (0x%x) = %s\n", reply.PLL_ACTIVE_REFCLOCK.REFCLOCK,
                        gRefClockToStr[reply.PLL_ACTIVE_REFCLOCK.REFCLOCK-SYNC_TIMING_PLL_REFCLOCK_BASE]); 
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "ACTIVE REFCLOCK (0x%x) = %s\n", 
                        reply.PLL_ACTIVE_REFCLOCK.REFCLOCK,
                        gRefClockToStr[reply.PLL_ACTIVE_REFCLOCK.REFCLOCK-SYNC_TIMING_PLL_REFCLOCK_BASE]); 
                }
            }
            else
            {
                printf( "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus);  
                }
            }            
        }
        else
        {
            printf("Invalid argument %s supplied for pll_status option\n", pPar); 
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Invalid argument %s supplied for pll_status option\n", pPar); 
            }
            Sync_Timing_DriverApp_PrintHelpFWMenu();
        }
    }
    else
    {
        printf("PLL ID not specified.\n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PLL ID not specified.\n");
        }
        Sync_Timing_DriverApp_PrintHelpFWMenu();
    }
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIReferenceStatus
 *
 * DESCRIPTION   : This function implements the FW API command for reading Reference Input Status
 *
 * IN PARAMS     : None
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIReferenceStatus()
{
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    union cmd_arg_union                     cmd                 = {0};
    union cmd_reply_union                   reply               = {0};
    uint32_t                                cmdRespLength       = 0;
    uint8_t                                 cmdStatus           = 0;
    uint8_t                                 uIdx                = 0;

    cmd.REFERENCE_STATUS.CMD = cmd_ID_REFERENCE_STATUS;
    
    cmdRespLength = (uint32_t)sizeof(reply.REFERENCE_STATUS);
    
    printf("cmd_len = %lu; resp_len = %lu\n", sizeof(cmd.REFERENCE_STATUS), sizeof(reply.REFERENCE_STATUS));
    
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                 (uint32_t)sizeof(cmd.REFERENCE_STATUS), 
                                                 cmdRespLength, 
                                                 &cmdStatus,
                                                 (uint8_t *)&reply, 0, 0);
    
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf( "Command Response: (cmd_resp_status = 0x%02x) \n", cmdStatus);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x) \n", 
                    cmdStatus);
        }
        for (uIdx = 0; uIdx < cmdRespLength; uIdx++)
        {
            //printf("0x%02x ", (uint8_t)reply[uIdx]);
        }
        printf("\n");

        printf("REFERENCE_CLOCK_VALIDATION  = 0x%x\n", 
                    reply.REFERENCE_STATUS.REFERENCE_CLOCK_VALIDATION); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "REFERENCE_CLOCK_VALIDATION  = 0x%x\n", 
                    reply.REFERENCE_STATUS.REFERENCE_CLOCK_VALIDATION);
        }
        printf("LOSS_OF_SIGNAL              = 0x%x\n", reply.REFERENCE_STATUS.LOSS_OF_SIGNAL); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "LOSS_OF_SIGNAL              = 0x%x\n", 
                    reply.REFERENCE_STATUS.LOSS_OF_SIGNAL);
        }
        printf("OUT_OF_FREQUENCY            = 0x%x\n", reply.REFERENCE_STATUS.OUT_OF_FREQUENCY); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "OUT_OF_FREQUENCY            = 0x%x\n", 
                    reply.REFERENCE_STATUS.OUT_OF_FREQUENCY);
        }
        printf("PHASE_MONITOR               = 0x%x\n", reply.REFERENCE_STATUS.PHASE_MONITOR); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PHASE_MONITOR               = 0x%x\n", 
                    reply.REFERENCE_STATUS.PHASE_MONITOR); 
        }
    }
    else
    {
        printf( "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        }
    }         
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIInterruptStatus
 *
 * DESCRIPTION   : This function implements the FW API command for reading current Interrupt Status
 *
 * IN PARAMS     : None
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIInterruptStatus()
{
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    union cmd_arg_union                     cmd                 = {0};
    union cmd_reply_union                   reply               = {0};
    uint32_t                                cmdRespLength       = 0;
    uint8_t                                 cmdStatus           = 0;
    uint8_t                                 uIdx                = 0;

    cmd.INTERRUPT_STATUS.CMD = cmd_ID_INTERRUPT_STATUS;
    
    cmdRespLength = (uint32_t)sizeof(reply.INTERRUPT_STATUS);
    
    printf("cmd_len = %lu; resp_len = %lu\n", sizeof(cmd.INTERRUPT_STATUS), sizeof(reply.INTERRUPT_STATUS));
    
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                 (uint32_t)sizeof(cmd.INTERRUPT_STATUS), 
                                                 cmdRespLength, 
                                                 &cmdStatus,
                                                 (uint8_t *)&reply, 0, 0);
    
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf( "Command Response: (cmd_resp_status = 0x%02x) \n", cmdStatus);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x) \n", 
                    cmdStatus);
        }
        for (uIdx = 0; uIdx < cmdRespLength; uIdx++)
        {
            //printf("0x%02x ", (uint8_t)reply[uIdx]);
        }
        printf("\n");

        printf("INPUT_CLOCK_INVALID INTERRUPT STATUS = 0x%x\n", 
                reply.INTERRUPT_STATUS.INPUT_CLOCK_INVALID); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "INPUT_CLOCK_INVALID INTERRUPT STATUS = 0x%x\n", 
                reply.INTERRUPT_STATUS.INPUT_CLOCK_INVALID);
        }
        printf("INPUT_CLOCK_VALID INTERRUPT STATUS   = 0x%x\n", 
                reply.INTERRUPT_STATUS.INPUT_CLOCK_VALID); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "INPUT_CLOCK_VALID INTERRUPT STATUS   = 0x%x\n", 
                reply.INTERRUPT_STATUS.INPUT_CLOCK_VALID); 
        }
        printf("PLLR INTERRUPT STATUS                = 0x%x\n", reply.INTERRUPT_STATUS.PLLR); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PLLR INTERRUPT STATUS                = 0x%x\n", 
                    reply.INTERRUPT_STATUS.PLLR); 
        }
        printf("PLLA INTERRUPT STATUS                = 0x%x\n", reply.INTERRUPT_STATUS.PLLA); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PLLA INTERRUPT STATUS                = 0x%x\n", 
                    reply.INTERRUPT_STATUS.PLLA);
        }
        printf("PLLB INTERRUPT STATUS                = 0x%x\n", reply.INTERRUPT_STATUS.PLLB); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PLLB INTERRUPT STATUS                = 0x%x\n", 
                    reply.INTERRUPT_STATUS.PLLB);
        }
    }
    else
    {
        printf( "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        }
    }         
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIClearStatus
 *
 * DESCRIPTION   : This function implements the FW API command for clear all status flags
 *
 * IN PARAMS     : None
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIClearStatus()
{
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    union cmd_arg_union                     cmd                 = {0};
    uint8_t                                 cmdStatus           = 0;

    cmd.CLEAR_STATUS_FLAGS.CMD = cmd_ID_CLEAR_STATUS_FLAGS;
    
    printf("cmd_len = %lu; \n", sizeof(cmd.REFERENCE_STATUS));
    
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                 (uint32_t)sizeof(cmd.CLEAR_STATUS_FLAGS), 
                                                 0, 
                                                 &cmdStatus,
                                                 NULL, 0, 0); 

    printf( "Command Response: (cmd_resp_status = 0x%02x) \n", cmdStatus);
    if (bLogScriptCmdOutputToFile)
    {
        fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x) \n", 
                cmdStatus);
    }

    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf("----- ALL STATUS FLAGS CLEARED ----- \n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "----- ALL STATUS FLAGS CLEARED ----- \n"); 
        }
    }
    else
    {
        printf("----- STATUS FLAGS CLEAR FAILED ----- syncStatus = %s\n", syncStatusStr[syncStatus]); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "----- STATUS FLAGS CLEAR FAILED ----- syncStatus = %s\n", 
                    syncStatusStr[syncStatus]); 
        }
    }
    printf("\n**************************************************************************\n");   
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIVariableOffsetDco
 *
 * DESCRIPTION   : This function implements the FW API for Variable Offset DCO
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIVariableOffsetDco(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    char                                    *pPar               = NULL;
    uint32_t                                uDivider            = 0xFF;
    uint32_t                                steps               = 0;
    uint32_t                                base                = 16;
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    union cmd_arg_union                     cmd                 = {0};
    uint8_t                                 cmdStatus           = 0;
    union cmd_reply_union                   reply               = {0};
    uint32_t                                cmdRespLength       = 0;

    // Extract specified Divider ID 
    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            uDivider = (uint32_t)strtol(pPar, NULL, base);

            if (uDivider != 0x1 && uDivider != 0x2 && uDivider != 0x4  && 
                uDivider != 0x8 && uDivider != 0x10)
            {
                printf("Invalid Divider 0x%x supplied.\n", uDivider); 
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Invalid Divider 0x%x supplied.\n", uDivider); 
                }
                Sync_Timing_DriverApp_PrintHelpFWMenu();
                return;
            }
        }
        else
        {
            printf("Divider not specified correctly - Need Hex number.\n"); 
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Divider not specified correctly - Need Hex number.\n"); 
            }
            Sync_Timing_DriverApp_PrintHelpFWMenu();
            return;
        }
    }
    else
    {
        printf("Divider not specified.\n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Divider not specified.\n");
        }
        Sync_Timing_DriverApp_PrintHelpFWMenu();
        return;
    }            

    /* Extract specified DCO Steps - the user knows the step size from on the metadata command
       So the specified steps is a multiple of the known step size */
    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            steps = Sync_Timing_OSAL_Wrapper_Atoi(pPar);

        }
        else
        {
            printf("Steps not specified correctly.\n"); 
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Steps not specified correctly.\n"); 
            }
            Sync_Timing_DriverApp_PrintHelpFWMenu();
            return;
        }
    }
    else
    {
        printf("steps not specified.\n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "steps not specified.\n"); 
        }
        Sync_Timing_DriverApp_PrintHelpFWMenu();
        return;
    }      
    
    cmd.VARIABLE_OFFSET_DCO.CMD = cmd_ID_VARIABLE_OFFSET_DCO;
    cmd.VARIABLE_OFFSET_DCO.DIVIDER_SELECT = (uint8_t)uDivider;
    cmd.VARIABLE_OFFSET_DCO.OFFSET = steps;
    
    printf("cmd_len = %lu; resp_len = %lu\n", sizeof(cmd.VARIABLE_OFFSET_DCO), sizeof(reply.VARIABLE_OFFSET_DCO));
    
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                 (uint32_t)sizeof(cmd.VARIABLE_OFFSET_DCO), 
                                                 cmdRespLength, 
                                                 &cmdStatus,
                                                 (uint8_t *)&reply, 0, 0);

    
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf( "Command Response: (cmd_resp_status = 0x%02x); DCO_STATUS = %u \n", 
                 cmdStatus, reply.VARIABLE_OFFSET_DCO.DCO_STATUS);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x); DCO_STATUS = %u \n", 
                    cmdStatus, reply.VARIABLE_OFFSET_DCO.DCO_STATUS);
        }
    }
    else
    {
        printf( "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        }
    }

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIPhaseJam1PPS
 *
 * DESCRIPTION   : This function implements the FW API command for performing 1PPS Phase JAM
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIPhaseJam1PPS(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    char                                    *pPar               = NULL;
    uint8_t                                 uGroup              = 0;
    uint32_t                                steps               = 0;
    uint32_t                                base                = 10;
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    //union cmd_arg_union                     cmd                 = {0};
    uint8_t                                 cmdStatus           = 0;
    //union cmd_reply_union                   reply               = {0};
    uint32_t                                cmdRespLength       = 0;
    cmd_PHASE_JAM_PPS_OUT_map_t             cmd                 = {0};
    reply_PHASE_JAM_PPS_OUT_map_t           reply               = {0};

#if 0
    // Extract group number
    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            uGroup = Sync_Timing_OSAL_Wrapper_Atoi(pPar);
        }
        else
        {
            printf("Phase Jam Group not specified correctly.\n"); 
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Phase Jam Group not specified correctly.\n");
            }
            Sync_Timing_DriverApp_PrintHelpFWMenu();
            return;
        }
    }
    else
    {
        printf("Phase Jam Divider Group not specified.\n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Phase Jam Group number not specified.\n");
        }
        Sync_Timing_DriverApp_PrintHelpFWMenu();
        return;
    }            
#endif

    // Extract steps specified by the user
    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            steps = Sync_Timing_OSAL_Wrapper_Atoi(pPar);

        }
        else
        {
            printf("Steps not specified correctly.\n"); 
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Steps not specified correctly.\n"); 
            }
            Sync_Timing_DriverApp_PrintHelpFWMenu();
            return;
        }
    }
    else
    {
        printf("steps not specified.\n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "steps not specified.\n");
        }
        Sync_Timing_DriverApp_PrintHelpFWMenu();
        return;
    }      
    
    cmd.CMD = cmd_ID_PHASE_JAM_PPS_OUT;
    cmd.GROUP_SELECT = (uint8_t)uGroup;
    cmd.OFFSET = steps;
    
    printf("cmd_len = %lu; resp_len = %lu\n", sizeof(cmd.CMD), sizeof(reply));
    
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                 (uint32_t)sizeof(cmd), 
                                                 cmdRespLength, 
                                                 &cmdStatus,
                                                 (uint8_t *)&reply, 0, 0);

    
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf( "Command Response: (cmd_resp_status = 0x%02x); PHASE_JAM_STATUS = %u \n", 
                 cmdStatus, reply.PHASE_JAM_STATUS);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x); PHASE_JAM_STATUS = %u \n", 
                    cmdStatus, reply.PHASE_JAM_STATUS);
        }
    }
    else
    {
        printf( "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus);
        }
    }

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPITempRead
 *
 * DESCRIPTION   : This function implements the FW API command for reading die temperature
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPITempRead()
{
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    union cmd_arg_union                     cmd                 = {0};
    union cmd_reply_union                   reply               = {0};
    uint32_t                                cmdRespLength       = 0;
    uint8_t                                 cmdStatus           = 0;
    uint8_t                                 uIdx                = 0;
    uint8_t                                 temp_int            = 0;
    uint32_t                                temp_frac           = 0;
    uint8_t                                 temp_sign           = 0;

    cmd.TEMPERATURE_READOUT.CMD = cmd_ID_TEMPERATURE_READOUT;
    
    cmdRespLength = (uint32_t)sizeof(reply.TEMPERATURE_READOUT);
    
    printf("cmd_len = %lu; resp_len = %lu\n", 
            sizeof(cmd.TEMPERATURE_READOUT), sizeof(reply.TEMPERATURE_READOUT));
    
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                 (uint32_t)sizeof(cmd.TEMPERATURE_READOUT), 
                                                 cmdRespLength, 
                                                 &cmdStatus,
                                                 (uint8_t *)&reply, 0, 0);
    
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf( "Command Response: (cmd_resp_status = 0x%02x) \n", cmdStatus);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x) \n", 
                    cmdStatus);
        }
        for (uIdx = 0; uIdx < cmdRespLength; uIdx++)
        {
            //printf("0x%02x ", (uint8_t)reply[uIdx]);
        }
        printf("\n");

        temp_int = (reply.TEMPERATURE_READOUT.TEMPERATURE_READOUT & 0x7F800000) >> 23;
        temp_frac = (reply.TEMPERATURE_READOUT.TEMPERATURE_READOUT & 0x7FFFFF);
        temp_sign = (reply.TEMPERATURE_READOUT.TEMPERATURE_READOUT & 0x80000000);

        
        printf("DIE TEMPERATURE READOUT  = %d; %c%u.%u Celsius\n", 
                        reply.TEMPERATURE_READOUT.TEMPERATURE_READOUT,
                        (temp_sign?'-':'+'), temp_int, temp_frac);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "DIE TEMPERATURE READOUT  = %d; %c%u.%u Celsius\n", 
                        reply.TEMPERATURE_READOUT.TEMPERATURE_READOUT,
                        (temp_sign?'-':'+'), temp_int, temp_frac);
        }
    }
    else
    {
        printf( "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        }
    }   
}


/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIFreqRead
 *
 * DESCRIPTION   : This function implements the FW API command for reading periods of specified ref inputs
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIFreqRead(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    char                                    *pPar               = NULL;
    uint8_t                                 uInputId1           = 0;
    uint8_t                                 uInputId2           = 0;
    uint32_t                                base                = 10;
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    union cmd_arg_union                     cmd                 = {0};
    union cmd_reply_union                   reply               = {0};
    uint32_t                                cmdRespLength       = 0;
    uint8_t                                 cmdStatus           = 0;
    uint8_t                                 uIdx                = 0;

    // Extract specified Input Reference ID before sending command to FW        
    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            uInputId1 = Sync_Timing_OSAL_Wrapper_Atoi(pPar);

            if (uInputId1 > 7)
            {
                printf("Invalid Input ID1 %u supplied.\n", uInputId1); 
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Invalid Input ID1 %u supplied.\n", uInputId1); 
                }
                Sync_Timing_DriverApp_PrintHelpFWMenu();
                return;
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
            Sync_Timing_DriverApp_PrintHelpFWMenu();
        }
    }
    else
    {
        printf("INPUT ID 1 not specified.\n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "INPUT ID 1 not specified.\n"); 
        }
        Sync_Timing_DriverApp_PrintHelpFWMenu();
    }

    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            uInputId2 = Sync_Timing_OSAL_Wrapper_Atoi(pPar);

            if (uInputId2 > 7)
            {
                printf("Invalid Input ID2 %u supplied.\n", uInputId1); 
                if (bLogScriptCmdOutputToFile)
                {
                    fprintf(fpScriptCmdOutput, "Invalid Input ID2 %u supplied.\n", uInputId1); 
                }
                Sync_Timing_DriverApp_PrintHelpFWMenu();
                return;
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
            Sync_Timing_DriverApp_PrintHelpFWMenu();
        }
    }
    else
    {
        printf("INPUT ID 2 not specified.\n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "INPUT ID 2 not specified.\n"); 
        }
        Sync_Timing_DriverApp_PrintHelpFWMenu();
    }


    cmd.INPUT_PERIOD_READOUT.CMD = cmd_ID_INPUT_PERIOD_READOUT;
    cmd.INPUT_PERIOD_READOUT.REFERENCE_SELECT_A = uInputId1;
    cmd.INPUT_PERIOD_READOUT.REFERENCE_SELECT_B = uInputId2;
    
    cmdRespLength = (uint32_t)sizeof(reply.INPUT_PERIOD_READOUT);
    
    printf("cmd_len = %lu; resp_len = %lu\n", sizeof(cmd.INPUT_PERIOD_READOUT), sizeof(reply.INPUT_PERIOD_READOUT));
    
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                 (uint32_t)sizeof(cmd.INPUT_PERIOD_READOUT), 
                                                 cmdRespLength, 
                                                 &cmdStatus,
                                                 (uint8_t *)&reply, 0, 0);
    
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf( "Command Response: (cmd_resp_status = 0x%02x) \n", cmdStatus);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x) \n", 
                    cmdStatus);
        }
        for (uIdx = 0; uIdx < cmdRespLength; uIdx++)
        {
            //printf("0x%02x ", (uint8_t)reply[uIdx]);
        }
        printf("\n");
    
        printf("PERIOD_READOUT_A  = %lu\n", reply.INPUT_PERIOD_READOUT.PERIOD_READOUT_A); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PERIOD_READOUT_A  = %lu\n", 
                    reply.INPUT_PERIOD_READOUT.PERIOD_READOUT_A);
        }
        printf("PERIOD_READOUT_B  = %lu\n", reply.INPUT_PERIOD_READOUT.PERIOD_READOUT_B); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "PERIOD_READOUT_B  = %lu\n", 
                    reply.INPUT_PERIOD_READOUT.PERIOD_READOUT_B); 
        }
    }
    else
    {
        printf( "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus); 
        }
    }   
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_FWAPIPhaseRead
 *
 * DESCRIPTION   : This function implements the FW API command for reading phase difference between 
 *                 2 signals in a group
 *
 * IN PARAMS     : pParTok - Input string containing the data for the command 
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_FWAPIPhaseRead(SYNC_TIMING_OSAL_STR_TOKENIZER_T *pParTok)
{
    char                                    *pPar               = NULL;
    uint8_t                                 uGroup              = 0;
    uint32_t                                base                = 10;
    SYNC_STATUS_E                           syncStatus          = SYNC_STATUS_SUCCESS;
    union cmd_arg_union                     cmd                 = {0};
    uint8_t                                 cmdStatus           = 0;
    union cmd_reply_union                   reply               = {0};
    uint32_t                                cmdRespLength       = 0;

    // Extract group number
    if (Sync_Timing_OSAL_Wrapper_StrTokenizer_HasMoreTokens(pParTok))
    {   
        pPar = Sync_Timing_OSAL_Wrapper_StrTokenizer_NextToken(pParTok);
    
        pPar = Sync_Timing_OSAL_Wrapper_StrRemoveLeadingSpaces(pPar);

        if (Sync_Timing_DriverApp_Is_Number(pPar, &base))
        {
            uGroup = Sync_Timing_OSAL_Wrapper_Atoi(pPar);
        }
        else
        {
            printf("Phase Jam Group not specified correctly.\n"); 
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Phase Jam Group not specified correctly.\n"); 
            }
            Sync_Timing_DriverApp_PrintHelpFWMenu();
            return;
        }
    }
    else
    {
        printf("Phase JAM Group not specified.\n"); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Phase JAM Group not specified.\n");
        }
        Sync_Timing_DriverApp_PrintHelpFWMenu();
        return;
    }            

    cmd.PHASE_READOUT.CMD = cmd_ID_PHASE_READOUT;
    cmd.PHASE_READOUT.PHASE_GROUP = (uint8_t)uGroup;
    
    printf("cmd_len = %lu; resp_len = %lu\n", sizeof(cmd.PHASE_READOUT), sizeof(reply.PHASE_READOUT));

    cmdRespLength = sizeof(reply.PHASE_READOUT);
    
    syncStatus = Sync_Timing_API_Mem_SendCommand(gTimingDevId, (uint8_t *)&cmd, 
                                                 (uint32_t)sizeof(cmd.PHASE_READOUT), 
                                                 cmdRespLength, 
                                                 &cmdStatus,
                                                 (uint8_t *)&reply, 0, 0);

    
    if (syncStatus == SYNC_STATUS_SUCCESS)
    {
        printf("Command Response: (cmd_resp_status = 0x%02x); "
               "PHASE_ERRORS = %u, "
               "PHASE_READOUT_DIFFERENCE = %ld, "
               "PHASE_VALID =  %u\n", 
               cmdStatus, 
               reply.PHASE_READOUT.ERRORS,
               reply.PHASE_READOUT.PHASE_READOUT, 
               reply.PHASE_READOUT.PHASE_VALID);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command Response: (cmd_resp_status = 0x%02x); "
                                       "PHASE_ERRORS = %u, "
                                       "PHASE_READOUT_DIFFERENCE = %ld, "
                                       "PHASE_VALID =  %u\n", 
                                       cmdStatus, 
                                       reply.PHASE_READOUT.ERRORS,
                                       reply.PHASE_READOUT.PHASE_READOUT, 
                                       reply.PHASE_READOUT.PHASE_VALID);
        }
    }
    else
    {
        printf( "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus);
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "Command failed - cmd_resp_status = 0x%02x \n", cmdStatus);
        }
    }

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_ProcessFWAPICmd
 *
 * DESCRIPTION   : This function process the FW API commmands and calls 
 *                 corresponding processing functions
 *
 * IN PARAMS     : prog - Program Name
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS
 *
 ****************************************************************************************/
SYNC_STATUS_E Sync_Timing_DriverApp_ProcessFWAPICmd(char *inputCmd)
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
    
        printf("FW API Cmd: %s\n", pCmd); 
        if (bLogScriptCmdOutputToFile)
        {
            fprintf(fpScriptCmdOutput, "FW API Cmd: %s\n", pCmd); 
        }
        
        if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "help") == 0)
        {
            /* help command */
            Sync_Timing_DriverApp_PrintHelpFWMenu();
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "main") == 0)
        {
            /* exit command */
            printf("Exiting Driver Menu.\n");
            return SYNC_STATUS_TIMEOUT;
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "dev_info") == 0)
        {
            /* Device Info command */
            Sync_Timing_DriverApp_FWAPIDeviceInfo();
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "restart") == 0)
        {
            /* Restart command */
            Sync_Timing_DriverApp_FWAPIRestart(SYNC_TIMING_FALSE);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "restart_bl") == 0)
        {
            /* Restart into bootloader mode command */
            Sync_Timing_DriverApp_FWAPIRestart(SYNC_TIMING_TRUE);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "boot") == 0)
        {
            /* Boot command */
            Sync_Timing_DriverApp_FWAPIBoot();
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "host_load") == 0)
        {
            /* Host Load command */
            Sync_Timing_DriverApp_FWAPIHostLoad(&cmdTok);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "app_info") == 0)
        {
            /* App Info command */
            Sync_Timing_DriverApp_FWAPIAppInfo();
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "metadata") == 0)
        {
            /* Metadata command */
            Sync_Timing_DriverApp_FWAPIMetadata();
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "pll_status") == 0)
        {
            /* pll_status command */
            Sync_Timing_DriverApp_FWAPIPllStatus(&cmdTok);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "input_status") == 0)
        {
            /* input_status command */
            Sync_Timing_DriverApp_FWAPIInputStatus(&cmdTok);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "pll_ho") == 0)
        {
            /* force pll ho command */
            Sync_Timing_DriverApp_FWAPIPllForceHoldover(&cmdTok);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "pll_input") == 0)
        {
            /* change pll input command */
            Sync_Timing_DriverApp_FWAPIManualInputClockSelect(&cmdTok);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "pll_active_ref") == 0)
        {
            /* change pll active refclock command */
            Sync_Timing_DriverApp_FWAPIPllActiveRefclock(&cmdTok);
        }            
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "ref_status") == 0)
        {
            /* Reference Status command */
            Sync_Timing_DriverApp_FWAPIReferenceStatus();
        }            
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "intr_status") == 0)
        {
            /* Interrupt Status command */
            Sync_Timing_DriverApp_FWAPIInterruptStatus(&cmdTok);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "clear_status") == 0)
        {
            /* Clear Status command */
            Sync_Timing_DriverApp_FWAPIClearStatus(&cmdTok);
        }
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "var_dco") == 0)
        {
            /* Variable Offset DCO command */
            Sync_Timing_DriverApp_FWAPIVariableOffsetDco(&cmdTok);
        }                  
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "ph_jam_1pps") == 0)
        {
            /* Phase JAM 1PPS command */
            Sync_Timing_DriverApp_FWAPIPhaseJam1PPS(&cmdTok);
        }                  
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "temp_read") == 0)
        {
            /* Temperature readout command */
            Sync_Timing_DriverApp_FWAPITempRead();
        }                  
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "in_period_read") == 0)
        {
            /* Frequency readout command */
            Sync_Timing_DriverApp_FWAPIFreqRead(&cmdTok);
        }                  
        else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "phase_read") == 0)
        {
            /* Phase readout command */
            Sync_Timing_DriverApp_FWAPIPhaseRead(&cmdTok);
        }                  
        else
        {
            printf("Unrecognized FW API command. Enter valid command.\n");
            if (bLogScriptCmdOutputToFile)
            {
                fprintf(fpScriptCmdOutput, "Unrecognized FW API command. Enter valid command.\n");
            }
        }
    }
    else
    {
        Sync_Timing_DriverApp_PrintHelpFWMenu();
    }

    return syncStatus;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_HandleFWAPICmd
 *
 * DESCRIPTION   : This function requests user for the FW API command and processes it
 *
 * IN PARAMS     : prog - Program Name
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS
 *
 ****************************************************************************************/
SYNC_STATUS_E   Sync_Timing_DriverApp_HandleFWAPICmd(char *prog)
{
    char                                inputCmd[SYNC_TIMING_DRIVER_APP_MAX_CMD_SIZE] = {0};
    SYNC_STATUS_E                       syncStatus          = SYNC_STATUS_SUCCESS;
    char                                *pRet               = NULL;

    Sync_Timing_DriverApp_PrintHelpFWMenu();

    while(1)
    {
        printf("\nEnter FW API Command (Type 'help' to list available FW API commands >> ");
        pRet = fgets(inputCmd, SYNC_TIMING_DRIVER_APP_MAX_CMD_SIZE, stdin);
        if (pRet != NULL)
        {
            printf("Command: %s\n", inputCmd); 

            syncStatus = Sync_Timing_DriverApp_ProcessFWAPICmd(inputCmd);

            if (SYNC_STATUS_TIMEOUT == syncStatus)
            {
                syncStatus = SYNC_STATUS_SUCCESS;
                break;
            }
        }
    }

    return syncStatus;
}



