#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <string.h>

struct lock lock_F;
static void syscall_handler (struct intr_frame *);

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
switch(*(uint32_t*)(f->esp)){
    case SYS_HALT:
	halt();	
     	break;
    case SYS_EXIT:
	validate(f->esp+4);
	exit((int)*(uint32_t*)(f->esp + 4));
     	break;
    case SYS_EXEC:
	validate(f->esp+4);
	f->eax = exec((const char*)*(uint32_t*)(f->esp + 4));
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
      validate(f->esp+4);
      f->eax = open((const char*)*(uint32_t*)(f->esp+4));
    	break;
    case SYS_FILESIZE:
      validate(f->esp+4);
      f->eax = filesize((int)*(uint32_t*)(f->esp+4));
    	break;
    case SYS_READ:
	    validate(f->esp+20); validate(f->esp+24); validate(f->esp+28);
      f->eax = read((int)*(uint32_t*)(f->esp+20), (void*)*(uint32_t*)(f->esp+24),(unsigned)*(uint32_t*)(f->esp+28));
    	break;
    case SYS_WRITE:
      lock_acquire(&lock_F);
	    validate(f->esp+20); validate(f->esp+24); validate(f->esp+28);
	    f->eax = write((int)*(uint32_t*)(f->esp+20), (void*)*(uint32_t*)(f->esp+24),(unsigned)*(uint32_t*)(f->esp+28));
	    lock_release(&lock_F);
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
