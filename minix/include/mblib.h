#ifndef _MBLIB_H_
#define _MBLIB_H_

#include <lib.h>
#include <unistd.h>

#define MB_OK 0
#define MB_ERROR -1
#define MB_FULLMB_ERROR -2
#define MB_MAXMB_ERROR -3
#define MB_MSGTOOLONG_ERROR -4
#define MB_BUFFERTOOSMALL_ERROR -5
#define MB_EMPTYMB_ERROR -6
#define MB_FULNOTIFYLIST_ERROR -7
#define MB_NOMSG_ERROR -8
#define MB_NAME_ERROR -9
#define MB_CLOSE_ERROR -10

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
	int rec_list[num_recv];
	for (int i = 0; i < num_recv; ++i) {
		memcpy(rec_list[i], pids_recv[i], sizeof(int));
	}
	m.m1_p2 = (char *)rec_list; /* Warning when receiving message in mailbox.c */
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
