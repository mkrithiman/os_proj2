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

/* Common constants for syscall handling */
extern const int CLOSE_ALL;    /* Special file descriptor to close all files */
extern const int ERROR;        /* Error status for invalid syscalls or failures */
extern const int NOT_LOADED;   /* Indicates a process has not loaded */
extern const int LOAD_SUCCESS; /* Indicates a process loaded successfully */
extern const int LOAD_FAIL;    /* Indicates a process failed to load */

/* Struct for mapping syscalls to their handlers */
struct syscall_mapping {
    int syscall_code; /* The syscall code (e.g., SYS_EXIT) */
    void (*handler)(struct intr_frame *f, int *arg); /* Corresponding handler function */
};

/* Syscall initialization */
void syscall_init(void);

/* Individual syscall functions */
void halt_system(void);                          // Replaces `halt`
void terminate_process(int status_code);         // Replaces `exit`
int execute_program(const char *cmd_line);       // Replaces `exec`
int wait_for_program(pid_t process_id);          // Replaces `wait`
bool create_file(const char *filename, unsigned initial_size); // Replaces `create`
bool delete_file(const char *filename);          // Replaces `remove`
int open_file(const char *filename);             // Replaces `open`
int get_file_size(int fd);                       // Replaces `filesize`
int read_from_file(int fd, void *buffer, unsigned size); // Replaces `read`
int write_to_file(int fd, const void *buffer, unsigned size); // Replaces `write`
void set_file_position(int fd, unsigned position); // Replaces `seek`
unsigned get_file_position(int fd);              // Replaces `tell`
void close_file(int fd);                         // Replaces `close`
void call_syscall_handler(int syscall_code, struct intr_frame *f, int *arg);

/* IPC syscall functions */
void ipc_send_message(const char *message);      // Replaces `ipc_send`
void ipc_receive_message(char *buffer, size_t size); // Replaces `ipc_receive`

/* Helper functions (shared across syscall.c and syscall_handlers.c) */
int convert_user_vaddr(const void *vaddr);       // Replaces `conv_vaddr`
bool is_valid_pointer(const void *vaddr);        // Replaces `verify_ptr`
void validate_buffer(void *buffer, unsigned size); // Replaces `verify_buffer`
void validate_string(const void *str);           // Replaces `verify_str`

#endif /* USERPROG_SYSCALL_H */
