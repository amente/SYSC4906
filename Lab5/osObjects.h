/*----------------------------------------------------------------------------
 * osObjects.h: CMSIS-RTOS global object definitions for an application
 *----------------------------------------------------------------------------
 *
 * This header file defines global RTOS objects used throughout a project
 *
 * #define osObjectsPublic indicates that objects are defined; without that
 * definition the objects are defined as external symbols.
 *
 *--------------------------------------------------------------------------*/


#ifndef __osObjects
#define __osObjects

#if (!defined (osObjectsPublic))
#define osObjectsExternal          // define RTOS objects with extern attribute
#endif

#include <cmsis_os.h>              // CMSIS RTOS header file


// global 'thread' functions ---------------------------------------------------

extern void Player_Thread (void const *argument);
extern osThreadId  PlayerThreadId;
osThreadDef (Player_Thread, osPriorityNormal, 1, 2048);

extern void GUI_Thread (void const *argument);
extern osThreadId  GUIThreadId;
osThreadDef (GUI_Thread, osPriorityNormal, 1, 1024);

// global 'semaphores' ----------------------------------------------------------
/* 
Example:
osSemaphoreId sid_sample_name;                           // semaphore id
osSemaphoreDef (sample_name);                            // semaphore object
*/


// global 'memory pools' --------------------------------------------------------
/* 
Example:
typedef struct sample_name type_sample_name;             // object data type

osPoolId mpid_sample_name;                               // memory pool id
osPoolDef (sample_name, 16, type_sample_name);           // memory pool object
*/


// global 'message queues' -------------------------------------------------------

extern osMessageQId PlayerMsgQId;                           // message queue id
osMessageQDef (PlayerMsgQ, 16, (const char *));             // message queue object


// global 'mail queues' ----------------------------------------------------------
/* 
Example:
typedef struct sample_name type_sample_name;             // object data type

osMailQId qid_sample_name;                               // mail queue id
osMailQDef (sample_name, 16, type_sample_name);          // mail queue object
*/

#endif  // __osObjects
