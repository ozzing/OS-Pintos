#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>

#define VM_BIN 1 
#define VM_FILE 2
#define VM_ANON 3
#define CLOSE_ALL 9999
struct vm_entry{
	uint8_t type;                      	
  void *vaddr;                       
	bool writable;                     
	bool is_loaded;                    
	bool pinned;
	struct file *file;
	struct list_elem mmap_elem;        
	size_t offset;
	size_t read_bytes;                   
	size_t zero_bytes;
	size_t swap_slot;
	struct hash_elem elem;             
};
struct page{
  void *kaddr;
  struct vm_entry *vme;
  struct thread *thread;
  struct list_elem lru;
};

void vm_init(struct hash *vm);
void vm_destroy(struct hash *vm);
struct vm_entry *find_vme(void *vaddr);
bool insert_vme(struct hash *vm, struct vm_entry *vme);
bool delete_vme(struct hash *vm, struct vm_entry *vme);
struct vm_entry *find_vme(void *vaddr);
bool insert_vme(struct hash *vm, struct vm_entry *vme);
bool delete_vme(struct hash *vm, struct vm_entry *vme);
bool load_file(void *kaddr, struct vm_entry *vme);
#endif
