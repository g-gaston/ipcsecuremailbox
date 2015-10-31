#ifndef _MBOX_H_
#define _MBOX_H_
/* Includes */


/* Defines */
#define MAX_LEN_MSG 255
#define MAX_N_REC 16
#define MAX_N_MSG 16
// Añadir n maximo de mailboxes y no sé si algo más

typedef struct {
	char *text;
	int *receivers_pid;
	int num_rec;
	struct mb_message_t *next;
} mb_message_t;

typedef struct {
	int pid;
	int signum;
	struct mb_req_t *next;
} mb_req_t;

typedef struct {
	char* name;
	int id;
	mb_message_t *first_msg;
	int num_msg;
	mb_req_t *first_req;
	int num_req;
	int conn_process;
	struct mb_mailbox_t *next;
} mb_mailbox_t;

typedef struct {
	mb_mailbox_t *first_mb;
	int num_mbs;
	int id_master;
} mb_mbs_t;

#endif /* _MBOX_H_ */
