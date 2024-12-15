#include "userprog/syscall.h"
#include <syscall-nr.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <stdio.h>

/* Handle system calls with one argument */

void syscall_exit(struct intr_frame *f, int *arg) {
    exit(arg[0]);
}

void syscall_exec(struct intr_frame *f, int *arg) {
    verify_str((const void *)arg[0]);
    arg[0] = conv_vaddr((const void *)arg[0]);
    f->eax = exec((const char *)arg[0]);
}

void syscall_wait(struct intr_frame *f, int *arg) {
    f->eax = wait(arg[0]);
}

void syscall_open(struct intr_frame *f, int *arg) {
    verify_str((const void *)arg[0]);
    arg[0] = conv_vaddr((const void *)arg[0]);
    f->eax = open((const char *)arg[0]);
}

void syscall_filesize(struct intr_frame *f, int *arg) {
    f->eax = filesize(arg[0]);
}

void syscall_tell(struct intr_frame *f, int *arg) {
    f->eax = tell(arg[0]);
}

void syscall_close(struct intr_frame *f, int *arg) {
    close(arg[0]);
}

void syscall_create(struct intr_frame *f, int *arg) {
    verify_str((const void *)arg[0]);
    arg[0] = conv_vaddr((const void *)arg[0]);
    f->eax = create((const char *)arg[0], (unsigned)arg[1]);
}

void syscall_seek(struct intr_frame *f, int *arg) {
    seek(arg[0], (unsigned)arg[1]);
}

void syscall_read(struct intr_frame *f, int *arg) {
    verify_buffer((void *)arg[1], (unsigned)arg[2]);
    arg[1] = conv_vaddr((const void *)arg[1]);
    f->eax = read(arg[0], (void *)arg[1], (unsigned)arg[2]);
}

void syscall_write(struct intr_frame *f, int *arg) {
    verify_buffer((void *)arg[1], (unsigned)arg[2]);
    arg[1] = conv_vaddr((const void *)arg[1]);
    f->eax = write(arg[0], (const void *)arg[1], (unsigned)arg[2]);
}

static const struct syscall_mapping syscall_map[] = {
    {SYS_EXIT, syscall_exit},
    {SYS_EXEC, syscall_exec},
    {SYS_WAIT, syscall_wait},
    {SYS_CREATE, syscall_create},
    {SYS_OPEN, syscall_open},
    {SYS_FILESIZE, syscall_filesize},
    {SYS_TELL, syscall_tell},
    {SYS_CLOSE, syscall_close},
    {SYS_SEEK, syscall_seek},
    {SYS_READ, syscall_read},
    {SYS_WRITE, syscall_write},
    // Add more syscalls as needed.
};
void call_syscall_handler(int syscall_code, struct intr_frame *f, int *arg) {
    size_t num_syscalls = sizeof(syscall_map) / sizeof(syscall_map[0]);
    for (size_t i = 0; i < num_syscalls; i++) {
        if (syscall_map[i].syscall_code == syscall_code) {
            syscall_map[i].handler(f, arg);
            return;
        }
    }
    exit(ERROR);  // Exit if the syscall code is invalid.
}
static void syscall_handler(struct intr_frame *f) {
    int arg[3];
    int esp = (int)f->esp;
    verify_ptr((const void *)esp);

    int syscall_code = *(int *)esp;
    int number_of_arg = number_of_args(syscall_code);

    load_arg(f, arg, number_of_arg);
    call_syscall_handler(syscall_code, f, arg);
}

#define ERROR -1
#define STDIN  0
#define STDOUT 1

/* Helper functions */
static struct file *get_file_with_lock(int fd, struct thread *cur_thread) {
    struct file *file_ptr = process_get_file(fd, cur_thread);
    if (file_ptr != NULL) {
        lock_acquire(&filesys_lock);
    }
    return file_ptr;
}

static void release_file_lock() {
    lock_release(&filesys_lock);
}

static int handle_file_operation(struct file *file_ptr, int (*operation)(struct file *, const void *, unsigned),
                                 const void *buffer, unsigned size) {
    if (file_ptr == NULL) return ERROR;
    lock_acquire(&filesys_lock);
    int result = operation(file_ptr, buffer, size);
    lock_release(&filesys_lock);
    return result;
}

static bool perform_file_action(struct file *file_ptr, void (*action)(struct file *)) {
    if (file_ptr == NULL) return false;
    lock_acquire(&filesys_lock);
    action(file_ptr);
    lock_release(&filesys_lock);
    return true;
}

/* Refactored system call functions */
void halt(void) {
    shutdown_power_off();
}

void exit(int status_code) {
    struct thread *current_thread = thread_current();
    if (current_thread->cp) {
        current_thread->cp->status = status_code;
    }
    printf("%s: exit(%d)\n", current_thread->name, status_code);
    thread_exit();
}

int exec(const char *cmd_line) {
    int process_id = process_execute(cmd_line);
    struct child_process *child_proc = get_child_process(process_id, thread_current());
    if (child_proc != NULL) {
        if (child_proc->load == NOT_LOADED) {
            sema_down(&child_proc->load_sema);
        }
        if (child_proc->load == LOAD_FAIL) {
            remove_child_process(child_proc);
            return ERROR;
        }
    } else {
        return ERROR;
    }
    return process_id;
}

int wait(pid_t process_id) {
    return process_wait(process_id);
}

bool create(const char *filename, unsigned initial_size) {
    lock_acquire(&filesys_lock);
    bool success = filesys_create(filename, initial_size);
    lock_release(&filesys_lock);
    return success;
}

bool remove(const char *filename) {
    lock_acquire(&filesys_lock);
    bool success = filesys_remove(filename);
    lock_release(&filesys_lock);
    return success;
}

int open(const char *filename) {
    lock_acquire(&filesys_lock);
    struct file *file_ptr = filesys_open(filename);
    int result = (file_ptr != NULL) ? process_add_file(file_ptr, thread_current()) : ERROR;
    lock_release(&filesys_lock);
    return result;
}

int filesize(int fd) {
    struct thread *current_thread = thread_current();
    struct file *file_ptr = get_file_with_lock(fd, current_thread);
    if (file_ptr == NULL) return ERROR;

    int length = file_length(file_ptr);
    release_file_lock();
    return length;
}

int read(int fd, void *buffer, unsigned size) {
    struct thread *current_thread = thread_current();
    if (fd == STDIN) {
        uint8_t *temp_buffer = (uint8_t *)buffer;
        for (unsigned i = 0; i < size; i++) {
            temp_buffer[i] = input_getc();
        }
        return size;
    }

    struct file *file_ptr = get_file_with_lock(fd, current_thread);
    if (file_ptr == NULL) return ERROR;

    int bytes_read = file_read(file_ptr, buffer, size);
    release_file_lock();
    return bytes_read;
}

int write(int fd, const void *buffer, unsigned size) {
    struct thread *current_thread = thread_current();
    if (fd == STDOUT) {
        putbuf(buffer, size);
        return size;
    }

    struct file *file_ptr = get_file_with_lock(fd, current_thread);
    if (file_ptr == NULL) return ERROR;

    int bytes_written = file_write(file_ptr, buffer, size);
    release_file_lock();
    return bytes_written;
}

void seek(int fd, unsigned position) {
    struct thread *current_thread = thread_current();
    struct file *file_ptr = get_file_with_lock(fd, current_thread);
    if (file_ptr != NULL) {
        file_seek(file_ptr, position);
        release_file_lock();
    }
}

unsigned tell(int fd) {
    struct thread *current_thread = thread_current();
    struct file *file_ptr = get_file_with_lock(fd, current_thread);
    if (file_ptr == NULL) return ERROR;

    unsigned position = file_tell(file_ptr);
    release_file_lock();
    return position;
}

void close(int fd) {
    struct thread *current_thread = thread_current();
    lock_acquire(&filesys_lock);
    process_close_file(fd, current_thread);
    lock_release(&filesys_lock);
}

void ipc_send(const char *message) {
    sema_down(&shared_ipc_buffer.sema);
    size_t i = 0;
    while (i < IPC_BUFFER_SIZE - 1 && message[i] != '\0') {
        shared_ipc_buffer.data[i] = message[i];
        i++;
    }
    shared_ipc_buffer.data[i] = '\0';
    sema_up(&shared_ipc_buffer.sema);
}

void ipc_receive(char *buffer, size_t size) {
    sema_down(&shared_ipc_buffer.sema);
    size_t i = 0;
    while (i < size - 1 && shared_ipc_buffer.data[i] != '\0') {
        buffer[i] = shared_ipc_buffer.data[i];
        i++;
    }
    buffer[i] = '\0';
    sema_up(&shared_ipc_buffer.sema);
}