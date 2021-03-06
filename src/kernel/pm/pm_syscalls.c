/* $Id$
********************************************************************************
* _____   ____ _______    _______ ____  ______  _____                          *
*|  __ \ / __ \__   __|/\|__   __/ __ \|  ____|/ ____|          Copyright 2008 *
*| |__) | |  | | | |  /  \  | | | |  | | |__  | (___              Daniel Bader *
*|  ___/| |  | | | | / /\ \ | | | |  | |  __|  \___ \           Vincenz Doelle *
*| |    | |__| | | |/ ____ \| | | |__| | |____ ____) |    Johannes Schamburger *
*|_|     \____/  |_/_/    \_\_|  \____/|______|_____/          Dmitriy Traytel *
*                                                                              *
*      Practical Oriented TeAching Tool, Operating (and) Educating System      *
*                                                                              *
*                           www.potatoes-project.tk                            *
*******************************************************************************/

/**
 * @file
 * This is the kernel side implementation of the syscalls.
 *
 * @author dbader
 * @author $LastChangedBy$
 * @version $Rev$
 */

#include "../include/assert.h"
#include "../include/const.h"
#include "../include/stdio.h"
#include "../include/stdlib.h"
#include "../include/string.h"
#include "../include/debug.h"
#include "../include/ringbuffer.h"
#include "pm_main.h"
#include "syscalls_shared.h"
#include "../fs/fs_file_table.h"
#include "../fs/fs_main.h"
#include "../fs/fs_io_functions.h"
#include "../include/init.h"
#include "pm_syscalls.h"
#include "pm_devices.h"

/**
 * The system call table. Make sure this corresponds to the constants
 * defined in syscall_shared.h (otherwise mayhem ensues). */
syscall_handler syscall_table[] = {
        sys_log,        // 0
        sys_exit,       // 1
        sys_getpid,     // 2
        sys_open,       // 3
        sys_close,      // 4
        sys_read,       // 5
        sys_write,      // 6
        sys_seek,       // 7
        sys_malloc,     // 8
        sys_free,       // 9
        sys_unlink,     // 10
        sys_stat,       // 11
        sys_kill        // 12
};

/** Syscall trace macro. Uncomment to print a message everytime a syscall gets executed. */
#define SYSCALL_TRACE //dprintf("[%u] ", getpid()); dprintf

/**
 * The syscall dispatch function. This gets called whenever a thread
 * request a syscall by raising the syscall interrupt (int 0x42).
 * It checks the syscall id for validity and calls the appropiate
 * syscall handler.
 * @see syscall_table
 * @see syscalls_shared.h
 *
 * @param id the syscall id number
 * @param data pointer to the syscall argument structure
 */
void pm_syscall(uint32 id, void* data)
{
        if (id > MAX_SYSCALL) {
                panic("pm_syscall: id > MAX_SYSCALL"); // more verbosity, ie pid, name, regs, stack
        }
        set_interrupts(); //FIXME: I don't know why, but without this, it won't work in virtualbox
        syscall_table[id](data);
}

/**
 * void log(char *msg)
 * Prints debug logging output to the console.
 * @param data arguments
 */
void sys_log(void *data)
{
        SYSCALL_TRACE("SYS_LOG('%s')\n", data);
        //printf("log [%u]: %s\n", getpid(), (char*)data);
        puts((char*)data);
}

/**
 * void _exit(int status)
 * Performs normal program termination. exit() marks the current process as terminated ("dead"),
 * informs any waiting parents but does not yet destroy the process. Freeing process resources
 * is done by pm_schedule().
 * @param data arguments
 */
void sys_exit(void *data)
{
        SYSCALL_TRACE("SYS_EXIT(%d)\n", (sint32)data);
        ASSERT(active_proc != NULL);

        if (getpid() == 0)
                panic("cannot exit() the kernel thread");

        dprintf("exit %u: %d\n", getpid(), (sint32)data);
        active_proc->state = PSTATE_DEAD; // pm_schedule will then unlink and destroy it.
        
        // This should be done in pm_schedule().      
//        free_virt_monitor(active_proc->vmonitor);
        //TODO: return status to waiting parents
}

/**
 * int _getpid();
 */
void sys_getpid(void *data)
{
        SYSCALL_TRACE("SYS_GETPID(0x%x)\n", data);
        if (data == NULL)
                return;

        *((uint32*) data) = getpid();
}

/**
 * int _open(char *path, int oflag, ...);
 */
void sys_open(void *data)
{
        sc_open_args_t *args = (sc_open_args_t*) data;
        SYSCALL_TRACE("SYS_OPEN(\"%s\", 0x%x, 0x%x)\n", args->path, args->oflag, args->mode);

        char* path = strdup(args->path);

        device_t *dev = pm_name2device(path);
        if (dev != NULL) {
                // It's a device file
                args->fd = dev->open(dev, path, args->oflag, args->mode);
        } else {
                // It's a real file.
                int fd = do_open(path);

                // Does not exist yet and the create flag ist set
                if (fd == NOT_POSSIBLE && args->oflag == O_CREAT) {
                        // See if it's a directory
                        if (path[strlen(path)-1] == '/') {
                                // Kill trailing slash.
                                path[strlen(path)-1] = '\0';
                                do_mkdir(path);

                        } else
                                do_mkfile(path);

                        fd = do_open(path);
                }

                if (fd == NOT_POSSIBLE)
                        args->fd = -1;
                else {
                        args->fd = insert_proc_file(active_proc->pft, fd) + MAX_DEVICES;
                }
        }

        free(path);
}

/**
 * int _close(int fd);
 */
void sys_close(void *data)
{
        sc_close_args_t *args = (sc_close_args_t*) data;
        SYSCALL_TRACE("SYS_CLOSE(%d)\n", args->fd);

        if (args->fd < 0) {
                args->success = -1;
                return;
        }

        if (args->fd < MAX_DEVICES) {
                // It's a device file
                device_t *dev = pm_fd2device(args->fd);
                if (!dev)
                        args->success = -1;
                else
                        args->success = dev->close(dev, args->fd);
        } else {
                if (do_close_pf(active_proc->pft, args->fd - MAX_DEVICES) == EOF)
                        args->success = -1;
                else
                        args->success = 0;
        }
}

/**
 * int _read(int fd, void *buf, int size);
 */
void sys_read(void* data)
{
        sc_read_write_args_t *args = (sc_read_write_args_t*) data;

        /* Tracing this syscall will drive you nuts as the shell
         * is using this to poll for new input. */
        //SYSCALL_TRACE("SYS_READ(%d, 0x%x, %d)\n", args->fd, args->buf, args->size);

        if (args->fd < MAX_DEVICES) {
                // It's a device file
                device_t *dev = pm_fd2device(args->fd);
                if (!dev)
                        args->rw_count = -1;
                else
                        args->rw_count = dev->read(dev, args->fd, args->buf, args->size);
        } else {
                // It's a regular file
                SYSCALL_TRACE("SYS_READ(%d, 0x%x, %d)\n", args->fd, args->buf, args->size);
                proc_file *pft_entry = get_proc_file(active_proc->pft, args->fd - MAX_DEVICES);
                args->rw_count = do_read(pft_entry->pf_f_desc, args->buf, args->size, pft_entry->pf_pos);

                pft_entry->pf_pos += args->rw_count;
        }
}
/**
 * int _write(int fd, void *buf, int size);
 */
void sys_write(void* data)
{
        sc_read_write_args_t *args = (sc_read_write_args_t*) data;
        SYSCALL_TRACE("SYS_WRITE(%d, 0x%x, %d)\n", args->fd, args->buf, args->size);

        if (args->fd < MAX_DEVICES) {
                // It's a device file
                device_t *dev = pm_fd2device(args->fd);
                if (!dev)
                        args->rw_count = -1;
                else {
                        args->rw_count = dev->write(dev, args->fd, args->buf, args->size);
                }
        } else {
                // It's a regular file or a dir
                proc_file *pft_entry = get_proc_file(active_proc->pft, args->fd - MAX_DEVICES);

                file_info_t info;
                if (get_file_info(pft_entry->pf_f_desc, &info)->mode == DIRECTORY) {
                        args->rw_count = -1;
                        return;
                }

                args->rw_count = do_write(pft_entry->pf_f_desc, args->buf, args->size, pft_entry->pf_pos);
                pft_entry->pf_pos += args->rw_count;
        }
}
/**
 * int _seek(int fd, int offset, int whence);
 */
void sys_seek(void* data)
{
        sc_seek_args_t *args = (sc_seek_args_t*) data;
        SYSCALL_TRACE("SYS_SEEK(%d, %d, %d)\n", args->fd, args->offset, args->whence);

        if (args->fd < MAX_DEVICES) {
                // It's a device file
                device_t *dev = pm_fd2device(args->fd);
                if (!dev)
                        args->pos = -1;
                else
                        args->pos = dev->seek(dev, args->fd, args->offset, args->whence);
        } else {
                // It's a regular file
                proc_file *pft_entry = get_proc_file(active_proc->pft, args->fd - MAX_DEVICES);

                switch (args->whence) {
                case SEEK_SET:
                        pft_entry->pf_pos = args->offset;
                        break;
                case SEEK_CUR:
                        pft_entry->pf_pos += args->offset;
                        break;
                case SEEK_END:
                        pft_entry->pf_pos = get_file(pft_entry->pf_f_desc)->f_inode->i_size + args->offset;
                        break;
                }

                args->pos = pft_entry->pf_pos;
        }
}

/**
 * void* _malloc(size_t size);
 */
void sys_malloc(void *data)
{
        sc_malloc_args_t *args = (sc_malloc_args_t*) data;
        SYSCALL_TRACE("SYS_MALLOC(%u)\n", args->size);
        args->mem = mallocn(args->size, active_proc->name);
}

/**
 * void _free(void *block);
 */
void sys_free(void *data)
{
        SYSCALL_TRACE("SYS_FREE(0x%x)\n", data);
        free(data);
}

/**
 * int _unlink(char *path)
 */
void sys_unlink(void *data)
{
        sc_unlink_args_t *args = (sc_unlink_args_t*) data;
        SYSCALL_TRACE("SYS_UNLINK(%s)\n", args->path);

        file_nr fd = do_open(args->path);


        //TODO: This should be handled by the file system.

        if (fd == NOT_FOUND) {
                dprintf("%{ERROR: file does not exist or is a device!}\n", RED);
                args->success = -1;
                return;
        }

        if (get_file(fd)->f_count > 0){
                dprintf("%{ERROR: file is still in use!}\n", RED);
                do_close(fd);
                args->success = -1;
                return;
        }

        file_info_t info;
        get_file_info(fd, &info);

        if (info.mode == DIRECTORY && info.size > 0) {
                dprintf("%{ERROR: directory is not empty!} (%d)\n", RED, info.size);
                do_close(fd);
                args->success = -1;
                return;
        }

        do_close(fd);

        args->success = do_remove(args->path);
}

/**
 * int _stat(char *path, stat *buf)
 */
void sys_stat(void *data)
{
        sc_stat_args_t *args = (sc_stat_args_t*) data;
        SYSCALL_TRACE("SYS_STAT(%s, 0x%x)\n", args->path, args->buf);

        bzero(args->buf, sizeof(stat));

        file_nr fd = do_open(args->path);

        if (fd == NOT_FOUND) {
                dprintf("%{ERROR: file does not exist or is a device!}\n", RED);
                args->success = -1;
                return;
        }

        int pft_fd = insert_proc_file(active_proc->pft, fd);

        file_info_t info;
        get_file_info(fd, &info);

        do_close_pf(active_proc->pft, pft_fd);

        memcpy(args->buf, &info, sizeof(stat));
        args->success = 0;
}

/**
 * void _kill(int pid)
 */
void sys_kill(void *data)
{
        SYSCALL_TRACE("SYS_KILL(%d)\n", data);
        pm_kill_proc((uint32)data);
}
