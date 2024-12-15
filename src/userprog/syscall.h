#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include <stdint.h>
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "userprog/process.h"

/* Define pid_t explicitly as int to avoid undefined type errors */
typedef int pid_t;

/* Syscall codes for IPC */
#define SYS_IPC_SEND 100    /* Code for sending IPC messages */
#define SYS_IPC_RECEIVE 101 /* Code for receiving IPC messages */

/* Filesystem lock (shared across files) */
extern struct lock filesys_lock;

/* Constants for syscall handling */
extern const int CLOSE_ALL;
extern const int ERROR;
extern const int NOT_LOADED;
extern const int LOAD_SUCCESS;
extern const int LOAD_FAIL;

/* Syscall initialization */
void syscall_init(void);

/* Syscall handlers (defined in syscall_handlers.c) */
void syscall_handle_one_arg(int syscall_code, struct intr_frame *f, int *arg);
void syscall_handle_two_args(int syscall_code, struct intr_frame *f, int *arg);
void syscall_handle_three_args(int syscall_code, struct intr_frame *f, int *arg);

/* Individual syscall functions */
void halt(void);
void exit(int status);
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

/* IPC syscall functions */
void ipc_send(const char *message);
void ipc_receive(char *buffer, size_t size);

/* Helper functions (shared across syscall.c and syscall_handlers.c) */
int conv_vaddr(const void *vaddr);
void verify_ptr(const void *vaddr);
void verify_buffer(void *buffer, unsigned size);
void verify_str(const void *str);

#endif /* USERPROG_SYSCALL_H */
