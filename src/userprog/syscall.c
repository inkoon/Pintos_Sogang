#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include <user/syscall.h>

static void syscall_handler (struct intr_frame *);

struct lock file_lock;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{
	int i;
	int *args = (int*)(f->esp);
	uint32_t *pd = thread_current()->pagedir;
	for(i = 0; i < 4; i++)
			if(args == NULL || !is_user_vaddr(args) || pagedir_get_page(pd,args) == NULL)
					exit(-1);
  //printf("syscall : %d : %d\n",args[0],thread_current()->tid);
	//hex_dump(f->esp,f->esp,100,1);
	switch(args[0]){
		case SYS_HALT:
				halt();
				break;
		case SYS_EXIT:
				if(is_user_vaddr((void*)args[1])){
					exit(args[1]);
				}
				else
					exit(-1);
				break;
		case SYS_EXEC:
				if(is_user_vaddr((void*)args[1])){
					f->eax = exec((char*)args[1]);	
				}
				else{
					f->eax = -1;
					exit(-1);
				}
				break;
		case SYS_WAIT:
				f->eax = wait((pid_t)args[1]);
				break;
		case SYS_CREATE:
				if((const char *)args[1] != NULL)
					f->eax = create((const char *)args[1],(unsigned)args[2]);
				else
					exit(-1);
				break;
		case SYS_REMOVE:
				f->eax = remove((const char *)args[1]);
				break;
		case SYS_OPEN:
				if((const char *)args[1] != NULL)
					f->eax = open((const char *)args[1]);
				else
					exit(-1);
				break;
		case SYS_FILESIZE:
				f->eax = filesize(args[1]);
				break;
		case SYS_READ:	
				if(is_user_vaddr((void*)args[2]))
					f->eax = read(args[1],(void*)args[2],(unsigned)args[3]);
				else
					exit(-1);
				break;
		case SYS_WRITE:
				if(is_user_vaddr((void*)args[2]))
					f->eax = write(args[1],(void*)args[2],(unsigned)args[3]);
				else
					exit(-1);
				break;
		case SYS_SEEK:
				seek(args[1],(unsigned)args[2]);
				break;
		case SYS_TELL:
				f->eax = tell(args[1]);
				break;
		case SYS_CLOSE:
				close(args[1]);
				break;
		case SYS_PIBONACCI:
				f->eax = pibonacci(args[1]);
				break;
		case SYS_SUM_OF_FOUR_INTEGERS:
				f->eax = sum_of_four_integers(args[1], args[2], args[3], args[4]);
				break;
	}
}

/* Project 1 */
void halt(void){
	shutdown_power_off();
}

void exit(int status){
	char *ptr;
	struct thread *t = thread_current();
	struct thread *p = t->parent;
	int i;

	p->exit_status = status;
	
	printf("%s: exit(%d)\n",strtok_r((char*)thread_name()," ",&ptr),status);

	for(i=3; i<128; i++)
			if(t->file[i] != NULL)
					close(i);

	sema_up(&p->wait);
	thread_exit();
}

pid_t exec(const char *cmd_line){
	struct thread *t = NULL;
	tid_t tid = process_execute(cmd_line);
	t = find_thread(tid);
	if(t == NULL)
		return -1;
	return (pid_t)tid;
}

int wait(pid_t pid){	
	return process_wait((tid_t)pid);
}

bool create(const char *file, unsigned initial_size){
	return filesys_create(file,initial_size);
}

bool remove(const char *file){
	return filesys_remove(file);
}

int open(const char *file){
	lock_acquire(&file_lock);
	struct file* f = filesys_open(file);
	lock_release(&file_lock);
	struct thread *t = thread_current();
	int i,rval = -1;
	char name_copy[128],*temp;
	if(f == NULL)
			rval = -1;
	else{
		for(i = 2; i < 128; i++){
			if(t->file[i] == NULL){
				strlcpy(name_copy,t->name,sizeof t->name);
				if(!strcmp(strtok_r(name_copy," ",&temp),file))
					file_deny_write(f);
				t->file[i] = f;
				rval = i;
				break;
			}
		}
	}
	
	return rval;
}

int filesize(int fd){
	if(thread_current()->file[fd] == NULL)
			return 0;
	return file_length(thread_current()->file[fd]);
}

int read(int fd, void *buffer, unsigned size){
	int i,rval = -1;
	char *buf = (char*)buffer;
	if(fd == 0){
		for(i = 0; i < (int)size; i++)
			if((buf[i] = (char)input_getc()) == '\0')
				break;
		rval = i;
	}
	else if(fd >= 2){
		if(thread_current()->file[fd] != NULL){
			lock_acquire(&file_lock);
			rval = file_read(thread_current()->file[fd],buffer,size);
			lock_release(&file_lock);
		}
	}

	return rval;
}

int write(int fd, const void *buffer, unsigned size){
	int rval = -1;
	struct thread *t = thread_current();
	if(fd == 1){
		putbuf(buffer, size);
		rval = size;
	}
	else if(fd >= 2){
		if(t->file[fd] == NULL){
			rval = -1;
		}
		else{
			lock_acquire(&file_lock);
			if(check_deny(t->file[fd])){
				file_deny_write(t->file[fd]);
				rval = file_write(t->file[fd],buffer,size);
				file_allow_write(t->file[fd]);
			}
			else
				rval = file_write(t->file[fd],buffer,size);
			lock_release(&file_lock);
		}
	}
	return rval;
}

void seek(int fd, unsigned position){
	if(thread_current()->file[fd] == NULL)
			return;
	file_seek(thread_current()->file[fd],position);
}

unsigned tell(int fd){
	if(thread_current()->file[fd] == NULL)
			return 0;
	return file_tell(thread_current()->file[fd]);
}

void close(int fd){
	if(thread_current()->file[fd] == NULL)
		exit(-1);
	file_close(thread_current()->file[fd]);
	thread_current()->file[fd] = NULL;
}

int pibonacci(int n){
	return n < 3 ? 1 : pibonacci(n - 1) + pibonacci(n - 2);
}

int sum_of_four_integers(int a, int b, int c, int d){
	return a + b + c + d;
}
