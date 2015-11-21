#include "mailbox.h"
#include "pm.h"
#include <stdlib.h>
#include <string.h>
#include "glo.h"
#include "mproc.h"

mb_mbs_t mailboxes={NULL, 0, 20, 0, NULL, 0, NULL, 0, NULL, 0}; //Start ids in 20 so we can use (1,20) for errors

//Functions declaration
mb_mailbox_t* getMailboxByID(int id);
mb_mailbox_t* getMailboxByName(char* name);
void removeMailboxByID(int id);
void removeMailboxSubscription(int pid, mb_mailbox_t* mailbox);
void removePidReceivers(int pid, mb_message_t* message);

mb_message_t* get_last_message(mb_mailbox_t* mb);
void remove_msg (mb_message_t* msg, mb_message_t* msg_prv, mb_mailbox_t* mailbox);
mb_req_t* get_last_req(mb_mailbox_t* mb);
mb_user_t* get_owner_by_pid(int pid);
mb_user_t* get_user_by_pid(mb_user_t *first_user, int numb_users, int pid);

int do_mb_open() {

	char *name = m_in.m3_ca1;
	//Check if there is a name
	if (name == NULL || strlen(name) > MAX_LEN_NAME){
		return MB_NAME_ERROR;
	}

//Check if there is a mailbox with this name
	mb_mailbox_t* mbfound = NULL;
	mbfound = getMailboxByName(name);

	if (mbfound == NULL){
		return MB_ERROR;
	//If not, return id
	} else {
		mbfound->conn_process++;
		//printf("Mb coinc, returning id %d, name: %s, conn process: %d\n", mbfound->id, mbfound->name, mbfound->conn_process);
		return mbfound->id;
	}
}

int do_mb_close() {

	int id = (int)m_in.m1_i1;
	mb_mailbox_t* mbfound = NULL;
	register struct mproc *rmp = mp;
	mbfound = getMailboxByID(id);

	//If it exists
	if (mbfound != NULL){

		int my_pid = mproc[who_p].mp_pid;
		//Check if it is subscribed and unsubscribed it
		removeMailboxSubscription(my_pid, mbfound);

		//Check messages and mark as read it
		mb_message_t* prevmessage;
		mb_message_t* message;
		for (int i = 0; i < MAX_NUM_MESSAGES; i++) {
			if(message != NULL){
				//Check and delete reference
				removePidReceivers(my_pid, message);
			}else{
				break;
			}
			message=message->next;
		}

		//If there are more decrease, if not error. ONLY owner removes
		if (mbfound->conn_process > 0) {
			mbfound->conn_process--;
		} else if (mbfound->conn_process == 0) {
			return MB_CLOSE_ERROR;
		}
		return MB_OK;
	//If try to close an unexisting mailbox
	} else {
		return MB_CLOSE_ERROR;
	}
}

// usar malloc para text?
int do_mb_deposit() {
	int id = m_in.m1_i1;
	int num_recv = m_in.m1_i3;
	int len_msg = m_in.m1_i2;

	// Begin of critic area
	mb_mailbox_t* mailbox = getMailboxByID(id);
	if (mailbox == NULL)
		return MB_ERROR;
	if (mailbox->num_msg >= MAX_N_MSG)
		return MB_FULLMB_ERROR;

	//Check permission of user for this mailbox
	register struct mproc *rmp = mp;
	int my_pid = mproc[who_p].mp_pid;
	//Check if DENIED
	mb_user_t* rootDeniedUser = get_user_by_pid(mailboxes.first_denied_send_user, mailboxes.denied_send_users, my_pid);
	mb_user_t* ownerDeniedUser = get_user_by_pid(mailbox->first_denied_send_user, mailbox->denied_send_users, my_pid);
	if (rootDeniedUser != NULL || ownerDeniedUser != NULL)
		return MB_PERMISSION_ERROR;
	//Check if allowed (if private/secured)
	if (mailbox->mailbox_type == PRIVATE){
		mb_user_t* ownerAllowedUser = get_user_by_pid(mailbox->first_allowed_send_user, mailbox->allowed_send_users, my_pid);
		if(ownerAllowedUser==NULL)
			return MB_PERMISSION_ERROR;
	}


	/* Error handling */
	if (len_msg >= MAX_LEN_MSG)
		return MB_MSGTOOLONG_ERROR;
	if (num_recv >= MAX_N_REC)
		return MB_ERROR;

	/* Alloc memory and copy msg text and pids receivers from user process */
	int *pid_recv = malloc(num_recv * sizeof(int));
	char *text = malloc(len_msg + 1);
	if (pid_recv == NULL || text == NULL) {
		return MB_ALLOC_MEM_ERROR;
	}
	sys_datacopy(who_e, (vir_bytes)m_in.m1_p1,
					SELF, (vir_bytes)text, len_msg + 1);
	sys_datacopy(who_e, (vir_bytes)m_in.m1_p2,
					SELF, (vir_bytes)pid_recv, num_recv * sizeof(int));

	/* Create the message */
	int size_msg = sizeof(mb_message_t);
	mb_message_t *msg = malloc(size_msg);
	if (msg == NULL) return MB_ALLOC_MEM_ERROR;

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
	for (int i = 0; i < num_recv; i++) {
		mb_req_t* req = mailbox->first_req;
		for (int j = 0; j < mailbox->num_req; j++) {
			if (pid_recv[i] == req->pid) {
				if (check_sig(req->pid, req->signum, FALSE) == -1)
					return MB_ERROR;
			}
			req = req->next;
		}
	}
	// End of critic area
	return MB_OK;
}

int do_mb_retrieve() {
	int id = m_in.m1_i1;
	int buffer_len = m_in.m1_i2;
	char *buffer = m_in.m1_p1;

	/* Error Handling */
	// Begin of critic area
	mb_mailbox_t* mailbox = getMailboxByID(id);
	if (mailbox == NULL)
		return MB_ERROR;
	if (mailbox->num_msg == 0)
		return MB_EMPTYMB_ERROR;

	//Check permission of user for this mailbox
	register struct mproc *rmp = mp;
	int my_pid = mproc[who_p].mp_pid;
	//Check if DENIED
	mb_user_t* rootDeniedUser = get_user_by_pid(mailboxes.first_denied_retrieve_user, mailboxes.denied_retrieve_users, my_pid);
	mb_user_t* ownerDeniedUser = get_user_by_pid(mailbox->first_denied_retrieve_user, mailbox->denied_retrieve_users, my_pid);
	if (rootDeniedUser != NULL || ownerDeniedUser != NULL)
		return MB_PERMISSION_ERROR;
	//Check if allowed (if private/secured)
	if (mailbox->mailbox_type == PRIVATE){
		mb_user_t* ownerAllowedUser = get_user_by_pid(mailbox->first_allowed_retrieve_user, mailbox->allowed_retrieve_users, my_pid);
		if(ownerAllowedUser==NULL)
			return MB_PERMISSION_ERROR;
	}


	/* Search for messages */
	mb_message_t* msg_prv = NULL;
	mb_message_t* msg = mailbox->first_msg;
	for (int i = 0; i < mailbox->num_msg; i++) {
		// Recorrer los mensajes del mailbox y en cada mensaje los pid a los que va dirigido.
		int* list_pids = msg->receivers_pid;
		for (int j = 0; i < msg->num_rec; j++) {
			if (list_pids[j] == my_pid){
				if (strlen(msg->text) < buffer_len) {
					sys_datacopy(SELF, (vir_bytes)msg->text,
						who_e, (vir_bytes)buffer, strlen(msg->text) + 1);
					removePidReceivers(my_pid, msg);
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
	mb_mailbox_t* mailbox = getMailboxByID(id);
	if (mailbox == NULL)
		return MB_ERROR;
	if (mailbox->num_req >= MAX_N_REQ)
		return MB_FULNOTIFYLIST_ERROR;

	int size_req = sizeof(mb_req_t);
	mb_req_t *req = malloc(size_req);

	register struct mproc *rmp = mp;
	req->pid = mproc[who_p].mp_pid;
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

mb_message_t* get_last_message(mb_mailbox_t* mb) {
	mb_message_t *msg = mb->first_msg;
	if (msg == NULL)
		return NULL;
	for (int i = 0; i < mb->num_msg; i++) {
		if (msg->next == NULL)
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
	for (int i = 0; i < mb->num_req; i++) {
		if (req->next == NULL)
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
	free(msg->text);
	free(msg);
}

mb_mailbox_t* getMailboxByID(int id){
	mb_mailbox_t* previous=NULL;
	mb_mailbox_t* mb=NULL;

	if(mailboxes.first_mb != NULL){
		mb=mailboxes.first_mb;
		if (mb->id == id){
			return mb;
		}else{
			for (int i = 0; i < MAX_NUM_MAILBOXES-1; i++){
				previous = mb;
				mb = previous->next;
				if(mb != NULL){
					//If coincidence break
					if(mb->id == id){
						return mb;
					}
				}else{
					break;
				}
			}
		}

	}
	return NULL;
}

mb_mailbox_t* getMailboxByName(char* name){
	mb_mailbox_t* previous;
	mb_mailbox_t* mb;
	if(mailboxes.first_mb != NULL){
		mb=mailboxes.first_mb;
		if (strcmp(mb->name, name) == 0){
			return mb;
		}else{
			for (int i = 0; i < MAX_NUM_MAILBOXES-1; i++){
				previous=mb;
				mb=previous->next;
				if(mb != NULL){
					if(strcmp(mb->name, name) == 0){
						return mb;
					}
				}else{
					break;
				}
			}
		}

	}
	return NULL;
}


void removePidReceivers(int pid, mb_message_t* message){
	int num_receivers = message->num_rec;
	int* receivers = message->receivers_pid;

	int coincidence=-1;
	//Find the pid
	for(int i = 0; i < num_receivers; i++){
		if(receivers[i] == pid){
			coincidence = i;
			break;
		}
	}

	//Reform the array
	int j=0;
	int i=0;
	if(coincidence != -1){
		if (num_receivers == 1) {
			message->num_rec--;
			message->receivers_pid=NULL;
			free(receivers);
			return;
		}
		int *new_array = malloc((num_receivers-1)*sizeof(int));
		while(j < num_receivers && i < num_receivers-1){
			if(j != coincidence){
				new_array[i] = receivers[j];
				i++;
			}
			j++;
		}
		message->num_rec--;
		message->receivers_pid=new_array;
		free(receivers);
	}
}

void removeMailboxSubscription(int pid, mb_mailbox_t* mailbox){

	mb_req_t* previous;
	mb_req_t* present;
	int num_req = mailbox->num_req;

	present = mailbox->first_req;
	if(present!= NULL){
		//Find the pid
		for(int i = 0; i < num_req; i++){
			if(present->pid == pid){
				if(i == 0){
					mailbox->first_req = present->next;
				}
				else{
					previous->next = present->next;
				}
			free(present);
			break;
			}
		previous = present;
		present = previous->next;
		}
	}
}

void removeMailboxByID(int id){

	mb_mailbox_t* previous;
	mb_mailbox_t* present;
	int num_mail = mailboxes.num_mbs;

	present = mailboxes.first_mb;
	if(present!= NULL){
		//Find the pid
		for(int i = 0; i < num_mail; i++){
			if(present->id == id){
				if(i == 0){
					mailboxes.first_mb = present->next;
				} else {
					previous->next = present->next;
				}
				free(present);
				break;
			}
			previous = present;
			present = previous->next;
		}
	}
}

int do_mb_be_root() {
	return 0;
}

int do_mb_assign_leader() {
	return 0;
}

int do_mb_remove_leader() {
	return 0;
}

int do_mb_root_deny_send() {
	return 0;
}

int do_mb_root_deny_retrieve() {
	return 0;
}

int do_mb_root_allow_send() {
	return 0;
}

int do_mb_root_allow_retrieve() {
	return 0;
}

int do_mb_create_secure_mailbox() {
	return 0;
}

int do_mb_create_public_mailbox() {
	return 0;
}

int do_mb_deny_send() {
	return 0;
}

int do_mb_deny_retrieve() {
	return 0;
}

int do_mb_allow_send() {
	return 0;
}

int do_mb_allow_retrieve() {
	return 0;
}

int do_mb_remove_group() {
	int id = m_in.m1_i1;

	//Check if the mailbox exists
	mb_mailbox_t* mailbox = getMailboxByID(id);
	if (mailbox == NULL)
		return MB_ERROR;

	//Check permission
	register struct mproc *rmp = mp;
	int my_pid = mproc[who_p].mp_pid;
	//Check if OWNER
	mb_user_t* owner = get_owner_by_pid(my_pid);
	if(owner == NULL)
		return MB_PERMISSION_ERROR;

	removeMailboxByID(id);


}

int do_mb_rmv_oldest_msg() {
	int id = m_in.m1_i1;

	//Check if the mailbox exists
	mb_mailbox_t* mailbox = getMailboxByID(id);
	if (mailbox == NULL)
		return MB_ERROR;

	//Check permission
	register struct mproc *rmp = mp;
	int my_pid = mproc[who_p].mp_pid;
	//Check if OWNER
	mb_user_t* owner = get_owner_by_pid(my_pid);
	if(owner == NULL)
		return MB_PERMISSION_ERROR;

	mb_message_t* oldest = mailbox->first_msg;
	mailbox->first_msg = oldest->next;
}

mb_user_t* get_owner_by_pid(int pid) {
	mb_user_t* previous = NULL;
	mb_user_t* user = NULL;

	if(mailboxes.first_owner != NULL){
		user = mailboxes.first_owner;
		if (user->pid == pid){
			return user;
		}else{
			for (int i = 0; i < mailboxes.num_owners-1; i++){
				previous = user;
				user = previous->next;
				if(user != NULL){
					//If coincidence break
					if(user->pid == pid){
						return user;
					}
				}else{
					break;
				}
			}
		}

	}
	return NULL;
}

mb_user_t* get_user_by_pid(mb_user_t *first_user, int numb_users, int pid) {
	mb_user_t* previous = NULL;
	mb_user_t* user = NULL;

	if(first_user != NULL){
		user = first_user;
		if (user->pid == pid){
			return user;
		}else{
			for (int i = 0; i < numb_users-1; i++){
				previous = user;
				user = previous->next;
				if(user != NULL){
					//If coincidence break
					if(user->pid == pid){
						return user;
					}
				}else{
					break;
				}
			}
		}

	}
	return NULL;
}
