#include "mailbox.h"
#include "mblib.h"
#include "pm.h"

#define MAX_NUM_MESSAGES 16
#define MAX_NUM_MAILBOXES 10


#define MB_NAME_ERROR -8
#define MB_CLOSE_ERROR -9

mb_mbs_t mailboxes;

int do_mb_open() {
	char* name = (char*)m1_in->m1_p1;

//Check if there is a name
	if (name == NULL){
		return MB_NAME_ERROR;
	}

//Check if mailboxes is initialized
	if (mailboxes == NULL ){
		mailboxes.first_mb=NULL;
		mailboxes.num_mbs=0;
		mailboxes.id_master=0;
	}

//Check if there is a mailbox with this name
	mb_mailbox_t *mbfound = getMailboxByName(*name);
	//If null, create it
	if (mbfound == NULL){
		if(mailboxes.num_mbs < MAX_NUM_MAILBOXES){
			mb_mailbox_t *mailbox=malloc(sizeof(mb_mailbox_t));
			mailbox.name=name;

			//Increase the id master counter
			mailboxes.id_master++;
			mailbox.id=mailboxes.id_master;

			mailbox.first_msg=NULL;
			mailbox.num_msg=0;
			mailbox.first_req=NULL;
			mailbox.num_req=0;
			mailbox.conn_process=1;
			mailbox.next=NULL;

			//Increase mailbox counter and add new mailbox
			mailboxes.num_mbs++;
			mailboxes.first_mb=*mailbox;
			return mailbox.id;
		//Max size exceed, return error
		}else{
			return MB_MAXMB_ERROR;
		}
		
	//If not, return id
	}else{
		return mbfound.id;
	}


}

int do_mb_close() {

	int* id=(int*)m1_in.m1_i1;
	mb_mailbox_t* previous=NULL;
	mb_mailbox_t mb=NULL;
	mb_mailbox_t* mbfound=NULL;

	if(mailboxes.first_mb!=NULL){
		mb=mailboxes.first_mb;
		if (mb.id==id){
			return *mb;
		}else{
			for (int i=0; i<MAX_NUM_MAILBOXES-1; i++){
				*previous=*mb;
				*mb=previous.next;
				if(mb != NULL){
					//If coincidence break
					if(mb.id==id){
						*mbfound= *mb;
						break;
					}
				}else{
					break;
				}
			}
		}
		
	}

	//If it exists
	if (mbfound!=NULL){
		//Check if it is subscribed and unsubscribed it


		//Check messages and mark as read it
		mb_message_t* prevmessage;
		mb_message_t* message;
		for (int i=0; i<MAX_NUM_MESSAGES; i++){
			*previousmessage=*message;
			*message=previous.next;
			if(message != NULL){
				//If coincidence break
				if(mb.id==id){
					*mbfound= *mb;
					break;
				}
			}else{
				break;
			}
		}


		//If there are more processes connected discount one process
		if(mbfound.conn_process>1){
			mbfound.conn_process--;
			
		}else{
		//It is the only waiting, so destroy the mailbox
			//Link the previous mailbox with the next to this
			previous.next=mbfound.next;

			//Free space
			free(mbfound.name);
			free(mbfound.id);
			free(mbfound.first_msg);
			free(mbfound.num_msg);
			free(mbfound.first_req);
			free(mbfound.num_req);
			free(mbfound.conn_process);
			free(mbfound.next);
			free(mbfound);

		}
		return MB_OK;
	//If try to close an unexisting mailbox
	}else{
		return MB_CLOSE_ERROR;
	}
	
}

int do_mb_deposit() {

}

int do_mb_retrieve() {

}

int do_mb_reqnot() {

}

mb_mailbox_t getMailboxByName(char* name){
	mb_mailbox_t previous=NULL;
	mb_mailbox_t mb=NULL;
	if(mailboxes.first_mb!=NULL){
		mb=mailboxes.first_mb;
		if (mb.name==name){
			return *mb;
		}else{
			for (int i=0; i<MAX_NUM_MAILBOXES-1; i++){
				*previous=*mb;
				*mb=previous.next;
				if(mb != NULL){
					if(mb.name==name){
						return *mb;
					}
				}else{
					break;
				}
			}
		}
		
	}
	return NULL;	
}

