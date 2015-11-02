#include "mailbox.h"
#include "mblib.h"
#include "pm.h"
#include <stdlib.h>

#define MAX_NUM_MESSAGES 16
#define MAX_NUM_MAILBOXES 10


#define MB_NAME_ERROR -9
#define MB_CLOSE_ERROR -10

mb_mbs_t mailboxes;

int do_mb_open() {
	char* name = (char*)m1_in->m1_p1;

//Check if there is a name
	if (name == NULL){
		return MB_NAME_ERROR;
	}

//Check if mailboxes is initialized
	if (mailboxes == NULL ){
		mailboxes->first_mb=NULL;
		mailboxes->num_mbs=0;
		mailboxes->id_master=0;
	}

//Check if there is a mailbox with this name
	mb_mailbox_t previous=NULL;
	mb_mailbox_t mb=NULL;
	if(mailboxes->first_mb!=NULL){
		mbfound=mailboxes->first_mb;
		if (mbfound->name==name){
			return *mbfound;
		}else{
			for (int i=0; i<MAX_NUM_MAILBOXES-1; i++){
				*previous=*mbfound;
				*mbfound=previous->next;
				if(mbfound != NULL){
					if(mbfound->name==name){
						break;
					}
				}else{
					break;
				}
			}
		}
		
	}
	//If null, create it
	if (mbfound == NULL){
		if(mailboxes->num_mbs < MAX_NUM_MAILBOXES){
			mb_mailbox_t *mailbox=malloc(sizeof(mb_mailbox_t));
			mailbox->name=name;

			//Increase the id master counter
			mailboxes->id_master++;
			mailbox->id=mailboxes->id_master;

			mailbox->first_msg=NULL;
			mailbox->num_msg=0;
			mailbox->first_req=NULL;
			mailbox->num_req=0;
			mailbox->conn_process=1;
			mailbox->next=NULL;

			//Increase mailbox counter and add new mailbox
			mailboxes->num_mbs++;
			mailboxes->first_mb=*mailbox;
			return mailbox->id;
		//Max size exceed, return error
		}else{
			return MB_MAXMB_ERROR;
		}
		
	//If not, return id
	}else{
		return mbfound->id;
	}


}

int do_mb_close() {

	int* id=(int*)m1_in->m1_i1;
	mb_mailbox_t* previous=NULL;
	mb_mailbox_t mb=NULL;
	mb_mailbox_t* mbfound=NULL;

	if(mailboxes->first_mb!=NULL){
		mb=mailboxes->first_mb;
		if (mb->id==id){
			return *mb;
		}else{
			for (int i=0; i<MAX_NUM_MAILBOXES-1; i++){
				*previous=*mb;
				*mb=previous->next;
				if(mb != NULL){
					//If coincidence break
					if(mb->id==id){
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
		int my_pid = getpid();
		//TODO:Check if it is subscribed and unsubscribed it


		//Check messages and mark as read it
		mb_message_t* prevmessage;
		mb_message_t* message;
		for (int i=0; i<MAX_NUM_MESSAGES; i++){
			if(message != NULL){
				//Check and delete reference
				removePidReceivers(my_pid, *message);
			}else{
				break;
			}
			*message=message->next;
		}


		//If there are more processes connected discount one process
		if(mbfound->conn_process>1){
			mbfound->conn_process--;
			
		}else{
		//It is the only waiting, so destroy the mailbox
			//Link the previous mailbox with the next to this
			previous->next=mbfound->next;

			//Free space
			free(mbfound->name);
			free(mbfound->id);
			free(mbfound->first_msg);
			free(mbfound->num_msg);
			free(mbfound->first_req);
			free(mbfound->num_req);
			free(mbfound->conn_process);
			free(mbfound->next);
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

 getMailboxByName(char* name){
	
	return NULL;	
}

void removePidReceivers(int pid, mb_message_t* message){
	int* num_receivers = message->num_rec;
	int receivers[num_receivers] = message->receivers_pid;

	int coincidence=-1;
	//Find the pid
	for(int i=0; i<num_receivers; i++){
		if(receivers[i]==pid){
			coincidence=receivers[i];
			break;
		}
	}

	//Reform the array
	int j=0;
	int i=0;
	int new_array[num_receivers-1];
	if(coincidence != -1){
		while(j<num_receivers-1){
			if(j!=coincidence){
				new_array[i]=receivers[j];
			}				

			j++;
			i++;
		}
		message->num_rec--;
		message->receivers_pid=*new_array;	
	}
	

}
