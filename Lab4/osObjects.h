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


extern void Samples_Thread (void const *argument);
extern void GUI_Thread (void const *argument);

osThreadDef (Samples_Thread, osPriorityNormal, 1, 0);
osThreadDef (GUI_Thread, osPriorityNormal, 1, 0);


// global 'message queues' -------------------------------------------------------
#define MSGQ_LEN    16

osMessageQId SampleQId;
osMessageQDef (SampleQ, MSGQ_LEN, uint16_t);

#endif  // __osObjects
