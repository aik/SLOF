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
#include <string.h>

extern void call_client_interface(of_arg_t *);

extern inline int
of_0_1(const char *serv)
{
	of_arg_t arg = {
		p32cast serv,
		0, 1,
		{ 0 }
	};

	call_client_interface(&arg);

	return arg.args[0];
}

extern inline void
of_1_0(const char *serv, int arg0)
{
	of_arg_t arg = {
		p32cast serv,
		1, 0,
		{arg0, 0}
	};

	call_client_interface(&arg);
}

extern inline unsigned int
of_1_1(const char *serv, int arg0)
{
	of_arg_t arg = {
		p32cast serv,
		1, 1,
		{arg0, 0}
	};

	call_client_interface(&arg);
	return arg.args[1];
}

extern inline unsigned int
of_1_2(const char *serv, int arg0, int *ret0)
{
        of_arg_t arg = {
                p32cast serv,
                1, 2,
                {arg0, 0, 0}
        };

        call_client_interface(&arg);
        *ret0 = arg.args[2];
        return arg.args[1];
}

extern inline void
of_2_0(const char *serv, int arg0, int arg1)
{
	of_arg_t arg = {
		p32cast serv,
		2, 0,
		{arg0, arg1, 0}
	};

	call_client_interface(&arg);
}

extern inline unsigned int
of_2_1(const char *serv, int arg0, int arg1)
{
	of_arg_t arg = {
		p32cast serv,
		2, 1,
		{arg0, arg1, 0}
	};

	call_client_interface(&arg);
	return arg.args[2];
}

extern inline unsigned int
of_2_2(const char *serv, int arg0, int arg1, int *ret0)
{
	of_arg_t arg = {
		p32cast serv,
		2, 2,
		{arg0, arg1, 0, 0}
	};

	call_client_interface(&arg);
	*ret0 = arg.args[3];
	return arg.args[2];
}

extern inline unsigned int
of_2_3(const char *serv, int arg0, int arg1, int *ret0, int *ret1)
{
	of_arg_t arg = {
		p32cast serv,
		2, 3,
		{arg0, arg1, 0, 0, 0}
	};

	call_client_interface(&arg);
	*ret0 = arg.args[3];
	*ret1 = arg.args[4];
	return arg.args[2];
}

extern inline void
of_3_0(const char *serv, int arg0, int arg1, int arg2)
{
	of_arg_t arg = {
		p32cast serv,
		3, 0,
		{arg0, arg1, arg2, 0}
	};

	call_client_interface(&arg);
	return;
}

extern inline unsigned int
of_3_1(const char *serv, int arg0, int arg1, int arg2)
{
	of_arg_t arg = {
		p32cast serv,
		3, 1,
		{arg0, arg1, arg2, 0}
	};

	call_client_interface(&arg);
	return arg.args[3];
}

extern inline unsigned int
of_3_2(const char *serv, int arg0, int arg1, int arg2, int *ret0)
{
	of_arg_t arg = {
		p32cast serv,
		3, 2,
		{arg0, arg1, arg2, 0, 0}
	};

	call_client_interface(&arg);
	*ret0 = arg.args[4];
	return arg.args[3];
}

extern inline unsigned int
of_3_3(const char *serv, int arg0, int arg1, int arg2, int *ret0, int *ret1)
{
	of_arg_t arg = {
		p32cast serv,
		3, 3,
		{arg0, arg1, arg2, 0, 0, 0}
	};

	call_client_interface(&arg);
	*ret0 = arg.args[4];
	*ret1 = arg.args[5];
	return arg.args[3];
}

extern inline unsigned int
of_4_1(const char *serv, int arg0, int arg1, int arg2, int arg3)
{
	of_arg_t arg = {
		p32cast serv,
		4, 1,
		{arg0, arg1, arg2, arg3, 0}
	};

	call_client_interface(&arg);
	return arg.args[4];
}

int
of_interpret_1(void *s, void *ret)
{
	return of_1_2("interpret", p32cast s, ret);
}

void
of_close(ihandle_t ihandle)
{
	of_1_0("close", ihandle);
}

int
of_write(ihandle_t ihandle, void *s, int len)
{
	return of_3_1("write", ihandle, p32cast s, len);
}

int
of_read(ihandle_t ihandle, void *s, int len)
{
	return of_3_1("read", ihandle, p32cast s, len);
}

int
of_seek(ihandle_t ihandle, int poshi, int poslo)
{
	return of_3_1("seek", ihandle, poshi, poslo);
}

int
of_getprop(phandle_t phandle, const char *name, void *buf, int len)
{
	return of_4_1("getprop", phandle, p32cast name, p32cast buf, len);
}

phandle_t
of_peer(phandle_t phandle)
{
	return (phandle_t) of_1_1("peer", phandle);
}

phandle_t
of_child(phandle_t phandle)
{
	return (phandle_t) of_1_1("child", phandle);
}

phandle_t
of_parent(phandle_t phandle)
{
	return (phandle_t) of_1_1("parent", phandle);
}

phandle_t
of_finddevice(const char *name)
{
	return (phandle_t) of_1_1("finddevice", p32cast name);
}

ihandle_t
of_open(const char *name)
{
	return (ihandle_t) of_1_1("open", p32cast name);
}

void *
of_claim(void *start, unsigned int size, unsigned int align)
{
	return(void *)(long)(size_t)of_3_1("claim", p32cast start, size, align);
}

void
of_release(void *start, unsigned int size)
{
	(void) of_2_0("release", p32cast start, size);
}

unsigned int
romfs_lookup(const char *name, void **addr)
{
	unsigned int high, low;
	unsigned int i = of_2_3("ibm,romfs-lookup", p32cast name, strlen(name),
				(int *) &high, (int *) &low);
	*addr = (void*)(((unsigned long) high << 32) | (unsigned long) low);
	return i;
}

void *
of_call_method_3(const char *name, ihandle_t ihandle, int arg0)
{
	int entry, rc;
	rc = of_3_2("call-method", p32cast name, ihandle, arg0, &entry);
	return rc != 0 ? 0 : (void *) (long) entry;
}

int
vpd_read(unsigned int offset, unsigned int length, char *data)
{
	int result;
	long tmp = (long) data;
	result = of_3_1("rtas-read-vpd", offset, length, (int) tmp);
	return result;
}

int
vpd_write(unsigned int offset, unsigned int length, char *data)
{
	int result;
	long tmp = (long) data;
	result = of_3_1("rtas-write-vpd", offset, length, (int) tmp);
	return result;
}

static void
ipmi_oem_led_set(int type, int instance, int state)
{
	return of_3_0("set-led", type, instance, state);
}

int
write_mm_log(char *data, unsigned int length, unsigned short type)
{
	long tmp = (long) data;

	ipmi_oem_led_set(2, 0, 1);
	return of_3_1("write-mm-log", (int) tmp, length, type);
}

int
of_yield(void)
{
	return of_0_1("yield");
}

void *
of_set_callback(void *addr)
{
	return (void *) (long) (size_t) of_1_1("set-callback", p32cast addr);
}

void
bootmsg_warning(short id, const char *str, short lvl)
{
	(void) of_3_0("bootmsg-warning", id, lvl, p32cast str);
}

void
bootmsg_error(short id, const char *str)
{
	(void) of_2_0("bootmsg-error", id, p32cast str);
}

void
bootmsg_debugcp(short id, const char *str, short lvl)
{
	(void) of_3_0("bootmsg-debugcp", id, lvl, p32cast str);
}

void
bootmsg_cp(short id)
{
	(void) of_1_0("bootmsg-cp", id);
}
