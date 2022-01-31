#include "vm/file.h"
#include "vm/page.h"
#include "vm/swap.h"
#include "devices/block.h"
#include "threads/synch.h"
#include "lib/kernel/bitmap.h"

struct lock swap_lock;
struct bitmap *swap_bitmap;
struct block *swap_block;

void swap_init(void)
{
  lock_init(&swap_lock);
  swap_block = block_get_role(BLOCK_SWAP);
	if(swap_block != NULL){
	  swap_bitmap = bitmap_create(block_size(swap_block) / 8 );
  }
}
void swap_in(size_t used_index, void* kaddr)
{
	lock_acquire(&swap_lock);
  int flag=0;
	for(int i=0; i<8; i++)
	{
    if(flag==0) flag=1;
    else flag=0;
		block_read(swap_block, used_index * 8 + i, (uint8_t *)kaddr + i * BLOCK_SECTOR_SIZE);
	}
	bitmap_flip(swap_bitmap, used_index);
	lock_release(&swap_lock);
}
size_t swap_out(void *kaddr)
{
	lock_acquire(&swap_lock);
	size_t slot_num = bitmap_scan_and_flip(swap_bitmap, 0, 1, SWAP_FREE);
	if(slot_num == BITMAP_ERROR) return BITMAP_ERROR;
	for(int i=0; i<8; i++)
	{
		block_write(swap_block, slot_num * 8 + i, (uint8_t *)kaddr + i * BLOCK_SECTOR_SIZE);
	}
	lock_release(&swap_lock);
	return slot_num;
}
