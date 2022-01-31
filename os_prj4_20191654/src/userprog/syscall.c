#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <string.h>
#include "vm/page.h"

static void syscall_handler (struct intr_frame *);

void check_address(void *addr, void *esp)
{
	if(addr < (void*)0x8048000 || addr >= (void*)0xc0000000)
	{
      exit(-1);
  }

	struct vm_entry * vme = find_vme(addr);
	if(vme != NULL) return;
	
	if(addr >= esp-32){
		if(expand_stack(addr) == false) exit(-1);
	}
	else exit(-1);	
}
void check_valid_buffer(void *buffer, unsigned size, void *esp, bool to_write)
{
	for(int i=0; i<size; i++)
	{
		check_address(buffer, esp);
		struct vm_entry * vme = find_vme(buffer);
		if(vme == NULL || vme->writable == false) exit(-1);
		buffer++;
	}
}
void check_valid_string(const void *str, void *esp)
{
  char* temp_str = str;
	while(1)
	{
		check_address(temp_str, esp);
    if(*temp_str == 0) break;
    temp_str++;
	}
}


//

void
get_argument(void *esp, int *arg, int count)
{
	void *stack_pointer=esp+4;
	if(count > 0)
	{
		for(int i=0; i<count; i++){
			check_address(stack_pointer, esp);
			arg[i] = *(int *)stack_pointer;
			stack_pointer = stack_pointer + 4;
		}
	}
}



void unpin_ptr(void *vaddr)
{
	struct vm_entry *vme  = find_vme(vaddr);
	if(vme != NULL)
	{
		vme->pinned = false;
	}
}

void unpin_string(void *str)
{
	unpin_ptr(str);
	while(*(char *)str != 0)
	{
		str = (char *)str + 1;
		unpin_ptr(str);
	}
}

void unpin_buffer(void *buffer, unsigned size)
{
	int i;
	char *local_buffer = (char *)buffer;
	for(i=0; i<size; i++)
	{
		unpin_ptr(local_buffer);
		local_buffer++;
	}
}
//----
void validate(void * addr){
	if(!is_user_vaddr(addr) || (is_kernel_vaddr(addr))) exit(-1);
	return;
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&lock_F);
}

void halt(){
	shutdown_power_off();
}

void exit(int status){ 
  for(int i=3;i<128;i++){
    if(thread_current()->fd[i]){
       close(i);
    }
  }
	thread_current()->exit_status = status;
	printf("%s: exit(%d)\n", thread_name(), status);
	thread_exit();
}

int exec(const char* file){
	return process_execute(file);
}

int wait(pid_t pid){
	return process_wait(pid);
}

int read(int fd, void *buffer, unsigned size){
  int res = -1;
  validate(buffer);
  lock_acquire(&lock_F);
 	if (fd==0){
    for(int i=0;i<size;i++){
      if (input_getc() == '\0') {
         res=i;
         break;
       }
    }
  } 
  else if (fd>=3&&fd<128){
    if(thread_current()->fd[fd]==NULL) {
      exit(-1);
    }
    res = file_read(thread_current()->fd[fd], buffer, size);
  }

  lock_release(&lock_F);
  return res;
}

int write(int fd, const void *buffer, unsigned size) {
  int res = -1;
  validate(buffer);
	if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  else if (fd>=3&&fd<128){
    if(thread_current()->fd[fd]==NULL) {
      exit(-1);
    }
    res = file_write(thread_current()->fd[fd], buffer, size);
  }
  return res;
}

int fibonacci(int n){
	int fibo;
	int a=0, b=1;
	if(n<2) return n;
	else {
		for(int i=0;i<n;i++){
			fibo=b;
			b+=a;
			a=fibo;
		}
	}
	return fibo; 
}

int max_of_four_int(int a, int b, int c, int d){
	int max=0;
	if((a>=b)&&(a>=c)&&(a>=d)) max=a;	
	else if((b>=a)&&(b>=c)&&(b>=d)) max=b;
	else if((c>=a)&&(c>=b)&&(c>=d)) max=c;
	else if((d>=a)&&(d>=b)&&(d>=c)) max=d;
	else max=-1;
	return max;
}


//-------------------------------------------------------//


bool create(const char *file, unsigned initial_size){
  if (file==NULL) exit(-1);
  validate(file);
  return filesys_create(file, initial_size);
}

bool remove(const char *file){
  if (file==NULL) exit(-1);
  validate(file);
  return filesys_remove(file);
}

int open(const char* file){
  int res=-1;
  if(file==NULL) return -1;
  validate(file);
  lock_acquire(&lock_F);
  struct file* f = filesys_open(file);
  if (f==NULL){
    res = -1;
  }
  else{
    for(int i=3;i<128;i++){
    if(thread_current()->fd[i]==NULL){
      if(strcmp(thread_current()->name, file)==0){
        file_deny_write(f);
      }
      thread_current()->fd[i]=f;
      res = i;
      lock_release(&lock_F);
      return res;
    }
  }
  }

  lock_release(&lock_F);
  return res;
}

int filesize(int fd){
  int size=-1;
  if(thread_current()->fd[fd]!=NULL && fd>=3){
    size=file_length(thread_current()->fd[fd]);
  }
  else{
    exit(-1);
  }
  return size;
}

void seek(int fd, unsigned position){
  if(thread_current()->fd[fd]!=NULL && fd >=3) file_seek(thread_current()->fd[fd], position);
  return;
}

unsigned tell(int fd){
  int pos = -1;
  if(thread_current()->fd[fd]!=NULL && fd >=3) pos=file_tell(thread_current()->fd[fd]);
  return pos;
}

void close(int fd){
  if(fd<=2 || fd>=128) exit(-1);
  if(thread_current()->fd[fd]==NULL) exit(-1);
  struct file *f;
  f = thread_current()->fd[fd];
  thread_current()->fd[fd]=NULL;
  file_close(f);
  return;
}


//-------------------------------------------------------//


static void
syscall_handler (struct intr_frame *f) 
{
  pid_t pid;
  int arg[5];
  void *esp = f->esp;
  check_address(esp, f->esp);
  switch(*(int*)esp){  
    case SYS_HALT:
	    halt();	
     	break;
    case SYS_EXIT:
	    validate(f->esp+4);
	    exit((int)*(uint32_t*)(f->esp + 4));
     	break;
    case SYS_EXEC:
	    get_argument(esp,arg,1);
		  check_valid_string((const void *)arg[0], f->esp);
		  f->eax = exec((const char *)arg[0]);
		  unpin_string((void *)arg[0]);
  //validate(f->esp+4);
	//f->eax = exec((const char*)*(uint32_t*)(f->esp + 4));
     	break;
    case SYS_WAIT:
	    validate(f->esp+4);
	    pid=*(pid_t*)(f->esp + 4);
	    f->eax = wait(pid);
     	break;
    case SYS_CREATE:
      validate(f->esp+4); validate(f->esp+8);
      f->eax= create((const char*)*(uint32_t*)(f->esp+4), (unsigned)*(uint32_t*)(f->esp+8));
      break;
    case SYS_REMOVE:
      validate(f->esp+4);
      f->eax = remove((const char*)*(uint32_t*)(f->esp+4));
     	break;
    case SYS_OPEN:
      get_argument(esp,arg,1);
		  check_valid_string((const void *)arg[0], f->esp);
		  f->eax = open((const char *)arg[0]);
		  unpin_string((void *)arg[0]);
      //validate(f->esp+4);
      //f->eax = open((const char*)*(uint32_t*)(f->esp+4));
    	break;
    case SYS_FILESIZE:
      validate(f->esp+4);
      f->eax = filesize((int)*(uint32_t*)(f->esp+4));
    	break;
    case SYS_READ:
      get_argument(esp,arg,3);
		  check_valid_buffer((void *)arg[1], (unsigned)arg[2], f->esp, true);
		  f->eax = read(arg[0],(void *)arg[1],(unsigned)arg[2]);
		  unpin_buffer((void *)arg[1], (unsigned) arg[2]);
		  //validate(f->esp+20); validate(f->esp+24); validate(f->esp+28);
      //f->eax = read((int)*(uint32_t*)(f->esp+20), (void*)*(uint32_t*)(f->esp+24),(unsigned)*(uint32_t*)(f->esp+28));
      break;
	  case SYS_WRITE:
		  get_argument(esp,arg,3);
		  check_valid_buffer((void *)arg[1], (unsigned)arg[2], f->esp, false);
		  f->eax = write(arg[0],(void *)arg[1],(unsigned)arg[2]);
		  unpin_buffer((void *)arg[1], (unsigned)arg[2]);    
	    //lock_acquire(&lock_F);
	    //validate(f->esp+20); validate(f->esp+24); validate(f->esp+28);
	    //f->eax = write((int)*(uint32_t*)(f->esp+20), (void*)*(uint32_t*)(f->esp+24),(unsigned)*(uint32_t*)(f->esp+28));
	    //lock_release(&lock_F);
      break;
   case SYS_SEEK:
      validate(f->esp+16); validate(f->esp+20);
      seek((int)*(uint32_t*)(f->esp+16), (unsigned)*(uint32_t*)(f->esp+20));
    	break;
    case SYS_TELL:
      validate(f->esp+4);
      f->eax = tell((int)*(uint32_t*)(f->esp+4));
    	break;
    case SYS_CLOSE:
      validate(f->esp+4);
      close((int)*(uint32_t*)(f->esp+4));
     	break;
    case SYS_MMAP:
	break;
    case SYS_MUNMAP:
	break;
    case SYS_CHDIR:
	break;
    case SYS_MKDIR:
	break;
    case SYS_READDIR:
	break;
    case SYS_ISDIR:
	break;
    case SYS_INUMBER:
	break;
    case SYS_FIBO:
	validate(f->esp+12);
	f->eax=fibonacci((int)*(uint32_t*)(f->esp+12));
	break;
    case SYS_MAX:
	validate(f->esp+28); validate(f->esp+32); validate(f->esp+36); validate(f->esp+40); validate(f->esp+44);
	f->eax=max_of_four_int((int)*(uint32_t*)(f->esp+28), (int)*(uint32_t*)(f->esp+32), (int)*(uint32_t*)(f->esp+36),(int)*(uint32_t*)(f->esp+40));
	break;
    default:
	break;	
  }
}
