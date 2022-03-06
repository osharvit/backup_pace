/*****************************************************************************************
 *
 * FILE NAME          : sync_timing_driver_app_main.c
 *
 * AUTHOR             : Srini Venkataraman
 *
 * DATE CREATED       : 12/07/2020
 *
 * DESCRIPTION        : Sync Timing Driver Usage Example Application
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

char syncStatusStr[SYNC_STATUS_MAX][64] = {"SYNC_STATUS_SUCCESS",
                                           "SYNC_STATUS_FAILURE",
                                           "SYNC_STATUS_ALREADY_INITIALIZED",
                                           "SYNC_STATUS_INVALID_PARAMETER",
                                           "SYNC_STATUS_NOT_SUPPORTED",
                                           "SYNC_STATUS_NOT_READY",
                                           "SYNC_STATUS_NOT_INITIALIZED",
                                           "SYNC_STATUS_TIMEOUT",
                                           "SYNC_STATUS_NO_RESOURCES"
                                          };

uint8_t                              gTimingDevId        = 0;
void                                 *gClientId          = 0;


/*****************************************************************************************
    Function Prototypes
 ****************************************************************************************/

static void Sync_Timing_DriverApp_SignalHandler(int s);

static uint32_t Sync_Timing_DriverApp_Driver_Callback(uint8_t timingDevId, uint32_t driverEvent, 
                                                      void * callbackPayload);

static SYNC_STATUS_E Sync_Timing_DriverApp_Init(char *prog);

static SYNC_STATUS_E Sync_Timing_DriverApp_Term(char * prog);

static void Sync_Timing_DriverApp_PrintHelpMainMenu();

/*****************************************************************************************
    Functions
 ****************************************************************************************/

#ifdef OS_LINUX
/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_SignalHandler
 *
 * DESCRIPTION   : This is a LINUX OS only signal handler function for gracefully handling 
 *                 signals from shell when the process is abnormally terminated.
 *
 * IN PARAMS     : sig - Received signal
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_SignalHandler(int sig)
{
    SYNC_STATUS_E   syncStatus = SYNC_STATUS_SUCCESS;

    printf("Caught signal %s\n", strsignal(sig));
    if (gClientId != 0)    
    {
        syncStatus = Sync_Timing_API_Driver_Term(gClientId);
        if (syncStatus != SYNC_STATUS_SUCCESS)
        {
            printf("Sync_Timing_API_Driver_Term failed syncStatus = %s\n", 
                    syncStatusStr[syncStatus]);
        }
    }
    exit(1); 
}
#endif

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_Driver_Callback
 *
 * DESCRIPTION   : This is a callback function registered by the application for receiving 
 *                 async notifications/events from the driver. Only events registered by the 
 *                 application are notified.
 *
 * IN PARAMS     : timingDevId    - The Timing device ID
 *               : driverEvent    - Driver event received
 *               : callbackPayload - Event data received as part of the callback event and 
 *                                   corresponds to structure SYNC_TIMING_DEVICE_DRIVER_EVENT_DATA_T
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : 0
 *
 ****************************************************************************************/
static uint32_t Sync_Timing_DriverApp_Driver_Callback(uint8_t timingDevId, uint32_t driverEvent, 
                                                      void * callbackPayload)
{
    SYNC_TIMING_DEVICE_DRIVER_EVENT_DATA_T      *pDriverEventData;

    pDriverEventData = (SYNC_TIMING_DEVICE_DRIVER_EVENT_DATA_T *)(callbackPayload);

    if (pDriverEventData != NULL)
    {
        switch(driverEvent)
        {
            case SYNC_TIMING_DEVICE_CHIP_EVENT:
                printf("Sync_Timing_UTIL_Driver_Callback invoked "
                             "timingDevId = %u, driverEvent = 0x%08x, "
                             "devicePllEvents = 0x%08x\n", timingDevId, driverEvent,
                             pDriverEventData->deviceEventInfo.devicePllEvents);
                break;
            default:
                break;
        }
    }
    else
    {
        ;
    }
    return 0;
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_Init
 *
 * DESCRIPTION   : This function implements the routines for initializing this application
 *
 * IN PARAMS     : prog - Program Name
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or what is returned by the Driver_API calls.
 *
 ****************************************************************************************/
static SYNC_STATUS_E Sync_Timing_DriverApp_Init(char *prog)
{
    SYNC_STATUS_E                   syncStatus         = SYNC_STATUS_SUCCESS;
    SYNC_TIMING_DEVICE_INFO_T       activeDeviceInfo   = {0};
    SYNC_TIMING_DEVICE_DRIVER_EVENT_FILTER_T drEventFilter = {0};

#ifdef OS_LINUX
    struct sigaction sigIntHandler;
    
    sigIntHandler.sa_handler = Sync_Timing_DriverApp_SignalHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGHUP, &sigIntHandler, NULL);    
    sigaction(SIGINT, &sigIntHandler, NULL);
    sigaction(SIGABRT, &sigIntHandler, NULL);    
    sigaction(SIGTERM, &sigIntHandler, NULL);
#endif

    syncStatus = Sync_Timing_API_Driver_Init(NULL, Sync_Timing_DriverApp_Driver_Callback, 0, &gClientId, 
                                             &activeDeviceInfo);
    /*printf("Sync_Timing_API_Driver_Init returned SyncStatus = %d, guClientAppId = %u\n"
           "ActiveDeviceInfo: %d:%d:%s\n",
           syncStatus, gClientId, 
           activeDeviceInfo.uNumActiveTimingDevices, 
           activeDeviceInfo.timingDeviceId[0], activeDeviceInfo.timingDeviceName[0]);*/

    if (syncStatus != SYNC_STATUS_SUCCESS)
    {
        printf("Sync_Timing_API_Driver_Init failed syncStatus = %s\n", syncStatusStr[syncStatus]);
        exit(1);
    }
    else
    {
        printf("Sync_Timing_API_Driver_Init succeeded syncStatus = %s\n", syncStatusStr[syncStatus]);
    }

    //gTimingDevId = activeDeviceInfo.timingDeviceId[0];
    if (gTimingDevId >= activeDeviceInfo.uNumActiveTimingDevices)
    {
        printf("Invalid Device ID %u specified. Number of Active Timing Devices = %u", 
                                         gTimingDevId, activeDeviceInfo.uNumActiveTimingDevices);
        Sync_Timing_API_Driver_Term(gClientId);
        return SYNC_STATUS_FAILURE;
    }

    /* The following API call shows how register for chip events with the driver
       If interrupts are enabled of the chip and on the host platform, when interrupted the
       driver calls asynchronously the callback function registered during the 
       Sync_Timing_API_Driver_Init call
     */
    drEventFilter.deviceEventFilter.devicePllEvents = SYNC_TIMING_DEVICE_EVENT_LOL_PLL_A | \
                                                      SYNC_TIMING_DEVICE_EVENT_ROL_PLL_A;

    syncStatus = Sync_Timing_API_Driver_RegisterEvents_Ex(gTimingDevId, 
                                                          SYNC_TIMING_DEVICE_CHIP_EVENT,
                                                          &drEventFilter);
    if (syncStatus != SYNC_STATUS_SUCCESS)
    {
        printf("Unable to register events with the timing driver.\n");
        Sync_Timing_API_Driver_Term(gClientId);
    }
    
    return syncStatus;

}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_Term
 *
 * DESCRIPTION   : This function implements the routines for gracefully terminating this application
 *
 * IN PARAMS     : prog - Program Name
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : SYNC_STATUS_SUCCESS or what is returned by Driver_Term.
 *
 ****************************************************************************************/
static SYNC_STATUS_E Sync_Timing_DriverApp_Term(char * prog)
{
    SYNC_STATUS_E                   syncStatus         = SYNC_STATUS_SUCCESS;

    //printf("\n**************************************************************************\n");
    syncStatus = Sync_Timing_API_Driver_Term(gClientId);
    //printf("Sync_Timing_API_Driver_Term returned SyncStatus = %d\n", syncStatus);
    if (syncStatus != SYNC_STATUS_SUCCESS)
    {
        printf("Sync_Timing_API_Driver_Term failed syncStatus = %s", syncStatusStr[syncStatus]);
    }
    //printf("\n**************************************************************************\n");

    return syncStatus;
    
}

/*****************************************************************************************
 * FUNCTION NAME : Sync_Timing_DriverApp_PrintHelpMainMenu
 *
 * DESCRIPTION   : This function is for printing the driver help menu
 *
 * IN PARAMS     : None
 *
 * OUT PARAMS    : None
 *
 * RETURN VALUE  : None
 *
 ****************************************************************************************/
static void Sync_Timing_DriverApp_PrintHelpMainMenu()
{
    printf("************************************************************************\n");
    printf("Main Help Menu\n");
    printf("************************************************************************\n");
    printf("help                                        - Print this help menu.\n");
    printf("driver                                      - Go into Driver API mode.\n");
    printf("fw                                          - Go into FW API mode.\n");
    printf("run_script <input_script> [cmd_output_file] - Execute commands from a script file\n");
    printf("exit                                        - Exit Application.\n");
    printf("************************************************************************\n");
}

int main(int argc, char *argv[])
{
    int32_t                             retVal              = 0;
    char                                inputCmd[SYNC_TIMING_DRIVER_APP_MAX_CMD_SIZE] = {0};
    SYNC_TIMING_OSAL_STR_TOKENIZER_T    cmdTok              = {0};
    char                                *pCmd               = NULL;
    char                                parDelim[]          = " ";
    uint32_t                            cmdLen              = 0;
    SYNC_STATUS_E                       syncStatus          = SYNC_STATUS_SUCCESS;
    char                                *pRet               = NULL;

    syncStatus = Sync_Timing_DriverApp_Init(argv[0]);
    if (syncStatus != SYNC_STATUS_SUCCESS)
    {
        printf("Sync_Timing_DriverApp_Init failed syncStatus = %s", syncStatusStr[syncStatus]);
        exit(1);
    }    

    Sync_Timing_DriverApp_PrintHelpMainMenu();

    while(1)
    {
        printf("\nEnter command (Type 'help' to list available commands >> ");
        pRet = fgets(inputCmd, SYNC_TIMING_DRIVER_APP_MAX_CMD_SIZE, stdin);
        if (pRet == NULL)
        {
            continue;
        }
        
        printf("Command: %s\n", inputCmd); 

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

            printf("pCmd: %s\n", pCmd); 
            
            if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "help") == 0)
            {
                /* help command */
                Sync_Timing_DriverApp_PrintHelpMainMenu();
            }
            else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "exit") == 0)
            {
                /* exit command */
                printf("Exiting driver application.\n");
                break;
            }
            else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "driver") == 0)
            {
                /* Driver API commands */
                printf("Entering Driver Commands mode.\n");
                syncStatus = Sync_Timing_DriverApp_HandleDriverAPICmd(argv[0]);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    printf("Sync_Timing_DriverApp_ProcessDriverAPICmd failed syncStatus = %s", 
                            syncStatusStr[syncStatus]);
                    exit(1);
                }  
                Sync_Timing_DriverApp_PrintHelpMainMenu();
            }
            else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "fw") == 0)
            {
                /* FW API commands */
                printf("Entering FW API Commands mode.\n");
                syncStatus = Sync_Timing_DriverApp_HandleFWAPICmd(argv[0]);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    printf("Sync_Timing_DriverApp_ProcessFWAPICmd failed syncStatus = %s", 
                            syncStatusStr[syncStatus]);
                    exit(1);
                }  
                Sync_Timing_DriverApp_PrintHelpMainMenu();                
            }
            else if(Sync_Timing_OSAL_Wrapper_Strcmp(pCmd, "run_script") == 0)
            {
                /* Script Commands */
                printf("Running Commands from script.\n");
                syncStatus = Sync_Timing_DriverApp_ProcessScript(&cmdTok);
                if (syncStatus != SYNC_STATUS_SUCCESS)
                {
                    printf("Sync_Timing_DriverApp_ProcessFWAPICmd failed syncStatus = %s", 
                            syncStatusStr[syncStatus]);
                    exit(1);
                }  
                Sync_Timing_DriverApp_PrintHelpMainMenu();                
            }
            
            else
            {
                printf("Unknown command. Enter valid command.\n");
            }
        }
        else
        {
            Sync_Timing_DriverApp_PrintHelpMainMenu();
        }
    }

    syncStatus = Sync_Timing_DriverApp_Term(argv[0]);
    if (syncStatus != SYNC_STATUS_SUCCESS)
    {
        printf("Sync_Timing_DriverApp_Term failed syncStatus = %s", syncStatusStr[syncStatus]);
    }    

    return retVal;

}


