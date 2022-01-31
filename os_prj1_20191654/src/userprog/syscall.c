#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"


static void syscall_handler (struct intr_frame *);

void validate(void * addr){
	if(!is_user_vaddr(addr) || (is_kernel_vaddr(addr))) exit(-1);
	return;
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void halt(){
	shutdown_power_off();
}

void exit(int status){
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
	if (fd==0){
		return input_getc();		
	}
	return -1;
}

int write(int fd, const void *buffer, unsigned size) {
	if (fd == 1) {
		putbuf(buffer, size);
		return size;
  	}
 	return -1;
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
    	break;
    case SYS_REMOVE:
     	break;
    case SYS_OPEN:
    	break;
    case SYS_FILESIZE:
    	break;
    case SYS_READ:
	validate(f->esp+20); validate(f->esp+24); validate(f->esp+28);
        f->eax = write((int)*(uint32_t*)(f->esp+20), (void*)*(uint32_t*)(f->esp+24),(unsigned)*(
uint32_t*)(f->esp+28));
	break;
    case SYS_WRITE:
	validate(f->esp+20); validate(f->esp+24); validate(f->esp+28);
	f->eax = write((int)*(uint32_t*)(f->esp+20), (void*)*(uint32_t*)(f->esp+24),(unsigned)*(uint32_t*)(f->esp+28));
	break;
    case SYS_SEEK:
    	break;
    case SYS_TELL:
    	break;
    case SYS_CLOSE:
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
