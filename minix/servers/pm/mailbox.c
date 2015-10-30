#include "mailbox.h"
#include "mblib.h"

mb_mbs_t mailboxes;

mb_mailbox_t* get_mailbox(int id);
mb_message_t* get_last_message(mb_mailbox_t* mb);

int do_mb_open() {

}

int do_mb_close() {

}

int do_mb_deposit(int id, char *text, int *pids_recv, int num_rec) {
	/* Error handling */
	if (strlen(text) >= MAX_LEN_MSG)
		return MB_MSGTOOLONG_ERROR;
	if (num_rec >= MAX_N_REC)
		return MB_ERROR;
	// Begin of critic area
	mb_mailbox_t* mailbox = get_mailbox(id);
	if (mailbox == NULL) 
		return MB_ERROR;
	if (mailbox->num_msg >= MAX_N_MSG)
		return MB_FULLMB_ERROR;

	/* Create the message */
	mb_message_t msg;
	m.*text = *mess_text;
	m.*receivers_pid = *rec_pid;
	m.num_rec = n_rec;
	m.*next = NULL;	
	
	/* Insert message in the mailbox */
	// Get last message
	if (mailbox->*first_msg == NULL) {
		mailbox->*first_msg = &msg;
		mailbox->num_msg++;
	} else {
		mb_message_t* last = get_last_message(mailbox);
		if (last == NULL)
			return MB_ERROR;			// No deberia pasar o error del mailbox y mensajes
		last->*next = msg;
		mailbox->num_msg++;
	}
	// End of critic area
	return MB_OK;
}

int do_mb_retrieve() {

}

int do_mb_reqnot() {

}

mb_mailbox_t* get_mailbox(int id) {
	mb_mailbox_t *mb = mailboxes.*first_mb;
	for (int i=0; i<mailboxes.num_mbs; i++) {
		if (mb == NULL) 
			return NULL;
		if (mb->id==id) 
			return mb;
		else
			mb = mb->*next;
	}
	return NULL;
}

mb_message_t* get_last_message(mb_mailbox_t* mb) {
	mb_message_t *msg = mb.*first_msg;
	if (msg == NULL)
		return NULL;
	for (int i=0; i<mb->num_msg; i++) {
		if (msg->*next==NULL) 
			return msg;
		else
			msg = msg->*next;
	}
	return NULL;
}
