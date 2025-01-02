#define _XOPEN_SOURCE 500 // needed for sbrk() on cslab

#include <unistd.h>

typedef struct freenode
{
    size_t size;
    struct freenode *next;
} freenode;

#define HEAP_CHUNK_SIZE 4096

// head node for the freelist
freenode *freelist = NULL;

/* allocate size bytes from the heap */
void *malloc(size_t size)
{
    // can't have less than 1 byte requested
    if (size < 1)
    {
        return NULL;
    }

    // add 8 bytes for bookkeeping
    size += 8;

    // 32 bytes is the minimum allocation
    if (size < 32)
    {
        size = 32;
    }

    // round up to the nearest 16-byte multiple
    else if (size%16 != 0)
    {
        size = ((size/16)+1)*16;
    }

    // if we have no memory, grab one chunk to start
    if (freelist == NULL) {
         
        freelist = sbrk(HEAP_CHUNK_SIZE) + 8;
        if (freelist == (freenode *)-1) {
            return NULL;
        }
        freelist->size = HEAP_CHUNK_SIZE - 8;
        freelist->next = NULL;

    }

    // look for a freenode that's large enough for this request
    // have to track the previous node also for list manipulation later
 
    freenode *prev = NULL;
    freenode *curr = freelist; 
    int needmem = 0;
    while(curr->size < size) {
        //if at end, note and leave 
        if (curr->next == NULL) {
            needmem = 1;
            break;
        }
        prev = curr;
        curr = curr->next;   
    }
        

    // if there is no freenode that's large enough, allocate more memory
    if (needmem) {
        int allocate_size = ((size/HEAP_CHUNK_SIZE)+1)*HEAP_CHUNK_SIZE; 
        
        freenode *new = sbrk(allocate_size) + 8;  
            if (new == (void *)-1) {
                return NULL;
            }
        new->size = allocate_size - 8;
        new->next = freelist->next;
        freelist = new;
        // change curr and prev to be at the front
        curr = freelist;
        prev = NULL;   
    }

    // here, should have a freenode with enough space for the request
    // - if there would be less than 32 bytes left, then return the entire chunk
    // - if there are remaining bytes, then break up the chunk into two pieces
    //     return the front of this memory chunk to the user
    //     and put the rest into the freelist 
  
    //chunk too big
    if (curr->size - size >= 32) {
        //split node and rehook
        freenode *new = (freenode *)((void *)curr + size);
        new->next = curr->next;
        curr->next = new;
        //fix sizes
        new->size = curr->size - size;
        curr->size = size;   
    }

    // here, get the address for the chunk being returned to the user and return it
    void *retpo = (void *)curr + 8;
    
    //got to fix the freelist before returning
    
    //if we at the front, then just shift the head
    if (curr == freelist) {
        freelist = curr->next;
    } else {
        //otherwise, excise node
        prev->next = curr->next;
    }

    return retpo;
}

/* return a previously allocated memory chunk to the allocator */
void free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    // make a new freenode starting 8 bytes before the provided address
    freenode *new_node = (freenode *)(ptr-8);

    // the size is already in memory at the right location (ptr-8)

    // add this memory chunk back to the beginning of the freelist
    new_node->next = freelist;
    freelist = new_node;

    return;
}
