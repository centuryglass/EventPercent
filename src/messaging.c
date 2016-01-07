#include <pebble.h>
#include "messaging.h"
#include "display.h"
#include "events.h"
#include "util.h"
#include "keys.h"

//----------LOCAL VALUE DEFINITIONS----------
#define DEBUG 0 //Set to 1 to enable messaging debug logging

//----------MESSAGE STACK STRUCTURE----------
struct msgStack{
  uint8_t buffer[DICT_SIZE];//Message dictionary buffer
  struct msgStack * next;//Link to the next message
};

//----------STATIC VARIABLES----------
static struct msgStack * messageStack = NULL; //Unsent message stack
static int init = 0;//Equals 1 iff messaging_init has been run

//----------STATIC FUNCTION DECLARATIONS----------
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

/**
*Automatically called whenever a message is recieved
*@param iterator the message data iterator
*@param context is required but unused
*@post message data is read and processed
*/
static void inbox_received_callback(DictionaryIterator *iterator, void *context);

/**
*Automatically called whenever recieving a message fails
*@param reason the failure reason
*@param context is required but unused
*@post new update data is requested
*/
static void inbox_dropped_callback(AppMessageResult reason, void *context);

/**
*Automatically calls whenever sending a message fails
*@param iterator data from the failed send
*@param reason the failure reason
*@param context is required but unused
*@post the message is re-sent
*/
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context);

/**
*Automatically calls whenever sending a message succeeds
*@param iterator data from the sent message
*@param context is required but unused
*@post the sent message is removed from the message queue
*/
static void outbox_sent_callback(DictionaryIterator *iterator, void *context);

/**
*Given an error appMessageResult, prints debug data explaining the result
*@param result the message result
*/
static void log_result_info(AppMessageResult result);

//----------PUBLIC FUNCTIONS----------
//Initializes AppMessage functionality
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

//Shuts down AppMessage functionality
void messaging_deinit(){
  if(init){
    while(messageStack != NULL){
      delete_message();
    }
    init = 0;
  }
}

//Requests updated event info from the companion app
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
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"request_event_updates:Attempting to send request");
  send_message();
}

//Requests updated battery info from the companion app
void request_battery_update(){
  if(!init)messaging_init();
  uint8_t buf[DICT_SIZE];
  DictionaryIterator iter;
  dict_write_begin(&iter,buf,DICT_SIZE);
  dict_write_cstring(&iter, KEY_BATTERY_UPDATE, "update");
  dict_write_end(&iter);
  add_message(buf);
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"request_battery_update:Attempting to send request");
  send_message();
}

//----------STATIC FUNCTIONS----------
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
    if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"delete_message:old message deleted");
  }
}

//Automatically called whenever a message is recieved
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Message received");
  Tuple *request = dict_find(iterator,KEY_REQUEST);
  Tuple *battery = dict_find(iterator,KEY_BATTERY_UPDATE);

  if(request != NULL){
    if(strcmp(request->value->cstring,"Event sent") == 0){
      if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Recieved event data");
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
      update_event_display();
    }
    else if(strcmp(request->value->cstring,"Color Update") == 0){
      if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Recieved color data");
      int i;
      char newColors [NUM_COLORS][7];
      for(i=0;i<NUM_COLORS;i++){
        Tuple * color = dict_find(iterator,KEY_COLORS_BEGIN+i);
        if(color != NULL){
          strcpy(newColors[i],color->value->cstring);
        }else strcpy (newColors[i],"FF0000");//default to white, this should never happen
      }
      update_colors(newColors);
    }  
  }
  if(battery != NULL){
    if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Getting battery info"); 
    update_phone_battery(battery->value->cstring);
  }
}

//Automatically called whenever recieving a message fails
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  if(DEBUG){
    APP_LOG(APP_LOG_LEVEL_ERROR, "inbox_dropped_callback:Message dropped");
    log_result_info(reason);
    APP_LOG(APP_LOG_LEVEL_ERROR, "inbox_dropped_callback:Requesting new updates");
  }
  request_event_updates();
}

//Automatically calls whenever sending a message fails
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  if(DEBUG){
    APP_LOG(APP_LOG_LEVEL_ERROR, "outbox_failed_callback:Outbox send failed, attempting to re-send");
    log_result_info(reason);
    APP_LOG(APP_LOG_LEVEL_ERROR, "outbox_failed_callback:Attempting to re-send message");
  }
  send_message();
}

//Automatically calls whenever sending a message succeeds
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_INFO, "outbox_sent_callback:Outbox send success!");
  delete_message();
}

//Given an error appMessageResult, prints debug data explaining the result
static void log_result_info(AppMessageResult result){
    switch(result){
      case APP_MSG_OK:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error code: APP_MSG_OK");
        break;
      case APP_MSG_SEND_TIMEOUT:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error code: APP_MSG_SEND_TIMEOUT");
        break;
      case APP_MSG_SEND_REJECTED:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error code: APP_MSG_SEND_REJECTED");
        break;
      case APP_MSG_NOT_CONNECTED:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error code: APP_MSG_NOT_CONNECTED");
        break;
      case APP_MSG_APP_NOT_RUNNING:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error code: APP_MSG_APP_NOT_RUNNING");
        break;
      case APP_MSG_INVALID_ARGS:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error code: APP_MSG_INVALID_ARGS");
        break;
      case APP_MSG_BUSY:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error code: APP_MSG_BUSY");
        break;
      case APP_MSG_BUFFER_OVERFLOW:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error code: APP_MSG_BUFFER_OVERFLOW");
        break;
      case APP_MSG_ALREADY_RELEASED:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error code: APP_MSG_ALREADY_RELEASED");
        break;
      case APP_MSG_CALLBACK_ALREADY_REGISTERED:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error code: APP_MSG_CALLBACK_ALREADY_REGISTERED");
        break;
      case APP_MSG_CALLBACK_NOT_REGISTERED:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error code: APP_MSG_CALLBACK_NOT_REGISTERED");
        break;
      case APP_MSG_OUT_OF_MEMORY:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error code: APP_MSG_OUT_OF_MEMORY");
        break;
      case APP_MSG_CLOSED:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error code: APP_MSG_CLOSED");
        break;
      case APP_MSG_INTERNAL_ERROR:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error code: APP_MSG_INTERNAL_ERROR");
        break;
      case APP_MSG_INVALID_STATE:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error code: APP_MSG_INVALID_STATE");
        break;
  }
  
}