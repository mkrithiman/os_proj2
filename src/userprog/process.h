#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "userprog/syscall.h"

/* the file that this process has */
struct process_file {
	int fd;
	struct file *file;
	struct list_elem elem;
};

/* original function from pintos */
tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

/* function header added for project 2: process_file struct */
int current_process_add_file (struct file *f, struct thread * t);
struct file* current_process_get_file (int fd, struct thread * t);
void current_process_close_file (int fd, struct thread * t);

/* function header added for child_process struct */
void add_child_process (int pid, struct thread * t);
struct child_process* get_child_process (int pid, struct thread * t);
void remove_child_process (struct child_process *cp);
void remove_children (struct thread * t);

#endif /* userprog/process.h */