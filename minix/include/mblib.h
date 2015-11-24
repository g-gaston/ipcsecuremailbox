#ifndef _MBLIB_H_
#define _MBLIB_H_

#include <lib.h>
#include <unistd.h>
#include <stdio.h>      /* Input/Output */

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
#define MB_ALLOC_MEM_ERROR -11
#define MB_PERMISSION_ERROR -12
#define MB_ROOT_ALREADY_REGISTERED_ERROR -13
#define MB_USER_EXIST_ERROR -14
#define MB_USER_NOT_EXIST_ERROR -15
#define MB_MAILBOX_ALREADY_EXISTS_ERROR -16

int transform_result(int result);

int mb_open(char *name) {
	int name_len = strlen(name);
	message m;
	if (name_len >= 44) {
		return MB_NAME_ERROR;
	}
	strcpy(m.m3_ca1, name);
	return transform_result(_syscall(PM_PROC_NR, MB_OPEN, &m));
}

int mb_close(int id) {
	message m;
	m.m1_i1 = id;
	return transform_result(_syscall(PM_PROC_NR, MB_CLOSE, &m));
}

int mb_deposit(int id, char *text, int *pids_recv, int num_recv) {
	message m;
	m.m1_i1 = id;
	m.m1_p1 = text;
	m.m1_p2 = pids_recv; /* Warning when receiving message in mailbox.c */
	m.m1_i2 = strlen(text);
	m.m1_i3 = num_recv;
	return transform_result(_syscall(PM_PROC_NR, MB_DEPOSIT, &m));
}

int mb_retrieve(int id, char *buffer, int buffer_len) {
	message m;
	m.m1_i1 = id;
	m.m1_p1 = buffer;
	m.m1_i2 = buffer_len;
	return transform_result(_syscall(PM_PROC_NR, MB_RETRIEVE, &m));
}

int mb_reqnot(int id, int signum) {
	message m;
	m.m1_i1 = id;
	m.m1_i2 = signum;
	return transform_result(_syscall(PM_PROC_NR, MB_REQNOT, &m));
}

int mb_be_root() {
	message m;
	return transform_result(_syscall(PM_PROC_NR, MB_BEROOT, &m));
}

int mb_assign_leader(int pid){
	message m;
	m.m1_i1 = pid;
	return transform_result(_syscall(PM_PROC_NR, MB_ASSIGNLEADER, &m));
}
int mb_remove_leader(int pid){
	message m;
	m.m1_i1 = pid;
	return transform_result(_syscall(PM_PROC_NR, MB_REMOVELEADER, &m));
}
int mb_root_deny_send(int pid){
	message m;
	m.m1_i1 = pid;
	return transform_result(_syscall(PM_PROC_NR, MB_ROOTDSEND, &m));
}
int mb_root_deny_retrieve(int pid){
	message m;
	m.m1_i1 = pid;
	return transform_result(_syscall(PM_PROC_NR, MB_ROOTDRETRIEVE, &m));
}
int mb_root_allow_send(int pid){
	message m;
	m.m1_i1 = pid;
	return transform_result(_syscall(PM_PROC_NR, MB_ROOTASEND, &m));
}
int mb_root_allow_retrieve(int pid){
	message m;
	m.m1_i1 = pid;
	return transform_result(_syscall(PM_PROC_NR, MB_ROOTARETRIEVE, &m));
}
int mb_create_secure_mailbox(char *name, int *pids_recv, int num_recv, int *pids_sends, int num_sends){
	int name_len = strlen(name);
	if (name_len >= 44) {
		return MB_NAME_ERROR;
	}
	message m;
	m.m1_i1 = num_recv;
	m.m1_p1 = pids_recv;
	m.m1_p2 = pids_sends; /* Warning when receiving message in mailbox.c */
	m.m1_i2 = num_sends;
	m.m1_i3 = name_len;
	m.m1_p3 = name;
	return transform_result(_syscall(PM_PROC_NR, MB_CREATESMAILBOX, &m));
}
int mb_create_public_mailbox(char *name){
	int name_len = strlen(name);
	if (name_len >= 44) {
		return MB_NAME_ERROR;
	}
	message m;
	m.m1_i3 = name_len;
	m.m1_p3 = name;
	return transform_result(_syscall(PM_PROC_NR, MB_CREATEPMAILBOX, &m));
}
int mb_deny_send(int mb_id, int pid){
	message m;
	m.m1_i1 = mb_id;
	m.m1_i2 = pid;
	return transform_result(_syscall(PM_PROC_NR, MB_DSEND, &m));

}
int mb_deny_retrieve(int mb_id, int pid){
	message m;
	m.m1_i1 = mb_id;
	m.m1_i2 = pid;
	return transform_result(_syscall(PM_PROC_NR, MB_DRETRIEVE, &m));
}
int mb_allow_send(int mb_id, int pid){
	message m;
	m.m1_i1 = mb_id;
	m.m1_i2 = pid;
	return transform_result(_syscall(PM_PROC_NR, MB_ASEND, &m));
}
int mb_allow_retrieve(int mb_id, int pid){
	message m;
	m.m1_i1 = mb_id;
	m.m1_i2 = pid;
	return transform_result(_syscall(PM_PROC_NR, MB_ARETRIEVE, &m));
}
int mb_remove_group(int mb_id){
	message m;
	m.m1_i1 = mb_id;
	return transform_result(_syscall(PM_PROC_NR, MB_REMOVEGROUP, &m));
}

int mb_rmv_oldest_msg(int mb_id) {
	message m;
	m.m1_i1 = mb_id;
	return transform_result(_syscall(PM_PROC_NR, MB_REMOVEOLDMSG, &m));
}

int mb_exit_root() {
	message m;
	return transform_result(_syscall(PM_PROC_NR, MB_EXITROOT, &m));
}

int transform_result(int result) {
	if (result < 21) {
		result = -result;
	}
	return result;
}

#endif /* _MBLIB_H_ */
