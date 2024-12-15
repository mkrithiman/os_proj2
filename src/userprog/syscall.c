#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/thread.h"

/* exit status, shared with process.c */
const int CLOSE_ALL = -1;
const int ERROR = -1;
const int NOT_LOADED = 0;
const int LOAD_SUCCESS = 1;
const int LOAD_FAIL = 2;

/* boolean flag from thread.c */
extern bool thread_alive;

static void syscall_handler (struct intr_frame *);

/* function header added for project 2 */

static void load_arg (struct intr_frame *f, int *arg, int n);
static int number_of_args(int syscall_code);

static int conv_vaddr(const void *vaddr);
static void verify_ptr (const void *vaddr);
static void verify_buffer (void* buffer, unsigned size);
static void verify_str (const void* str);

void
syscall_init (void) 
{
	lock_init(&filesys_lock);
	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	int arg[3];
	int esp = conv_vaddr((const void*) f->esp);
	int syscall_code = *(int*) esp;
	int number_of_arg = number_of_args(syscall_code);

	/* categorize the syscall with respect to how many arguments they need */
	switch(number_of_arg){
		case 1:
			load_arg(f, &arg[0], number_of_arg );
			switch( syscall_code ){

			case SYS_EXIT:

				exit(arg[0]);
				break;
			case SYS_IPC_SEND:
            	verify_str((const void *) arg[0]);
            	ipc_send((const char *) arg[0]);
            	break;

        	case SYS_IPC_RECEIVE:
            	verify_buffer((void *) arg[0], (unsigned) arg[1]);
            	ipc_receive((char *) arg[0], (size_t) arg[1]);
            	break;

			case SYS_EXEC:

				verify_str((const void *) arg[0]);
				arg[0] = conv_vaddr((const void *) arg[0]);
				f->eax = exec((const char *) arg[0]);
				break;

			case SYS_WAIT:

				f->eax = wait(arg[0]);
				break;

			case SYS_REMOVE:

				verify_str((const void *) arg[0]);
				arg[0] = conv_vaddr((const void *) arg[0]);
				f->eax = remove((const char *) arg[0]);
				break;

			case SYS_OPEN:

				verify_str((const void *) arg[0]);
				arg[0] = conv_vaddr((const void *) arg[0]);
				f->eax = open((const char *) arg[0]);
				break;

			case SYS_FILESIZE:

				f->eax = filesize(arg[0]);
				break;

			case SYS_TELL:

				f->eax = tell(arg[0]);
				break;

			case SYS_CLOSE:

				close(arg[0]);
				break;
			}
			break;

		case 2:
			load_arg(f, &arg[0], number_of_arg );
			switch( syscall_code ){
			case SYS_CREATE:

				verify_str((const void *) arg[0]);
				arg[0] = conv_vaddr((const void *) arg[0]);
				f->eax = create((const char *)arg[0], (unsigned) arg[1]);
				break;

			case SYS_SEEK:

				seek(arg[0], (unsigned) arg[1]);
				break;
			}

		break;

		case 3:
			load_arg(f, &arg[0], number_of_arg );
			verify_buffer((void *) arg[1], (unsigned) arg[2]);
			arg[1] = conv_vaddr((const void *) arg[1]);
			switch( syscall_code ){
			case SYS_READ:

				f->eax = read(arg[0], (void *) arg[1], (unsigned) arg[2]);
				break;

			case SYS_WRITE:

				f->eax = write(arg[0], (const void *) arg[1], (unsigned) arg[2]);
				break;
			}

		break;

		default:
			halt();
			break;
	}
}

/*
 *
 * helper functions for system call
 *
 */

/* get parsed arguments */
static void load_arg (struct intr_frame *f, int *arg, int n){
	int i = 0;
	while(i < n){
		int* ptr = (int*) f->esp + i + 1;
		verify_ptr((const void*) ptr);
		arg[i++] = *ptr;
	}
}

/* number of argument need to be add */
static int number_of_args(int syscall_code){
	switch (syscall_code){
	case SYS_HALT:
	case SYS_EXIT:
	case SYS_EXEC:
	case SYS_WAIT:
	case SYS_REMOVE:
	case SYS_OPEN:
	case SYS_FILESIZE:
	case SYS_TELL:
	case SYS_CLOSE:
		return 1;

	case SYS_SEEK:
	case SYS_CREATE:
		return 2;

	case SYS_READ:
	case SYS_WRITE:
		return 3;
	default:
        return 0; /* Default for unknown syscall codes */

	}
}

/* Send data via IPC. */
void ipc_send(const char *message) {
    sema_down(&shared_ipc_buffer.sema); /* Acquire semaphore */

    size_t i = 0;
    while (i < IPC_BUFFER_SIZE - 1 && message[i] != '\0') {
        shared_ipc_buffer.data[i] = message[i];
        i++;
    }
    shared_ipc_buffer.data[i] = '\0'; /* Null-terminate */

    sema_up(&shared_ipc_buffer.sema); /* Release semaphore */
}


/* Receive data via IPC. */
void ipc_receive(char *buffer, size_t size) {
    sema_down(&shared_ipc_buffer.sema); /* Acquire semaphore */

    size_t i = 0;
    while (i < size - 1 && shared_ipc_buffer.data[i] != '\0') {
        buffer[i] = shared_ipc_buffer.data[i];
        i++;
    }
    buffer[i] = '\0'; /* Null-terminate */

    sema_up(&shared_ipc_buffer.sema); /* Release semaphore */
}

/* convert given user virtual address to kernel virtual address */
static int conv_vaddr(const void *vaddr) {
    verify_ptr(vaddr);
    void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
    if (ptr != NULL) {
        return (int)(uintptr_t)ptr; /* Use uintptr_t for pointer-to-int conversion */
    } else {
        exit(ERROR);
    }
}


/* check the validity of given address */
static void verify_ptr (const void *vaddr){
	if ( vaddr < (void *)0x08048000 || vaddr >= PHYS_BASE ){
		exit(ERROR);
	}
}

/* check the validity of given buffer with specific size */
static void verify_buffer (void* buffer, unsigned size){
	unsigned i = 0;
	char* temp = (char *) buffer;
	while( i < size ){
		verify_ptr((const void*) temp++);
		i++;
	}
}

/* check the validity of the given string */
static void verify_str (const void* str){
	char* toCheck = *(char*)conv_vaddr(str);
	for( toCheck; toCheck != 0; toCheck = *(char*)conv_vaddr(++str));
}

/*
 *  13 syscall functions
 *
 */

void halt (void){
	shutdown_power_off();
}

void exit (int status){
	struct thread *cur = thread_current();
	enum intr_level old_level = intr_disable();
	thread_foreach(is_alive_func, (void *) cur->parent);
	if (thread_alive && cur->cp){
		cur->cp->status = status;
	}
	reset_flag();
	intr_set_level(old_level);
	printf ("%s: exit(%d)\n", cur->name, status);
	thread_exit();
}

int exec (const char *cmd_line){
	int pid = process_execute(cmd_line);
	struct child_process* cp = get_child_process(pid, thread_current());
	if (cp != NULL){
		if (cp->load == NOT_LOADED){
			sema_down(&cp->load_sema);
		}
		if (cp->load == LOAD_FAIL){
			remove_child_process(cp);
			return ERROR;
		}
	}else{
		return ERROR;
	}

	return pid;
}

int wait (pid_t pid){
	return process_wait(pid);
}

bool create (const char *file, unsigned initial_size){
	lock_acquire(&filesys_lock);
	bool success = filesys_create(file, initial_size);
	lock_release(&filesys_lock);
	return success;
}

bool remove (const char *file){
	lock_acquire(&filesys_lock);
	bool success = filesys_remove(file);
	lock_release(&filesys_lock);
	return success;
}

int open (const char *file){
	struct file *f = filesys_open(file);
	int to_return;
	lock_acquire(&filesys_lock);
	to_return = f != NULL ? process_add_file(f, thread_current()) : ERROR;
	lock_release(&filesys_lock);
	return to_return;
}

int filesize (int fd){
	struct file *f = process_get_file(fd, thread_current());
	int to_return;
	lock_acquire(&filesys_lock);
	to_return = f != NULL ? file_length(f) : ERROR;
	lock_release(&filesys_lock);
	return to_return;

}

int read (int fd, void *buffer, unsigned size){
	if (fd != STDIN_FILENO){
		struct file *f = process_get_file(fd, thread_current());
		if (f != NULL){
			lock_acquire(&filesys_lock);
			int bytes = file_read(f, buffer, size);
			lock_release(&filesys_lock);
			return bytes;
		}else{
			return ERROR;
		}
	}else{
		uint8_t* temp = (uint8_t *) buffer;
		unsigned i = 0;
		while(i < size){
			temp[i++] = input_getc();
		}
		return size;
	}


}

int write (int fd, const void *buffer, unsigned size){
	if (fd != STDOUT_FILENO){
		struct file *f = process_get_file(fd, thread_current());
		if (f != NULL){
			lock_acquire(&filesys_lock);
			int bytes = file_write(f, buffer, size);
			lock_release(&filesys_lock);
			return bytes;
		}else{
			return ERROR;
		}

	}else{
		putbuf(buffer, size);
		return size;
	}
}

void seek (int fd, unsigned position){
	struct file *f = process_get_file(fd, thread_current());
	if (f != NULL && (off_t)position >= 0){
		lock_acquire(&filesys_lock);
		file_seek(f, position);
		lock_release(&filesys_lock);
	}

}

unsigned tell (int fd){
	struct file *f = process_get_file(fd, thread_current());
	off_t to_return;
	lock_acquire(&filesys_lock);
	to_return = f != NULL ? file_tell(f) : ERROR;
	lock_release(&filesys_lock);
	return to_return;

}

void close (int fd){
	lock_acquire(&filesys_lock);
	process_close_file(fd, thread_current());
	lock_release(&filesys_lock);
}