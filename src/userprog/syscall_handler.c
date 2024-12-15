// #include "userprog/syscall.h"
// #include <syscall-nr.h> // Add this include for syscall constants
// #include "threads/thread.h"
// #include "threads/vaddr.h"
// #include "threads/interrupt.h"
// #include "threads/malloc.h"
// #include "devices/shutdown.h"

// /* Hash table for system calls. */
// syscall_handler_func syscall_hash_table[SYSCALL_TABLE_SIZE];

// /* Sys call*/


// /* Individual system call handlers. */
// int syscall_halt(int *args UNUSED) {
//     halt();
//     return 0;
// }

// int syscall_exit(int *args) {
//     exit(args[0]);
//     return 0;
// }

// int syscall_ipc_send(int *args) {
//     verify_str((const void *)args[0]);
//     ipc_send((const char *)args[0]);
//     return 0;
// }

// int syscall_ipc_receive(int *args) {
//     verify_buffer((void *)args[0], (unsigned)args[1]);
//     ipc_receive((char *)args[0], (size_t)args[1]);
//     return 0;
// }

// int syscall_create(int *args) {
//     verify_str((const void *)args[0]);
//     args[0] = conv_vaddr((const void *)args[0]);
//     return create((const char *)args[0], (unsigned)args[1]);
// }

// int syscall_seek(int *args) {
//     seek(args[0], (unsigned)args[1]);
//     return 0;
// }

// int syscall_read(int *args) {
//     verify_buffer((void *)args[1], (unsigned)args[2]);
//     args[1] = conv_vaddr((const void *)args[1]);
//     return read(args[0], (void *)args[1], (unsigned)args[2]);
// }

// int syscall_write(int *args) {
//     verify_buffer((void *)args[1], (unsigned)args[2]);
//     args[1] = conv_vaddr((const void *)args[1]);
//     return write(args[0], (const void *)args[1], (unsigned)args[2]);
// }

// void load_arg(struct intr_frame *f, int *arg, int n) {
//     int i = 0;
//     while (i < n) {
//         int *ptr = (int *)f->esp + i + 1;
//         verify_ptr((const void *)ptr);
//         arg[i++] = *ptr;
//     }
// }


// /* Array mapping syscall codes to the number of arguments they require. */
// static const int syscall_arg_count[SYSCALL_TABLE_SIZE] = {
//     [SYS_HALT] = 0,
//     [SYS_EXIT] = 1,
//     [SYS_IPC_SEND] = 1,
//     [SYS_IPC_RECEIVE] = 1,
//     [SYS_CREATE] = 2,
//     [SYS_SEEK] = 2,
//     [SYS_READ] = 3,
//     [SYS_WRITE] = 3,
//     // Add more syscalls as needed...
// };

// /* Determine the number of arguments needed for each syscall. */
// int number_of_args(int syscall_code) {
//     if (syscall_code >= 0 && syscall_code < SYSCALL_TABLE_SIZE) {
//         return syscall_arg_count[syscall_code];
//     } else {
//         return 0; /* Unknown syscalls */
//     }
// }

#include "userprog/syscall.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "devices/shutdown.h"
#include <syscall-nr.h>

syscall_handler_func syscall_hash_table[SYSCALL_TABLE_SIZE];

/* Individual system call handlers */
int syscall_halt(int *args UNUSED) {
    halt();
    return 0;
}

int syscall_exit(int *args) {
    exit(args[0]);
    return 0;
}

int syscall_ipc_send(int *args) {
    verify_str((const void *)args[0]);
    ipc_send((const char *)args[0]);
    return 0;
}

int syscall_ipc_receive(int *args) {
    verify_buffer((void *)args[0], (unsigned)args[1]);
    ipc_receive((char *)args[0], (size_t)args[1]);
    return 0;
}

int syscall_create(int *args) {
    verify_str((const void *)args[0]);
    args[0] = conv_vaddr((const void *)args[0]);
    return create((const char *)args[0], (unsigned)args[1]);
}

int syscall_seek(int *args) {
    seek(args[0], (unsigned)args[1]);
    return 0;
}

int syscall_read(int *args) {
    verify_buffer((void *)args[1], (unsigned)args[2]);
    args[1] = conv_vaddr((const void *)args[1]);
    return read(args[0], (void *)args[1], (unsigned)args[2]);
}

int syscall_write(int *args) {
    verify_buffer((void *)args[1], (unsigned)args[2]);
    args[1] = conv_vaddr((const void *)args[1]);
    return write(args[0], (const void *)args[1], (unsigned)args[2]);
}

/* Static helper functions */

/* Load arguments from the user stack */
void load_arg(struct intr_frame *f, int *arg, int n) {
    int i = 0;
    while (i < n) {
        int *ptr = (int *)f->esp + i + 1;
        verify_ptr((const void *)ptr);
        arg[i++] = *ptr;
    }
}

/* Determine the number of arguments needed for each syscall */
int number_of_args(int syscall_code) {
    switch (syscall_code) {
        case SYS_HALT:
        case SYS_EXIT:
        case SYS_IPC_SEND:
        case SYS_IPC_RECEIVE:
            return 1;

        case SYS_CREATE:
        case SYS_SEEK:
            return 2;

        case SYS_READ:
        case SYS_WRITE:
            return 3;

        default:
            return 0; /* Unknown syscalls */
    }
}
