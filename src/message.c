#include <pebble.h>
#include "message.h"

//Message queueing structure
struct msgStack{
  uint8_t buffer[DICT_SIZE];
  struct msgStack * next;
};
struct msgStack * messageStack = NULL;

//adds a new message to the queue
void add_message(uint8_t dictBuf[DICT_SIZE]){
  struct msgStack ** index = &messageStack;
  while(*index != NULL){
    index = &((*index)->next);
  }
  //allocate message
  *index = malloc(sizeof(struct msgStack));
  if(*index == NULL){
    APP_LOG(APP_LOG_LEVEL_ERROR,"Out of memory!");
    return;
  }
  //copy over dictionary buffer
  memcpy((*index)->buffer, dictBuf, DICT_SIZE);
  (*index)->next = NULL;
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Added message to queue");
}

//Sends the first message in the queue
void send_message(){
  if(messageStack == NULL)return;
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Retrieved message from queue");
  DictionaryIterator read,*send;
  app_message_outbox_begin(&send);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Copying message data");
  dict_read_begin_from_buffer(send, messageStack->buffer, DICT_SIZE);
  uint32_t size = OUTBOX_SIZE;
  Tuple * req = dict_find(send,0);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Request is %s",req->value->cstring);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Attempting dictionary merge");
  DictionaryResult result =dict_merge(send,&size,&read,false,0,0);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Dictionary size out is %d",(int)size);
  if(result == DICT_OK) app_message_outbox_send();
  else if(result == DICT_INVALID_ARGS){
    APP_LOG(APP_LOG_LEVEL_ERROR,"Dict merge failed, invalid args");
  }else if(result == DICT_NOT_ENOUGH_STORAGE){
    APP_LOG(APP_LOG_LEVEL_ERROR,"Dict merge failed, not enough storage");
  }
}

//Removes the first message in the queue
void delete_message(){
  if(messageStack != NULL){
    struct msgStack * old = messageStack;
    messageStack = messageStack->next;
    free(old);
  }
}

//Removes all messages in the queue
void delete_all_messages(){
  while(messageStack != NULL){
    delete_message();
  }
}

//return 1 if the queue is empty, 0 if messages remain
int message_queue_empty(){
  if(messageStack == NULL)return 1;
  return 0;
}