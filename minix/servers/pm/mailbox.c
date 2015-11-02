#include "mailbox.h"
#include "mblib.h"

mb_mbs_t mailboxes;

mb_mailbox_t* get_mailbox(int id);
mb_message_t* get_last_message(mb_mailbox_t* mb);
void remove_msg (mb_message_t* msg, mb_message_t* msg_prv, mb_mailbox_t* mailbox);
mb_req_t* get_last_req(mb_mailbox_t* mb);

int do_mb_open() {
	return 0;
}

int do_mb_close() {
	return 0;
}

// usar malloc para text?
int do_mb_deposit() {
	int id = m_in.m1_i1;
	char *text[MAX_LEN_MSG] = m_in.m1_p1;
	int *pid_recv = (int *)m_in.m1_p2;
	int num_recv = m_in.m1_i2;

	/* Error handling */
	if (strlen(text) >= MAX_LEN_MSG)
		return MB_MSGTOOLONG_ERROR;
	if (num_recv >= MAX_N_REC)
		return MB_ERROR;
	// Begin of critic area
	mb_mailbox_t* mailbox = get_mailbox(id);
	if (mailbox == NULL) 
		return MB_ERROR;
	if (mailbox->num_msg >= MAX_N_MSG)
		return MB_FULLMB_ERROR;

	/* Create the message */
	int size_msg = sizeof(char*)+sizeof(int*)+sizeof(int)+sizeof(mb_message_t*);
	mb_message_t *msg = malloc(size_msg);

	msg->text = text;
	msg->receivers_pid = pid_recv;
	msg->num_rec = num_recv;
	msg->next = NULL;	
	
	/* Insert message in the mailbox */
	// Get last message
	if (mailbox->first_msg == NULL) {
		mailbox->first_msg = msg;
		mailbox->num_msg++;
	} else {
		mb_message_t* last = get_last_message(mailbox);
		if (last == NULL)
			return MB_ERROR;			// No deberia pasar o error del mailbox y mensajes
		last->next = msg;
		mailbox->num_msg++;
	}
	
	/* Notify */
	for (int i=0; i<num_recv; i++) {
		mb_req_t* req = mailbox->first_req;
		for (int j=0; j<mailbox->num_req; j++) {
			if (pid_recv[i] == req->pid)
				kill(req->pid, req->signum);
			req = req->next;
		}
	}
	// End of critic area
	return MB_OK;
}

int do_mb_retrieve() {
	int id = m_in.m1_i1;
	char *buffer = m_in.m1_p1;
	int buffer_len = m_in.m1_i2;

	/* Error Handling */
	// Begin of critic area
	mb_mailbox_t* mailbox = get_mailbox(id);
	if (mailbox == NULL) 
		return MB_ERROR;
	if (mailbox->num_msg == 0)
		return MB_EMPTYMB_ERROR;
	
	/* Search for messages */
	int my_pid = getpid();
	mb_message_t* msg_prv = NULL;
	mb_message_t* msg = mailbox->first_msg;
	for (int i=0; i<mailbox->num_msg; i++) {
		// Recorrer los mensajes del mailbox y en cada mensaje los pid a los que va dirigido.
		int* list_pids = msg->receivers_pid;
		for (int j=0; i<msg->num_rec; j++) {
			if (list_pids[j] == my_pid){
				if (strlen(msg->text) <= buffer_len) {
					strcpy(buffer, msg->text);
					while (j<msg->num_rec-1) {
						list_pids[j]=list_pids[j+1];
						j++;
					}
					msg->num_rec--;
					if (msg->num_rec == 0) 
						remove_msg(msg, msg_prv, mailbox);
					return MB_OK;
				} else {
					return MB_BUFFERTOOSMALL_ERROR;
				}
			}
		}
		msg_prv = msg;
		msg = msg->next;
	}
	// End of critic area
	return MB_NOMSG_ERROR;
}

int do_mb_reqnot() {
	int id = m_in.m1_i1;
	int signum = m_in.m1_i2;

	// Begin of critic area
	mb_mailbox_t* mailbox = get_mailbox(id);
	if (mailbox == NULL) 
		return MB_ERROR;
	if (mailbox->num_req >= MAX_N_REQ) 
		return MB_FULNOTIFYLIST_ERROR;
	
	int size_req = sizeof(int)*2+sizeof(mb_req_t*);
	mb_req_t *req = malloc(size_req);

	req->pid = getpid();
	req->signum = signum;
	req->next = NULL;

	
	/* Insert req in the mailbox */
	// Get last req
	if (mailbox->first_req == NULL) {
		mailbox->first_req = req;
		mailbox->num_req++;
	} else {
		mb_req_t* last = get_last_req(mailbox);
		if (last == NULL)
			return MB_ERROR;			// No deberia pasar o error del mailbox y mensajes
		last->next = req;
		mailbox->num_req++;
	}
	return MB_OK;
}

mb_mailbox_t* get_mailbox(int id) {
	mb_mailbox_t *mb = mailboxes.first_mb;
	for (int i=0; i<mailboxes.num_mbs; i++) {
		if (mb == NULL) 
			return NULL;
		if (mb->id==id) 
			return mb;
		else
			mb = mb->next;
	}
	return NULL;
}

mb_message_t* get_last_message(mb_mailbox_t* mb) {
	mb_message_t *msg = mb->first_msg;
	if (msg == NULL)
		return NULL;
	for (int i=0; i<mb->num_msg; i++) {
		if (msg->next==NULL) 
			return msg;
		else
			msg = msg->next;
	}
	return NULL;
}

mb_req_t* get_last_req(mb_mailbox_t* mb) {
	mb_req_t *req = mb->first_req;
	if (req == NULL)
		return NULL;
	for (int i=0; i<mb->num_req; i++) {
		if (req->next==NULL) 
			return req;
		else
			req = req->next;
	}
	return NULL;
}

void remove_msg (mb_message_t* msg, mb_message_t* msg_prv, mb_mailbox_t* mailbox) {
	if (msg_prv == NULL) {
		if (mailbox->num_msg == 1) {
			mailbox->first_msg = NULL;
		} else {
			mailbox->first_msg = msg->next;
		}
	} else {
		msg_prv->next = msg->next;
	}
	mailbox->num_msg--;
	free(msg);
}
