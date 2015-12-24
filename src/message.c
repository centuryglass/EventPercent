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
  //APP_LOG(APP_LOG_LEVEL_DEBUG,"Retrieved message from queue");
  DictionaryIterator read,*send;
  app_message_outbox_begin(&send);
  //APP_LOG(APP_LOG_LEVEL_DEBUG,"Copying message data");
  Tuple * dictItem = dict_read_begin_from_buffer(&read, messageStack->buffer, DICT_SIZE);
  while(dictItem != NULL){
    uint32_t key = dictItem->key;
    uint16_t size = dictItem->length;
    if(dictItem->type == TUPLE_BYTE_ARRAY){
      dict_write_data(send,key,dictItem->value->data,size);
    }
    else if(dictItem->type == TUPLE_CSTRING){
      dict_write_cstring(send, key, dictItem->value->cstring);
    }
    if(dictItem->type == TUPLE_UINT){
      uint32_t  val = dictItem->value->uint32;
      dict_write_int(send,key,(int *)&val,size,false);
    }
    if(dictItem->type == TUPLE_INT){
      int32_t val = dictItem->value->int32;
      dict_write_int(send,key,(int *)&val,size,true);
    }  
    dictItem = dict_read_next(&read);
  }
  //APP_LOG(APP_LOG_LEVEL_DEBUG,"Sending message");
  //debugDictionary(&read);
  app_message_outbox_send();
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

