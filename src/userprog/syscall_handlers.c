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

// /* Handle system calls with one argument */

// void syscall_handle_one_arg(int syscall_code, struct intr_frame *f, int *arg) {
//     switch (syscall_code) {
//         case SYS_EXIT:
//             exit(arg[0]);
//             break;
//         case SYS_IPC_SEND:
//             verify_str((const void *)arg[0]);
//             ipc_send((const char *)arg[0]);
//             break;
//         case SYS_IPC_RECEIVE:
//             verify_buffer((void *)arg[0], (unsigned)arg[1]);
//             ipc_receive((char *)arg[0], (size_t)arg[1]);
//             break;
//         case SYS_EXEC:
//             verify_str((const void *)arg[0]);
//             arg[0] = conv_vaddr((const void *)arg[0]);
//             f->eax = exec((const char *)arg[0]);
//             break;
//         case SYS_WAIT:
//             f->eax = wait(arg[0]);
//             break;
//         case SYS_REMOVE:
//             verify_str((const void *)arg[0]);
//             arg[0] = conv_vaddr((const void *)arg[0]);
//             f->eax = remove((const char *)arg[0]);
//             break;
//         case SYS_OPEN:
//             verify_str((const void *)arg[0]);
//             arg[0] = conv_vaddr((const void *)arg[0]);
//             f->eax = open((const char *)arg[0]);
//             break;
//         case SYS_FILESIZE:
//             f->eax = filesize(arg[0]);
//             break;
//         case SYS_TELL:
//             f->eax = tell(arg[0]);
//             break;
//         case SYS_CLOSE:
//             close(arg[0]);
//             break;
//     }
// }

// /* Handle system calls with two arguments */
// void syscall_handle_two_args(int syscall_code, struct intr_frame *f, int *arg) {
//     switch (syscall_code) {
//         case SYS_CREATE:
//             verify_str((const void *)arg[0]);
//             arg[0] = conv_vaddr((const void *)arg[0]);
//             f->eax = create((const char *)arg[0], (unsigned)arg[1]);
//             break;
//         case SYS_SEEK:
//             seek(arg[0], (unsigned)arg[1]);
//             break;
//     }
// }

// /* Handle system calls with three arguments */
// void syscall_handle_three_args(int syscall_code, struct intr_frame *f, int *arg) {
//     verify_buffer((void *)arg[1], (unsigned)arg[2]);
//     arg[1] = conv_vaddr((const void *)arg[1]);
//     switch (syscall_code) {
//         case SYS_READ:
//             f->eax = read(arg[0], (void *)arg[1], (unsigned)arg[2]);
//             break;
//         case SYS_WRITE:
//             f->eax = write(arg[0], (const void *)arg[1], (unsigned)arg[2]);
//             break;
//     }
// }


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


/* System call implementations */
void halt(void) {
    shutdown_power_off();
}

void exit(int status) {
    struct thread *cur = thread_current();
    if (cur->cp) {
        cur->cp->status = status;
    }
    printf("%s: exit(%d)\n", cur->name, status);
    thread_exit();
}

int exec(const char *cmd_line) {
    int pid = process_execute(cmd_line);
    struct child_process *cp = get_child_process(pid, thread_current());
    if (cp != NULL) {
        if (cp->load == NOT_LOADED) {
            sema_down(&cp->load_sema);
        }
        if (cp->load == LOAD_FAIL) {
            remove_child_process(cp);
            return ERROR;
        }
    } else {
        return ERROR;
    }
    return pid;
}

int wait(pid_t pid) {
    return process_wait(pid);
}

bool create(const char *file, unsigned initial_size) {
    lock_acquire(&filesys_lock);
    bool success = filesys_create(file, initial_size);
    lock_release(&filesys_lock);
    return success;
}

bool remove(const char *file) {
    lock_acquire(&filesys_lock);
    bool success = filesys_remove(file);
    lock_release(&filesys_lock);
    return success;
}

int open(const char *file) {
    struct file *f = filesys_open(file);
    int to_return;
    lock_acquire(&filesys_lock);
    to_return = f != NULL ? process_add_file(f, thread_current()) : ERROR;
    lock_release(&filesys_lock);
    return to_return;
}

int filesize(int fd) {
    struct file *f = process_get_file(fd, thread_current());
    int to_return;
    lock_acquire(&filesys_lock);
    to_return = f != NULL ? file_length(f) : ERROR;
    lock_release(&filesys_lock);
    return to_return;
}

int read(int fd, void *buffer, unsigned size) {
    if (fd != STDIN_FILENO) {
        struct file *f = process_get_file(fd, thread_current());
        if (f != NULL) {
            lock_acquire(&filesys_lock);
            int bytes = file_read(f, buffer, size);
            lock_release(&filesys_lock);
            return bytes;
        } else {
            return ERROR;
        }
    } else {
        uint8_t *temp = (uint8_t *)buffer;
        for (unsigned i = 0; i < size; i++) {
            temp[i] = input_getc();
        }
        return size;
    }
}

int write(int fd, const void *buffer, unsigned size) {
    if (fd != STDOUT_FILENO) {
        struct file *f = process_get_file(fd, thread_current());
        if (f != NULL) {
            lock_acquire(&filesys_lock);
            int bytes = file_write(f, buffer, size);
            lock_release(&filesys_lock);
            return bytes;
        } else {
            return ERROR;
        }
    } else {
        putbuf(buffer, size);
        return size;
    }
}

void seek(int fd, unsigned position) {
    struct file *f = process_get_file(fd, thread_current());
    if (f != NULL) {
        lock_acquire(&filesys_lock);
        file_seek(f, position);
        lock_release(&filesys_lock);
    }
}

unsigned tell(int fd) {
    struct file *f = process_get_file(fd, thread_current());
    if (f != NULL) {
        lock_acquire(&filesys_lock);
        unsigned pos = file_tell(f);
        lock_release(&filesys_lock);
        return pos;
    }
    return ERROR;
}

void close(int fd) {
    lock_acquire(&filesys_lock);
    process_close_file(fd, thread_current());
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
