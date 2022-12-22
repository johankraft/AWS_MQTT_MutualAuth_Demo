/* -----------------------------------------------------------------------------
 * Copyright (c) 2021 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * -------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "main.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "iot_logging_task.h"
#include "aws_demo.h"

#include "trcRecorder.h"



/* Set logging task as high priority task */
#define LOGGING_TASK_PRIORITY                         (configMAX_PRIORITIES - 1)
#define LOGGING_TASK_STACK_SIZE                       (1440)
#define LOGGING_MESSAGE_QUEUE_LENGTH                  (15)

extern int32_t socket_startup (void);

static const osThreadAttr_t app_main_attr = {
  .stack_size = 4096U
};

#if (configAPPLICATION_ALLOCATED_HEAP == 1U)
#if !(defined(configHEAP_REGION0_ADDR) && (configHEAP_REGION0_ADDR != 0U))
static uint8_t heap_region0[configHEAP_REGION0_SIZE] __ALIGNED(8);
#endif

const HeapRegion_t xHeapRegions[] = {
#if defined(configHEAP_REGION0_ADDR) && (configHEAP_REGION0_ADDR != 0U)
 { (uint8_t *)configHEAP_REGION0_ADDR, configHEAP_REGION0_SIZE },
#else
 { (uint8_t *)heap_region0, configHEAP_REGION0_SIZE },
#endif
 { NULL, 0 }
};
#endif

extern uint32_t uiTraceTickCount;

/*---------------------------------------------------------------------------
 * Application main thread
 *---------------------------------------------------------------------------*/
static void app_main (void *argument) {
  int32_t status;

  (void)argument;

  status = socket_startup();

  if (status == 0) {
    /* Start demos. */
   // DEMO_RUNNER_RunDemos();
  }

  osDelay(osWaitForever);
  for (;;) {}
}






/* Test for marketing example */
#if(0)
extern void * badalloc_malloc( size_t xWantedSize );
#define malloc badalloc_malloc
#define free badalloc_free

#define MAX_SIZE 256

typedef struct
{
  int type;
  size_t size;
  char message[MAX_SIZE]; 
} request_t;



char* otherbuffer = NULL;

// An inbound network request
request_t req1 = {42, 0xFFFFFFF0, "            DataFromNetworkMessage"};

#define get_message_ptr() &req1

#define get_time() 1669303021

static void app_main_test (void *argument) 
{
    int max_length;
    char* formatted_data;
    request_t* request = NULL;

    printf("\nBadAlloc demo\n");
  
    // Some setup needed to reproduce the issue
    char* temp = malloc(12);
    otherbuffer = malloc(256);
    free(temp); 

    sprintf(otherbuffer, "OriginalData");
    


    printf("Adjacent memory before malloc: \"%s\"\n", otherbuffer);
    request = get_message_ptr();
    max_length = request->size + 15;

    if (max_length > MAX_SIZE) return; // max_length is accidentally signed integer (-1)...
        
    printf("malloc(0x%08X)\n", max_length);
    
    formatted_data = malloc(max_length); // Called with 0xFFFFFFFF - allocates 8 bytes instead of fails

    if (formatted_data != NULL){                
        sprintf(formatted_data, "%d: %s", get_time(), request->message);
    }
    printf("Adjacent memory after malloc: \"%s\"\n", otherbuffer);
}
#endif



void vApplicationIdleHook( void )
{    
    if (RecorderDataPtr != NULL)
    {
        
        //printf("Recorder events: %d\n", RecorderDataPtr->numEvents);
        if (RecorderDataPtr->internalErrorOccured == 1)
        {
            printf("Error in recorder: \"%s\"\n", RecorderDataPtr->systemInfo);
            
            portDISABLE_INTERRUPTS();
            for(;;);
        }

    }
}

/*---------------------------------------------------------------------------
 * Application initialization
 *---------------------------------------------------------------------------*/
void app_initialize (void) {

#if (configAPPLICATION_ALLOCATED_HEAP == 1U)
  vPortDefineHeapRegions (xHeapRegions);
#endif

  /* Create logging task */
  xLoggingTaskInitialize (LOGGING_TASK_STACK_SIZE,
                          LOGGING_TASK_PRIORITY,
                          LOGGING_MESSAGE_QUEUE_LENGTH);

  osThreadNew(app_main, NULL, &app_main_attr);
}
