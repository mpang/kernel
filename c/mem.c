/* mem.c : memory manager
 */

#include <xeroskernel.h>
#include <i386.h>

/* Your code goes here */
// to make the address align with 16 bit
#define PARAGRAPH_MASK (~(0xf))

void defrag(void);

extern long freemem;
extern char *maxaddr;

// memory list contains free blocks, starting with 2 blocks
// one before and one after the hole
// should always point at the start of the list
struct mem_header *mem_list;

void kmeminit(void)
{
  // one before hole
  mem_list = (struct mem_header *) (freemem & PARAGRAPH_MASK);
  mem_list->size = HOLESTART - (unsigned int) mem_list;
  mem_list->previous = 0;

  // one after hole
  struct mem_header *after_hole = (struct mem_header *) HOLEEND;
  after_hole->size = (unsigned int) maxaddr - HOLEEND;
  after_hole->previous = mem_list;
  after_hole->next = 0;
  mem_list->next = after_hole;  // link together

  kprintf("memory manager initialized\n");
}

void *kmalloc(int size)
{
  if (size <= 0) {
    return 0;
  }

  int amount = size / 16 + ((size % 16) ? 1 : 0);
  amount = amount * 16 + sizeof(struct mem_header); // include mem header

  // loop through free mem list until find a chunk large enough
  struct mem_header *available_slot = 0;
  for (available_slot = mem_list; available_slot && available_slot->size < amount; available_slot = available_slot->next);

  if (!available_slot) {
    return 0;
  }

  // split the block if available block is larger than what is requested
  // the exceeded portion has to be larger than the size of mem_header
  if (available_slot->size > amount + sizeof(struct mem_header)) {
    struct mem_header *remaining_free_slot = (struct mem_header *) ((unsigned int) available_slot + amount);
    // set the size here
    remaining_free_slot->size = available_slot->size - amount;
    available_slot->size = amount;
    
    // insert the newly splitted block into the list, after the allocated block
    remaining_free_slot->previous = available_slot;
    remaining_free_slot->next = available_slot->next;
    if (available_slot->next) {
      available_slot->next->previous = remaining_free_slot;
    }
    available_slot->next = remaining_free_slot;
  }

  // adjust free memory list
  if (available_slot->previous) {
    available_slot->previous->next = available_slot->next;  
  }
  
  if (available_slot->next) {
    available_slot->next->previous = available_slot->previous;
  }

  // adjust list header
  if (mem_list == available_slot) {
    mem_list = available_slot->next;
  }

  // fill in the fields and return the memory address
  available_slot->previous = 0;
  available_slot->next = 0;
  return available_slot->data_start;
}

void kfree(void *pointer)
{
  struct mem_header *header = (struct mem_header *) ((unsigned int) pointer - sizeof(struct mem_header));
  // memory block is before the first free memory segment in the list
  if ((unsigned int) header < (unsigned int) mem_list) {
    mem_list->previous = header;
    header->next = mem_list;
    header->previous = 0;
    mem_list = header;
  } else {
    struct mem_header *free_block;
    for (free_block = mem_list; free_block->next; free_block = free_block->next) {
      if ((unsigned int) header > (unsigned int) free_block && (unsigned int) header < (unsigned int) free_block->next) {
        break;
      }
    }

    // insert between free_block and free_block->next
    if (free_block->next) {
      header->next = free_block->next;
      header->previous = free_block;
      free_block->next->previous = header;
      free_block->next = header;
    } else {
      // insert at the end of the list
      free_block->next = header;
      header->previous = free_block;
      header->next = 0;   
    }
  }

  defrag();
}

// defrag the free mem list
void defrag(void)
{
  struct mem_header *mem_block = mem_list;
  while (mem_block->next) {
    unsigned int mem_boundary = (unsigned int) mem_block + mem_block->size;
    // if current memory block is adjacent to the next one, merge them
    if (mem_boundary == (unsigned int) mem_block->next) {
      mem_block->size += mem_block->next->size;

      if (mem_block->next->next) {
        mem_block->next->next->previous = mem_block;
      }
      mem_block->next = mem_block->next->next;
    } else {
      // if we successfully merge two blocks, we don't want to move to the next one
      // because there's still a chance that the merged one is still adjacent to its next
      // block. Instead we only advance to the next one if we can't merge anymore.
      mem_block = mem_block->next;
    }
  }
}
