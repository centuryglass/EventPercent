#include <pebble.h>
#include "messaging.h"
#include "display.h"
#include "events.h"
#include "util.h"
#include "keys.h"

//LOCAL VALUE DEFINITIONS
#define DEBUG 0 //set to 1 to enable messaging debug logging

//MESSAGE STACK STRUCTURE
struct msgStack{
  uint8_t buffer[DICT_SIZE];
  struct msgStack * next;
};

//STATIC VARIABLES
static struct msgStack * messageStack = NULL; //Unsent message stack
static int init = 0;//Equals 1 iff messaging_init has been run

//STATIC FUNCTION DECLARATIONS
/**
*adds a new message to the queue
*param dictBuf a dictionary buffer containing the new message
*/
static void add_message(uint8_t dictBuf[DICT_SIZE]);
/**
*Sends the first message in the queue
*/
static void send_message();
/**
*Removes the first message in the queue
*/
static void delete_message();
//AppMessage callback functions
static void inbox_received_callback(DictionaryIterator *iterator, void *context);
static void inbox_dropped_callback(AppMessageResult reason, void *context);
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context);
static void outbox_sent_callback(DictionaryIterator *iterator, void *context);

//PUBLIC FUNCTIONS
/**
*Initializes AppMessage functionality
*/
void messaging_init(){
  if(!init){
    // Register AppMessage callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    // Open AppMessage
    app_message_open(DICT_SIZE,DICT_SIZE);
    init = 1;
  }
}
/**
*Shuts down AppMessage functionality
*/
void messaging_deinit(){
  if(init){
    while(messageStack != NULL){
      delete_message();
    }
    init = 0;
  }
}
/**
*Requests updated event info from the companion app
*/
void request_event_updates(){
  if(!init)messaging_init();
  uint8_t buf[DICT_SIZE];
  DictionaryIterator iter;
  //request all events:
  dict_write_begin(&iter,buf,DICT_SIZE);
  dict_write_cstring(&iter, KEY_REQUEST, "Event request");
  dict_write_cstring(&iter, KEY_BATTERY_UPDATE, "update");
  dict_write_int32(&iter,KEY_EVENT_NUM,NUM_EVENTS);
  dict_write_end(&iter);
  add_message(buf);
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"Attempting to send message");
  send_message();
}
/**
*Requests updated battery info from the companion app
*/
void request_battery_update(){
  if(!init)messaging_init();
  uint8_t buf[DICT_SIZE];
  DictionaryIterator iter;
  dict_write_begin(&iter,buf,DICT_SIZE);
  dict_write_cstring(&iter, KEY_BATTERY_UPDATE, "update");
  dict_write_end(&iter);
  add_message(buf);
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"Attempting to send message");
  send_message();
}

//STATIC FUNCTIONS
/**
*adds a new message to the queue
*/
static void add_message(uint8_t dictBuf[DICT_SIZE]){
  if(!init)messaging_init();
  if(DEBUG){
    APP_LOG(APP_LOG_LEVEL_DEBUG,"add_message:New message content:");
    DictionaryIterator read;
    dict_read_begin_from_buffer(&read, dictBuf, DICT_SIZE);
    debugDictionary(&read);
  }
  
  struct msgStack ** index = &messageStack;
  while(*index != NULL){
    index = &((*index)->next);
  }
  //allocate message
  *index = malloc(sizeof(struct msgStack));
  if(*index == NULL){
    APP_LOG(APP_LOG_LEVEL_ERROR,"add_message:Out of memory!");
    return;
  }
  //copy over dictionary buffer
  memcpy((*index)->buffer, dictBuf, DICT_SIZE);
  (*index)->next = NULL;
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"add_message:Added message to queue");
}
/**
*Sends the first message in the queue
*/
static void send_message(){
  if(!init)messaging_init();
  if(messageStack == NULL)return;
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Retrieved message from queue");
  DictionaryIterator read,*send;
  app_message_outbox_begin(&send);
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Copying message data");
  Tuple * dictItem = dict_read_begin_from_buffer(&read, messageStack->buffer, DICT_SIZE);
  while(dictItem != NULL){
    uint32_t key = dictItem->key;
    uint16_t size = dictItem->length;
    if(dictItem->type == TUPLE_BYTE_ARRAY){
      if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Writing byte array for key %d",(int)key);
      dict_write_data(send,key,dictItem->value->data,size);
    }
    else if(dictItem->type == TUPLE_CSTRING){
      if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Writing string %s for key %d"
                       ,dictItem->value->cstring,(int)key);
      dict_write_cstring(send, key, dictItem->value->cstring);
    }
    if(dictItem->type == TUPLE_UINT){
      uint32_t  val = dictItem->value->uint32;
      if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Writing int for key %d",(int)key);
      dict_write_int(send,key,(int *)&val,size,false);
    }
    if(dictItem->type == TUPLE_INT){
      if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Writing int for key %d",(int)key);
      int32_t val = dictItem->value->int32;
      dict_write_int(send,key,(int *)&val,size,true);
    }  
    dictItem = dict_read_next(&read);
  }
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Sending message");
  if(DEBUG)debugDictionary(&read);
  app_message_outbox_send();
}
/**
*Removes the first message in the queue
*/
static void delete_message(){
  if(!init)messaging_init();
  if(messageStack != NULL){
    struct msgStack * old = messageStack;
    messageStack = messageStack->next;
    free(old);
  }
}
//AppMessage callback functions
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"Message received");
  Tuple *request = dict_find(iterator,KEY_REQUEST);
  Tuple *battery = dict_find(iterator,KEY_BATTERY_UPDATE);

  if(request != NULL){
    if(strcmp(request->value->cstring,"Event sent") == 0){
      Tuple *title = dict_find(iterator,KEY_EVENT_TITLE);
      Tuple *start = dict_find(iterator,KEY_EVENT_START);
      Tuple *end = dict_find(iterator,KEY_EVENT_END);
      Tuple *color = dict_find(iterator,KEY_EVENT_COLOR);
      Tuple *num = dict_find(iterator,KEY_EVENT_NUM);  
      if((title != NULL)&&(start != NULL)&&(end != NULL)&&
         (color != NULL)&&(num != NULL)){
        add_event(num->value->int16,
                 title->value->cstring,
                 start->value->int32,
                 end->value->int32,
                 color->value->cstring);   
      }
    }
    update_event_display();
  }
  if(battery != NULL){
    if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"Getting battery info"); 
    update_phone_battery(battery->value->cstring);
  }
  send_message();
}
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
  request_event_updates();
}
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
  send_message();
}
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
  delete_message();
}