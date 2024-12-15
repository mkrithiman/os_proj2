#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

/* Exit status constants */
const int CLOSE_ALL = -1;
const int ERROR = -1;
const int NOT_LOADED = 0;
const int LOAD_SUCCESS = 1;
const int LOAD_FAIL = 2;

/* Filesystem lock */
struct lock filesys_lock;

/* Function prototypes */
static void syscall_handler(struct intr_frame *f);
static void load_arg(struct intr_frame *f, int *arg, int n);
static int number_of_args(int syscall_code);

/* Syscall initialization */
void syscall_init(void) {
    lock_init(&filesys_lock);
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* Main syscall handler */
static void syscall_handler(struct intr_frame *f) {
    int arg[3];
    int esp = (int)f->esp;

    /* Verify the stack pointer */
    verify_ptr((const void *)esp);

    int syscall_code = *(int *)esp;
    int number_of_arg = number_of_args(syscall_code);

    /* Load arguments from the stack */
    load_arg(f, arg, number_of_arg);

    /* Dispatch the syscall using the handler mapping in syscall_handlers.c */
    call_syscall_handler(syscall_code, f, arg);
}

/* Load arguments from the stack */
static void load_arg(struct intr_frame *f, int *arg, int n) {
    for (int i = 0; i < n; i++) {
        arg[i] = *((int *)f->esp + i + 1);
    }
}

/* Determine the number of arguments for a syscall */
static int number_of_args(int syscall_code) {
    switch (syscall_code) {
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
        case SYS_CREATE:
        case SYS_SEEK:
            return 2;
        case SYS_READ:
        case SYS_WRITE:
            return 3;
        default:
            return 0;
    }
}


/* Convert user virtual address to kernel virtual address */
int conv_vaddr(const void *vaddr) {
    verify_ptr(vaddr);
    void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
    if (ptr != NULL) {
        return (int)(uintptr_t)ptr; /* Use uintptr_t for pointer-to-int conversion */
    } else {
        exit(ERROR);
    }
}

/* check the validity of given address */
void verify_ptr (const void *vaddr){
	if ( vaddr < (void *)0x08048000 || vaddr >= PHYS_BASE ){
		exit(ERROR);
	}
}

/* check the validity of given buffer with specific size */
void verify_buffer (void* buffer, unsigned size){
	unsigned i = 0;
	char* temp = (char *) buffer;
	while( i < size ){
		verify_ptr((const void*) temp++);
		i++;
	}
}

/* check the validity of the given string */
void verify_str (const void* str){
	char* toCheck = *(char*)conv_vaddr(str);
	for( toCheck; toCheck != 0; toCheck = *(char*)conv_vaddr(++str));
}
