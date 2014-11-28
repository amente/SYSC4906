#ifndef __RTX
#error "CMSIS RTX required!"
#endif

#include <stdlib.h>
#include <string.h>
#include "../osObjects.h"
#include "../ff.h"


#if _FS_REENTRANT
/*------------------------------------------------------------------------*/
/* Create a Synchronization Object                                        */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount() function to create a new
/  synchronization object, such as semaphore and mutex. When a 0 is returned,
/  the f_mount() function fails with FR_INT_ERR.
*/

int ff_cre_syncobj (	/* !=0:Function succeeded, ==0:Could not create due to any error */
	BYTE vol,			/* Corresponding logical drive being processed */
	_SYNC_t *mutexId		/* Pointer to return the created sync object */
)
{
    uint32_t *array;
    osMutexDef_t *FS_MutexDef_p;
    
     /**
        #define osMutexDef(name)  \
        uint32_t os_mutex_cb_##name[4] = { 0 }; \
        const osMutexDef_t os_mutex_def_##name = { (os_mutex_cb_##name) }
    
        This could change in the future but will do for now.
        The steps below should emulate the above behavior.
    */
    
    // create the uint32_t array and init to zero
    if ( (array = calloc(4, sizeof(uint32_t))) == NULL )
        return 0;
    
    // create the osMutexDef_t struct
    if ( (FS_MutexDef_p = malloc(sizeof(osMutexDef_t))) == NULL )
        return 0;
    
    // init the osMutexDef_t struct
    FS_MutexDef_p->mutex = array;
    
    // create the mutex
    *mutexId = osMutexCreate(FS_MutexDef_p);
    
    return (int)*mutexId;
}



/*------------------------------------------------------------------------*/
/* Delete a Synchronization Object                                        */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount() function to delete a synchronization
/  object that created with ff_cre_syncobj function. When a 0 is returned,
/  the f_mount() function fails with FR_INT_ERR.
*/

int ff_del_syncobj (	/* !=0:Function succeeded, ==0:Could not delete due to any error */
	_SYNC_t mutexId		/* Sync object tied to the logical drive to be deleted */
)
{
	if (osMutexDelete(mutexId) == osOK)
        return 1;
    else
        return 0;
}



/*------------------------------------------------------------------------*/
/* Request Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on entering file functions to lock the volume.
/  When a 0 is returned, the file function fails with FR_TIMEOUT.
*/

int ff_req_grant (	/* 1:Got a grant to access the volume, 0:Could not get a grant */
	_SYNC_t mutexId	/* Sync object to wait */
)
{
	if ( (osMutexWait(mutexId, FR_TIMEOUT) == osOK ) )
        return 1;
    else
        return 0;
}



/*------------------------------------------------------------------------*/
/* Release Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on leaving file functions to unlock the volume.
*/

void ff_rel_grant (
	_SYNC_t mutexId	/* Sync object to be signaled */
)
{
	osMutexRelease(mutexId);
}

#endif




#if _USE_LFN == 3	/* LFN with a working buffer on the heap */
/*------------------------------------------------------------------------*/
/* Allocate a memory block                                                */
/*------------------------------------------------------------------------*/
/* If a NULL is returned, the file function fails with FR_NOT_ENOUGH_CORE.
*/

void* ff_memalloc (	/* Returns pointer to the allocated memory block */
	UINT msize		/* Number of bytes to allocate */
)
{
	return malloc(msize);	/* Allocate a new memory block with POSIX API */
}


/*------------------------------------------------------------------------*/
/* Free a memory block                                                    */
/*------------------------------------------------------------------------*/

void ff_memfree (
	void* mblock	/* Pointer to the memory block to free */
)
{
	free(mblock);	/* Discard the memory block with POSIX API */
}

#endif
