#include "userprog/syscall.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "userprog/pagedir.h"

/* Syscall numbers */
#define SYS_HALT 0
#define SYS_EXIT 1
#define SYS_CREATE 2
#define SYS_SEEK 3
#define SYS_READ 4
#define SYS_WRITE 5
#define SYS_IPC_SEND 6
#define SYS_IPC_RECEIVE 7

/* Convert user virtual address to kernel virtual address. */
int conv_vaddr(const void *vaddr) {
    verify_ptr(vaddr);
    void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
    if (ptr != NULL) {
        return (int)(uintptr_t)ptr; /* Use uintptr_t for pointer-to-int conversion */
    } else {
        exit(SYSCALL_ERROR);
    }
}

/* Verify the validity of a pointer. */
void verify_ptr(const void *vaddr) {
    if (vaddr < (void *)0x08048000 || vaddr >= PHYS_BASE) {
        exit(SYSCALL_ERROR);
    }
}

/* Verify the validity of a buffer. */
void verify_buffer(void *buffer, unsigned size) {
    unsigned i = 0;
    char *temp = (char *)buffer;
    while (i < size) {
        verify_ptr((const void *)temp++);
        i++;
    }
}

/* Verify the validity of a string. */
void verify_str(const void *str) {
    char *toCheck = *(char *)conv_vaddr(str);
    for (toCheck; toCheck != 0; toCheck = *(char *)conv_vaddr(++str));
}

/*
 *  13 syscall functions
 *
 */

void halt (void){
	shutdown_power_off();
}

// void exit(int status) {
//     struct thread *cur = thread_current();
    
//     /* Ensure the parent-child structure is updated correctly. */
//     if (cur->status != THREAD_DYING && cur->cp) {
//         cur->cp->status = status;
//     }
    
//     printf("%s: exit(%d)\n", cur->name, status);
//     thread_exit();  // Terminate the current thread
// }
void exit(int status) {
    struct thread *cur = thread_current(); // Get the current thread

    // Debugging log
    printf("%s: exit(%d)\n", cur->name, status);

    /* Update the exit status of the current process */
    if (cur->cp != NULL) {
        cur->cp->status = status; // Update the status for parent-child tracking
        cur->cp->exit = true;   // Mark as exited
    }

    /* Close all open files for the thread */
    process_close_file(CLOSE_ALL, cur);

    /* Release any held resources (if required) */
    // Example: clean up locks, memory, etc. if needed

    thread_exit(); // Terminate the current thread
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