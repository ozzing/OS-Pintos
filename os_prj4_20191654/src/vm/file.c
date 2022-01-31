#include "vm/file.h"
#include "vm/swap.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "userprog/pagedir.h"
#include "filesys/file.h"
#include <threads/malloc.h>
#include <stdio.h>
#include "userprog/syscall.h"


void lru_list_init(void)
{
	list_init(&lru_list);
	lock_init(&lru_list_lock);
	lru_clock = NULL;
}
void add_page_to_lru_list(struct page *page)
{
	lock_acquire(&lru_list_lock);
	list_push_back(&lru_list, &page->lru);
	lock_release(&lru_list_lock);
}
void del_page_from_lru_list(struct page* page)
{
		if(lru_clock != page)
		{
      list_remove(&page->lru);
      return;
	  }		
    lru_clock = list_entry(list_remove(&page->lru), struct page, lru);
		return;
}
struct page *alloc_page(enum palloc_flags flags)
{
  void *kaddr = palloc_get_page(flags);
	struct page *new_page = malloc(sizeof(struct page));
	 if(new_page == NULL)
  {
    return NULL;
  }
  while(kaddr == NULL)
	{
		kaddr = palloc_get_page(flags);
	  try_to_free_pages();
  }
  new_page->kaddr = kaddr;
	new_page->thread = thread_current();
	add_page_to_lru_list(new_page);
	return new_page;
}
void free_page(void *kaddr)
{
  lock_acquire(&lru_list_lock);
	for(struct list_elem *element = list_begin(&lru_list); element != list_end(&lru_list); element = list_next(element))
	{
		struct page *lru_page = list_entry(element, struct page, lru);
		if(lru_page->kaddr == kaddr){
			__free_page(lru_page);
      break;
    }
  }
  lock_release(&lru_list_lock);
}
void __free_page(struct page *page)
{
	del_page_from_lru_list(page);
  palloc_free_page(page->kaddr);
	free(page);
}
struct list_elem* get_next_lru_clock(void)
{
	struct list_elem *element;
	if(lru_clock == NULL)
	{
		element = list_begin(&lru_list);
		if(element != list_end(&lru_list))
		{
			lru_clock = list_entry(element, struct page, lru);
			return element;
		}
		else
		{
			element = NULL;
		}
	}
	element = list_next(&lru_clock->lru);
	if(element == list_end(&lru_list))
	{
		if(&lru_clock->lru == list_begin(&lru_list))
		{
			element = NULL;
		}
		else
		{
		element = list_begin(&lru_list);
		}
	}
	lru_clock = list_entry(element, struct page, lru);
	return element;
}

void try_to_free_pages(void)
{
	lock_acquire(&lru_list_lock);
	if(list_empty(&lru_list) == false)
	{
	while(true)
	{
    struct list_elem * element = get_next_lru_clock();
    struct page * lru_page = list_entry(element, struct page, lru);
    struct thread* page_thread = lru_page->thread;
    if(pagedir_is_accessed(page_thread->pagedir, lru_page->vme->vaddr))
		{
			pagedir_set_accessed(page_thread->pagedir, lru_page->vme->vaddr, false);
			continue;
		}
		if(pagedir_is_dirty(page_thread->pagedir, lru_page->vme->vaddr) || lru_page->vme->type == VM_ANON){
      if(lru_page->vme->type == VM_BIN)
      {
        lru_page->vme->type = VM_ANON;
        lru_page->vme->swap_slot = swap_out(lru_page->kaddr);
      }
			else if(lru_page->vme->type == VM_FILE)
			{
				lock_acquire(&lock_F);
				file_write_at(lru_page->vme->file, lru_page->kaddr ,lru_page->vme->read_bytes, lru_page->vme->offset);
				lock_release(&lock_F);
			}
			else if(lru_page->vme->type == VM_ANON)
			{
				lru_page->vme->swap_slot = swap_out(lru_page->kaddr);
 			}
		}

		lru_page->vme->is_loaded = false;
		pagedir_clear_page(page_thread->pagedir, lru_page->vme->vaddr);
		__free_page(lru_page);
		break;
	}
  }
  lock_release(&lru_list_lock);
	return;
}
