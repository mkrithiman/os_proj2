#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <user/syscall.h>
#include "threads/synch.h"
#include "userprog/process.h"
#include "threads/interrupt.h" // Required for `struct intr_frame`

/* Maximum number of system calls */
#define SYSCALL_TABLE_SIZE 128

/* IPC syscall codes */
#define SYS_IPC_SEND 100    /* Code for sending IPC messages */
#define SYS_IPC_RECEIVE 101 /* Code for receiving IPC messages */

/* Define standard file descriptors if not already defined */
#define STDIN_FILENO  0  /* File descriptor for standard input */
#define STDOUT_FILENO 1  /* File descriptor for standard output */

extern const int CLOSE_ALL;
extern const int ERROR;
extern const int NOT_LOADED;
extern const int LOAD_SUCCESS;
extern const int LOAD_FAIL;

/* Common constants */
#define SYSCALL_ERROR -1

/* File system lock for thread-safe file operations */
extern struct lock filesys_lock;

/* Typedef for system call handler functions */
typedef int (*syscall_handler_func)(int *args);

/* Syscall hash table declaration */
extern syscall_handler_func syscall_hash_table[SYSCALL_TABLE_SIZE];

/* Function declarations for system calls */
void syscall_init(void);
void halt(void) NO_RETURN;
void exit(int status) NO_RETURN;
int exec(const char *file);
int wait(pid_t pid);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);
void ipc_send(const char *message);
void ipc_receive(char *buffer, size_t size);

/* Function declarations for utility functions */
int conv_vaddr(const void *vaddr);
void verify_ptr(const void *vaddr);
void verify_buffer(void *buffer, unsigned size);
void verify_str(const void *str);

/* Function declarations for syscall_handler.c */
int syscall_halt(int *args);
int syscall_exit(int *args);
int syscall_ipc_send(int *args);
int syscall_ipc_receive(int *args);
int syscall_create(int *args);
int syscall_seek(int *args);
int syscall_read(int *args);
int syscall_write(int *args);
void load_arg(struct intr_frame *f, int *arg, int n);
int number_of_args(int syscall_code);
/* Function declarations for syscall.c */
void syscall_handler(struct intr_frame *f);

#endif /* userprog/syscall.h */
