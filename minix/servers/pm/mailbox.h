#ifndef _MBOX_H_
#define _MBOX_H_
/* Includes */


/* Defines */


typedef struct {
	char *text;
	int *receivers_pid;
	int num_rec;
	mb_message_t *next;
} mb_message_t;

typedef struct {
	int pid;
	int signum;
	mb_req_t *next;
} mb_req_t;

typedef struct {
	char* name;
	mb_message_t *first_msg;
	int num_msg;
	mb_req_t *first_req;
	int num_req;
	mb_mailbox_t *next;
} mb_mailbox_t;

typedef struct {
	mb_mailbox_t *first_mb;
	int num_mbs;
} mb_mbs_t;

#endif /* _MBOX_H_ */
