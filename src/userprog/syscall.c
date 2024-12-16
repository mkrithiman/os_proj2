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
#include "userprog/syscall.h"

/* Exit status constants */
const int CLOSE_ALL = -1;
const int ERROR = -1;
const int NOT_LOADED = 0;
const int LOAD_SUCCESS = 1;
const int LOAD_FAIL = 2;

/* Filesystem lock */
struct lock filesys_lock;

/* Syscall usage metrics */
static int syscall_usage[SYS_IPC_RECEIVE + 1] = {0}; // Tracks usage count of each syscall

/* Function prototypes */
static void syscall_handler(struct intr_frame *f);
static void load_syscall_args(struct intr_frame *f, int *arg, int n);
static int num_syscall_args(int syscall_code);
static void log_syscall(const char *syscall_name, int *args, int arg_count, int result);
static void track_syscall_usage(int syscall_code);

/* Syscall initialization */
void syscall_init(void) {
    lock_init(&filesys_lock);
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* Main syscall handler */
static void syscall_handler(struct intr_frame *f) {
    int arg[3];
    int *esp = (int *)f->esp;

    /* Verify the stack pointer */
    if (!is_valid_pointer((void *)esp)) {
        terminate_process(ERROR);
    }

    int syscall_code = *esp;
    int arg_count = num_syscall_args(syscall_code);

    /* Track syscall usage */
    track_syscall_usage(syscall_code);

    /* Load syscall arguments from the stack */
    load_syscall_args(f, arg, arg_count);

    /* Dispatch the syscall using the handler mapping in syscall_handlers.c */
    call_syscall_handler(syscall_code, f, arg);
}

/* Load syscall arguments from the stack */
static void load_syscall_args(struct intr_frame *f, int *arg, int n) {
    for (int i = 0; i < n; i++) {
        int *esp = (int *)f->esp + i + 1;
        if (!is_valid_pointer((void *)esp)) {
            terminate_process(ERROR);
        }
        arg[i] = *esp;
    }
}

/* Determine the number of arguments for a syscall */
static int num_syscall_args(int syscall_code) {
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
int convert_user_vaddr(const void *vaddr) {
    if (!is_valid_pointer(vaddr)) {
        terminate_process(ERROR);
    }

    void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
    if (ptr == NULL) {
        terminate_process(ERROR);
    }
    return (int)(uintptr_t)ptr; // Safely cast pointer to int
}

/* Validate a user-provided pointer */
bool is_valid_pointer(const void *vaddr) {
    return (vaddr != NULL &&
            is_user_vaddr(vaddr) &&
            pagedir_get_page(thread_current()->pagedir, vaddr) != NULL);
}

/* Validate a user-provided buffer */
void validate_buffer(void *buffer, unsigned size) {
    char *buf = (char *)buffer;
    for (unsigned i = 0; i < size; i++) {
        if (!is_valid_pointer((const void *)&buf[i])) {
            terminate_process(ERROR);
        }
    }
}

/* Validate a user-provided string */
void validate_string(const void *str) {
    for (const char *s = (const char *)str; is_valid_pointer(s) && *s != '\0'; s++) {
        // No action needed, just validate each byte
    }
}

/* Log syscall usage with arguments and result */
static void log_syscall(const char *syscall_name, int *args, int arg_count, int result) {
    printf("[SYSCALL] %s(", syscall_name);
    for (int i = 0; i < arg_count; i++) {
        printf("%d", args[i]);
        if (i < arg_count - 1) {
            printf(", ");
        }
    }
    printf(") -> %d\n", result);
}

/* Track syscall usage metrics */
static void track_syscall_usage(int syscall_code) {
    if (syscall_code >= 0 && syscall_code <= SYS_IPC_RECEIVE) {
        syscall_usage[syscall_code]++;
    }
}



