/******************************************************************************
 * Copyright (c) 2004, 2007 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/


#include <of.h>
#include <systemcall.h>
#include <stdarg.h>

extern ihandle_t fd_array[32];
    
int _socket (int, int, int, char*);
long _recv (int, void*, int, int);
long _sendto (int, void*, int, int, void*, int);
long _send (int, void*, int, int);
long _ioctl (int, int, void*);
int vsprintf(char *, const char *, va_list);

long 
_syscall_write (int fd, char *buf, long len)
{
    char dest_buf[512];
    char *dest_buf_ptr;
    int i;
    if (fd == 1 || fd == 2)
    {
	dest_buf_ptr = &dest_buf[0];
        for (i = 0; i < len && i < 256; i++)
        {
            *dest_buf_ptr++ = *buf++;
            if (buf[-1] == '\n')
                *dest_buf_ptr++ = '\r';
	}
	len = dest_buf_ptr - &dest_buf[0];
	buf = &dest_buf[0];
    }
    return of_write (fd_array[fd], buf, len);
}

int
printk(const char* fmt, ...)
{
    int count;
    va_list ap;
    char buffer[256];
    va_start (ap, fmt);
    count=vsprintf(buffer, fmt, ap);
    _syscall_write (1, buffer, count);
    va_end (ap);
    return count;
}

long 
_syscall_read (int fd, char *buf, long len)
{
    return of_read (fd_array[fd], buf, len);
}

long 
_syscall_lseek (int fd, long offset, int whence)
{
    if (whence != 0)
	return -1;

    of_seek (fd_array[fd], (unsigned int) (offset>>32), (unsigned int) (offset & 0xffffffffULL));

    return offset;
}

void 
_syscall_close(int fd)
{
    of_close(fd_array[fd]);
}

int 
_syscall_ioctl (int fd, int request, void* data)
{
	return _ioctl (fd, request, data);
}

long 
_syscall_socket (long which, long arg0, long arg1, long arg2, 
		 long arg3, long arg4, long arg5)
{
    long rc = -1;
    switch (which)
    {
	case _sock_sc_nr:
	    rc = _socket (arg0, arg1, arg2, (char*) arg3);
	    break;
	case _recv_sc_nr:
	    rc = _recv (arg0, (void *) arg1, arg2, arg3);
	    break;
	case _send_sc_nr:
	    rc = _send (arg0, (void *) arg1, arg2, arg3);
	    break;
	case _sendto_sc_nr:
	    rc = _sendto (arg0, (void *) arg1, arg2, arg3, (void *) arg4, arg5);
	    break;
    }
    return rc;
}

int
_syscall_open(const char* name, int flags)
{
    static int fd = 2;
    ihandle_t ihandle;

    if ((ihandle = of_open (name)) == 0)
    {
	    printk ("Cannot open %s\n", name);
	    return -1;
    }

    fd++;
    fd_array[fd] = ihandle;

    return fd;
}

long
_system_call(long arg0, long arg1, long arg2, long arg3, 
	     long arg4, long arg5, long arg6, int nr)
{
    long rc = -1;
    switch (nr)
    {

	case _open_sc_nr:
	    rc = _syscall_open ((void *) arg0, arg1);
	    break;
	case _read_sc_nr:
	    rc = _syscall_read (arg0, (void *) arg1, arg2);
	    break;
	case _close_sc_nr:
	    _syscall_close (arg0);
	    break;
	case _lseek_sc_nr:
	    rc = _syscall_lseek (arg0, arg1, arg2);
	    break;
	case _write_sc_nr:
	    rc = _syscall_write (arg0, (void *) arg1, arg2);
	    break;
	case _ioctl_sc_nr:
	    rc = _syscall_ioctl (arg0, arg1, (void *) arg2);
	    break;
	case _socket_sc_nr:
	    rc = _syscall_socket (arg0, arg1, arg2, arg3,
				  arg4, arg5, arg6);
	    break;
    }
    return rc;
}

void _exit(int status);

void 
exit(int status)
{
	_exit(status);
}
