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
#include "main.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "iot_logging_task.h"
#include "aws_demo.h"


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

/*---------------------------------------------------------------------------
 * Application main thread
 *---------------------------------------------------------------------------*/
static void app_main2 (void *argument) {
  int32_t status;

  (void)argument;

  status = socket_startup();

  if (status == 0) {
    /* Start demos. */
    DEMO_RUNNER_RunDemos();
  }

  osDelay(osWaitForever);
  for (;;) {}
}

extern void * badalloc_malloc( size_t xWantedSize );
#define malloc badalloc_malloc
#define free badalloc_free

// Idea - Add the old BadAlloc "malloc" function and make an example exploit
// Like pvPortMalloc(reqeust.size) where size is 0xFFFF FFFF leads to 7 byte allocation
// What should it be? Buffer overrun?

#define MAX_SIZE 256

typedef struct
{
  int type;
  int size;
  char message[MAX_SIZE]; 
} request_t;


char* copy_message(request_t* req)
{
    char* response;

    // Input validation (insufficient)
    if (req->size > MAX_SIZE) 
        return NULL;
    
    printf("malloc(0x%08X)\n", req->size);
 
    // Allocates only 8 bytes when req->size is 0xFFFFFFFF (-1)
    response = malloc(req->size);
    
    // Causes buffer overrun, corrupting "otherbuffer" since allocation is smaller than expected
    strcpy(response, req->message);
    return response;
}

static void app_main (void *argument) 
{
    // An inbound network request
    request_t req = {42, -1, "                        ABCDEFGHIJ"};

    printf("\nBadAlloc demo\n");
  
    // Some setup needed to reproduce the issue
    char* temp = malloc(12);
    char* otherbuffer = malloc(256);
    free(temp); 

    sprintf(otherbuffer, "1234567890");
    printf("otherbuffer before malloc: \"%s\"\n", otherbuffer);

    copy_message(&req); // uses vulnareble malloc

    printf("otherbuffer after malloc: \"%s\"\n", otherbuffer);
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
