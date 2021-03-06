/* $Id$
********************************************************************************
* _____   ____ _______    _______ ____  ______  _____                          *
*|  __ \ / __ \__   __|/\|__   __/ __ \|  ____|/ ____|          Copyright 2008 *
*| |__) | |  | | | |  /  \  | | | |  | | |__  | (___              Daniel Bader *
*|  ___/| |  | | | | / /\ \ | | | |  | |  __|  \___ \           Vincenz Doelle *
*| |    | |__| | | |/ ____ \| | | |__| | |____ ____) |    Johannes Schamburger *
*|_|     \____/  |_/_/    \_\_|  \____/|______|_____/          Dmitriy Traytel *
*                                                                              *
*      Practical Oriented TeAching Tool, Operating (and) Educating System      *
*                                                                              *
*                           www.potatoes-project.tk                            *
*******************************************************************************/

/**
 * @file
 * Functions to allocate and free memory
 *
 * @author Johannes Schamburger
 * @author $LastChangedBy$
 * @version $Rev$
 */

#include "../include/types.h"
#include "../include/const.h"
#include "../include/stdio.h"
#include "../include/string.h"
#include "../include/debug.h"
#include "../mm/mm.h"
#include "../mm/mm_paging.h"


//TODO: comment!
static long _seed = 1L;

void srand(unsigned int seed)
{
        _seed = (long) seed;
}

int rand()
{
        return (((_seed = _seed * 214013L + 2531011L) >> 16) & 0x7fff);
}

/*
 * Enables a braindead memory "manager" for debugging purposes. (daniel)
 * Use this to make sure a bug is not related to MM.
 */
//#define MEM_FAILSAFE

#ifdef MEM_FAILSAFE
void* mem = (void*) 0x300000; // assume this is past the kernel code...
#endif

/**
 * Allocates 'size' bytes and additionally saves a name in the header of the block.
 * ATTENTION: if malloc fails (i.e. there is not enough free space), the return value is
 * (void*) NULL. So, this should always be tested!
 *
 * @param size  how much space shall be allocated
 * @param name  name of the memory block (mainly for debugging purposes)
 * @return      pointer to the allocated space
 */
void* mallocn(size_t size, char *name)
{
#ifdef MEM_FAILSAFE
        void *ret = mem;
        mem += size;
        return ret;
#endif

        return heap_mallocn(size, name, 0, kernel_heap);
}

/**
 * Allocates 'size' bytes.
 * ATTENTION: if malloc fails (i.e. there is not enough free space), the return value is
 * (void*) NULL. So, this should always be tested!
 *
 * @param size  how much space shall be allocated
 * @return      pointer to the allocated space
 */
void* malloc(size_t size)
{
        return (void*) mallocn(size, "noname");
}

/**
 * Allocates space for n elements of the same size and additionally saves a name in
 * the header of the block.
 *
 * @param n     number of elements
 * @param size  size of each element
 * @param name  name of the memory block (mainly for debugging purposes)
 */
void* callocn(size_t n, size_t size, char *name)
{
        void* ret = mallocn(n * size, name);
        if (ret == NULL) {
                return NULL;
        }
        mm_header* header = (mm_header*) ((uint32)ret - sizeof(mm_header));
        bzero(ret, n * size);
        return ret;
}

/**
 * Allocates space for n elements of the same size.
 *
 * @param n     number of elements
 * @param size  size of each element
 */
void* calloc(size_t n, size_t size)
{
        return callocn(n, size, "noname");
}

/**
 * Frees a memory block.
 *
 * @param start pointer to the start of the block that shall be freed
 */
void free(void *start)
{
#ifdef MEM_FAILSAFE
        return;
#endif
        //dprintf("free 0x%x\n", start);
        /*mm_header *this = (mm_header*) ((uint32)start - sizeof(mm_header));
        // check if there is a valid mm_header structure at start
        if ((this->prev)->next == this) {
                (this->prev)->next = this->next;
                (this->next)->prev = this->prev;
                return;
        }
        dprintf("ERROR: free(): attempt to free unallocated block 0x%x\n", start);*/
        
        //return;
        return heap_free(start, kernel_heap);
}

/**
 * Reallocates a memory block to 'size' bytes.
 * ATTENTION: if realloc fails (i.e. there is not enough free space), the return value is
 * (void*) NULL. So this should always be tested!
 * Especially realloc() shouldn't be used like this:
 * @code
 * int* p = malloc(50);
 * p = realloc(100);
 * @endcode
 * In this case, if realloc fails, p is overwritten by (void*) NULL. So, the memory allocated
 * for p is no longer accessible, which means that the data stored in p is lost and the memory
 * allocated for p can't be used any more.
 *
 * @param pointer       pointer to the old allocated space
 * @param size          the new size
 * @return              pointer to the reallocated space
 */
void* realloc(void *pointer, size_t size)
{
        mm_header* hdr = (mm_header*)(pointer - sizeof(mm_header));
        
        // the free space after pointer is big enough for the new size
        if ((uint32)hdr->next >= ((uint32) pointer + size)) {
                hdr->size = size;
                return pointer;
        }
        
        void* new = mallocn(size, hdr->name);
        
        if (new == NULL) {
                return NULL;
        }
        
        memmove(new, pointer, hdr->size);
        free(pointer);
        
        return new;
}

void mem_dump()
{
        heap_mem_dump();
}        
/**
 * Function to return the free memory space.
 *
 * @return      free memory space in bytes
 */
uint32 free_memory()
{
        /*mm_header *ptr;
        uint32 free = 0;
        
        for (ptr = mm_start->next; ptr != mm_end->next; ptr = ptr->next) {
                free += (uint32)ptr - ((uint32)ptr->prev + sizeof(mm_header) + (ptr->prev)->size);
        }

        return free;*/
}
