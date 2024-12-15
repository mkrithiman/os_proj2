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

/* Syscall numbers */
#define SYS_HALT 0
#define SYS_EXIT 1
#define SYS_CREATE 2
#define SYS_SEEK 3
#define SYS_READ 4
#define SYS_WRITE 5
#define SYS_IPC_SEND 6
#define SYS_IPC_RECEIVE 7

const int CLOSE_ALL = -1;
const int ERROR = -1;
const int NOT_LOADED = 0;
const int LOAD_SUCCESS = 1;
const int LOAD_FAIL = 2;

struct lock filesys_lock;

/* Syscall handler function */
void syscall_handler(struct intr_frame *f UNUSED); // Correct the declaration

/* Syscall initialization function */
void syscall_init(void) {
    lock_init(&filesys_lock);

    /* Initialize the syscall hash table */
    for (int i = 0; i < SYSCALL_TABLE_SIZE; i++) {
        syscall_hash_table[i] = NULL; /* Set all entries to NULL */
    }

    /* Register syscall handlers */
    syscall_hash_table[SYS_HALT] = syscall_halt;
    syscall_hash_table[SYS_EXIT] = syscall_exit;
    syscall_hash_table[SYS_IPC_SEND] = syscall_ipc_send;
    syscall_hash_table[SYS_IPC_RECEIVE] = syscall_ipc_receive;
    syscall_hash_table[SYS_CREATE] = syscall_create;
    syscall_hash_table[SYS_SEEK] = syscall_seek;
    syscall_hash_table[SYS_READ] = syscall_read;
    syscall_hash_table[SYS_WRITE] = syscall_write;

    /* Register the system call interrupt handler */
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* System call handler implementation */
void syscall_handler(struct intr_frame *f UNUSED) {
    int *esp = (int *)conv_vaddr((const void *)f->esp);
    int syscall_code = *esp; /* Syscall code */
    int args[3] = {0};       /* Buffer for up to 3 arguments */

    load_arg(f, args, number_of_args(syscall_code));

    if (syscall_code >= 0 && syscall_code < SYSCALL_TABLE_SIZE &&
        syscall_hash_table[syscall_code] != NULL) {
        /* Call the appropriate syscall handler from the hash table */
        f->eax = syscall_hash_table[syscall_code](args);
    } else {
        /* Handle invalid syscall codes */
        exit(SYSCALL_ERROR);
    }
}
