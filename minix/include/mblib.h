#ifndef _MBLIB_H_
#define _MBLIB_H_

#include <lib.h>
#include <unistd.h>

int mb_open(char *name) {
	message m;
	m.m1_p1 = name;
	return ( _syscall(PM_PROC_NR, MB_OPEN, &m) );
}

int mb_close(int id) {
	message m;
	m.m1_i1 = id;
	return ( _syscall(PM_PROC_NR, MB_CLOSE, &m) );
}

int mb_deposit(int id, char *text, int *pids_recv, int num_recv) {
	message m;
	m.m1_i1 = id;
	m.m1_p1 = text;
	char rec_list[num_recv];
	memcpy(rec_list, pids_recv, num_recv);
	m.m1_p2 = rec_list;
	m.m1_i2 = num_recv;
	return ( _syscall(PM_PROC_NR, MB_DEPOSIT, &m) );
}

int mb_retrieve(int id, char *buffer, int buffer_len) {
	message m;
	m.m1_i1 = id;
	m.m1_p1 = buffer;
	m.m1_i2 = buffer_len;
	return ( _syscall(PM_PROC_NR, MB_RETRIEVE, &m) );
}

int mb_reqnot(int id, int signum) {
	message m;
	m.m1_i1 = id;
	m.m1_i2 = signum;
	return ( _syscall(PM_PROC_NR, MB_REQNOT, &m) );
}

#endif /* _MBLIB_H_ */
