#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <threads/malloc.h>
#include <threads/palloc.h>
#include "filesys/file.h"
#include "vm/page.h"
#include "vm/file.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "userprog/syscall.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "lib/kernel/list.h"

static bool vm_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED);
static unsigned vm_hash_func(const struct hash_elem *e, void *aux UNUSED);
static void vm_destroy_func(struct hash_elem *e, void *aux UNUSED);


void vm_init(struct hash *vm)
{
  hash_init(vm, vm_hash_func, vm_less_func, NULL);
}

static unsigned vm_hash_func(const struct hash_elem *e, void *aux UNUSED)
{
	struct vm_entry *vme = hash_entry(e, struct vm_entry, elem);
	return hash_int(vme->vaddr);
}
static bool vm_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  struct vm_entry *vme_a = hash_entry(a, struct vm_entry, elem);
  struct vm_entry *vme_b = hash_entry(b, struct vm_entry, elem);
  return (vme_a->vaddr < vme_b->vaddr);
}
bool insert_vme(struct hash *vm, struct vm_entry *vme)
{
  if(hash_insert(vm, &vme->elem) == NULL) return true;
  else return false;
}

bool delete_vme(struct hash *vm, struct vm_entry *vme)
{
  if(hash_delete(vm, &vme->elem) != NULL) {
    free(vme);
    return true;
  }
  else return false;
}
struct vm_entry *find_vme(void *vaddr)
{
  struct vm_entry vme;
  struct hash_elem *element;
  vme.vaddr = pg_round_down(vaddr);
  element = hash_find(&thread_current()->vm, &vme.elem);
  if(element==NULL) return NULL;
  return hash_entry(element, struct vm_entry, elem);
}
void vm_destroy(struct hash *vm)
{
  hash_destroy(vm, vm_destroy_func);
}
bool load_file(void *kaddr, struct vm_entry *vme)
{
  if((int)vme->read_bytes != file_read_at(vme->file, kaddr, vme->read_bytes, vme->offset))
  {
     return false;
  }
  memset(kaddr + vme->read_bytes, 0, vme->zero_bytes);
  return true;
}
static void vm_destroy_func(struct hash_elem *e, void *aux UNUSED)
{
	struct vm_entry *vme = hash_entry(e, struct vm_entry, elem);
	void *physical_address;
	if(vme->is_loaded == true)
	{
			physical_address = pagedir_get_page(thread_current()->pagedir, vme->vaddr);
		  free_page(physical_address);
			pagedir_clear_page(thread_current()->pagedir, vme->vaddr);
	}
	free(vme);
}

