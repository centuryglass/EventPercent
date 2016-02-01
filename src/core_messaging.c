#include <pebble.h>
#include "core_messaging.h"
#include "util.h"

//----------LOCAL VALUE DEFINITIONS----------
//#define DEBUG_MESSAGING //Uncomment to enable messaging debug logging
#define RESEND_TIME 60000 //Time to wait before assuming an outgoing message was lost

//----------MESSAGE STACK STRUCTURE----------

typedef struct msgStack{
  uint8_t buffer[DICT_SIZE];//Message dictionary buffer
  struct msgStack * next;//Link to the next message
}MessageStack;

//----------LOCAL VARIABLES----------
MessageStack * messageStack = NULL; //Unsent message stack
int messageStackSize = 0;//Number of messages on the stack
bool init = false;//Equals 1 iff messaging_init has been run

bool sendingMessage = false;//tracks the state of the message sending process
AppTimer * resend_timer = NULL;//Time until an ignored message should be re-sent

InboxHandler inbox_handler = NULL;//Function incoming messages are handed off to

//----------STATIC FUNCTION DECLARATIONS----------
static void delete_message();
  //Removes the first message in the queue
static void send_message();
  //Sends the first message in the queue
static void resend_message(void * data);
  //Re-sends an ignored message 
static void inbox_received_callback(DictionaryIterator *iterator, void *context);
  //Automatically called whenever a message is recieved
static void inbox_dropped_callback(AppMessageResult reason, void *context);
  //Automatically called whenever recieving a message fails
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context);
  //Automatically calls whenever sending a message fails
static void outbox_sent_callback(DictionaryIterator *iterator, void *context);
  //Automatically calls whenever sending a message succeeds
static void log_result_info(AppMessageResult result);
  //Given an error appMessageResult, prints debug data explaining the result

//Initialize messaging and open AppMessage
void open_messaging(){
  //Register AppMessage callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  // Open AppMessage
  app_message_open(DICT_SIZE,DICT_SIZE);
  init = true;
}

//Free up memory and disable messaging
void close_messaging(){
  if(init){
    //delete remaining messages
    while(messageStack != NULL){
      delete_message();
    }
    app_message_deregister_callbacks();
  }
  init = false;
}

//Registers a function to pass incoming messages
void register_inbox_handler(InboxHandler handler){
  inbox_handler = handler;
}

/**
*adds a new message to the queue
*param dictBuf a dictionary buffer containing the new message
*/
void add_message(uint8_t dictBuf[DICT_SIZE]){
  if(!connection_service_peek_pebble_app_connection()) return;//disable messaging if not connected
  if(!init)open_messaging();
  if(messageStackSize > 10){
    APP_LOG(APP_LOG_LEVEL_ERROR,"add_message:Too many messages! Deleting old messages");
    while(messageStackSize >0) delete_message();
  }
  #ifdef DEBUG_MESSAGING
    APP_LOG(APP_LOG_LEVEL_DEBUG,"add_message:New message content:");
    DictionaryIterator read;
    dict_read_begin_from_buffer(&read, dictBuf, DICT_SIZE);
    debugDictionary(&read);
  #endif
  
  MessageStack ** index = &messageStack;
  while(*index != NULL){
    index = &((*index)->next);
  }
  //allocate message
  *index = malloc(sizeof(MessageStack));
  if(*index == NULL){
    APP_LOG(APP_LOG_LEVEL_ERROR,"add_message:Out of memory!");
    return;
  }
  //copy over dictionary buffer
  memcpy((*index)->buffer, dictBuf, DICT_SIZE);
  (*index)->next = NULL;
  messageStackSize++;
  #ifdef DEBUG_MESSAGING
    APP_LOG(APP_LOG_LEVEL_DEBUG,"add_message:Added message to queue");
  #endif
  //Send the message now, if message sending isn't already in progress
  if(!sendingMessage) send_message();
}

/**
*Sends the first message in the queue
*/
static void send_message(){
  if(!connection_service_peek_pebble_app_connection()) return;//disable messaging if not connected
  if(!init)open_messaging();
  if(messageStack == NULL)return;
  sendingMessage = true;
  #ifdef DEBUG_MESSAGING
    APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Retrieved message from queue");
  #endif
  DictionaryIterator read,*send;
  app_message_outbox_begin(&send);
  #ifdef DEBUG_MESSAGING
    APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Copying message data");
  #endif
  Tuple * dictItem = dict_read_begin_from_buffer(&read, messageStack->buffer, DICT_SIZE);
  while(dictItem != NULL && dictItem->length > 0){
    uint32_t key = dictItem->key;
    uint16_t size = dictItem->length;
    switch(dictItem->type){
      case TUPLE_BYTE_ARRAY:
        #ifdef DEBUG_MESSAGING
          APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Writing byte array for key %d",(int)key);
        #endif
        dict_write_data(send,key,dictItem->value->data,size);
        break;
      case TUPLE_CSTRING:
        #ifdef DEBUG_MESSAGING
          APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Writing string %s for key %d"
                       ,dictItem->value->cstring,(int)key);
        #endif
        dict_write_cstring(send, key, dictItem->value->cstring);
        break;
      case TUPLE_UINT:
      {
        uint32_t val = dictItem->value->uint32;
        #ifdef DEBUG_MESSAGING
          APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Writing int for key %d",(int)key);
        #endif
        dict_write_int(send,key,(int *)&val,size,false);
        break;
      }
      case TUPLE_INT:
      {
        #ifdef DEBUG_MESSAGING
          APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Writing int for key %d",(int)key);
        #endif
        int32_t val = dictItem->value->int32;
        dict_write_int(send,key,(int *)&val,size,true);
      }   
    }
    dictItem = dict_read_next(&read);
  }
  
  #ifdef DEBUG_MESSAGING
    APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Sending message");
    debugDictionary(&read);
  #endif
  app_message_outbox_send();
  //Set a timer to re-send the message if it is ignored
  if(resend_timer == NULL)
    resend_timer = app_timer_register(RESEND_TIME,resend_message,NULL);
}

/**
*Removes the first message in the queue
*/
static void delete_message(){
  if(!init)open_messaging();
  if(messageStack != NULL){
    MessageStack * old = messageStack;
    messageStack = messageStack->next;
    free(old);
    messageStackSize--;
    #ifdef DEBUG_MESSAGING
      APP_LOG(APP_LOG_LEVEL_DEBUG,"delete_message:old message deleted");
    #endif
  }
}

/**
*Re-sends an ignored message 
*@param data: unused callback data
*/
static void resend_message(void * data){
  resend_timer = NULL;
  send_message();
}
  
/**
*Automatically called whenever a message is recieved
*@param iterator the message data iterator
*@param context is required but unused
*@post message data is read and processed
*/
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO,"inbox_received_callback:Message received");
  #ifdef DEBUG_MESSAGING
    APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Received message content:");
    debugDictionary(iterator);
  #endif
  //pass message to the message handler
  if(inbox_handler != NULL)inbox_handler(iterator);  
}

/**
*Automatically called whenever recieving a message fails
*@param reason the failure reason
*@param context is required but unused
*/
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "inbox_dropped_callback:Message dropped");
    log_result_info(reason);
}

/**
*Automatically calls whenever sending a message fails
*@param iterator data from the failed send
*@param reason the failure reason
*@param context is required but unused
*@post the message is re-sent
*/
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "outbox_failed_callback:Outbox send failed");
    log_result_info(reason);
    //reset timer to give Android more time to receive the message before re-sending
    if(resend_timer != NULL){
      app_timer_reschedule(resend_timer, RESEND_TIME);
    }
}

/**
*Automatically calls whenever sending a message succeeds
*@param iterator data from the sent message
*@param context is required but unused
*@post the sent message is removed from the message queue
*/
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "outbox_sent_callback:Outbox send success!");
  app_timer_cancel(resend_timer);
  resend_timer = NULL;//cancel re-send timer
  delete_message();//delete successfully sent message
  sendingMessage = false;
  send_message();//send the next message in the queue, if there is one
}


/**
*Given an error appMessageResult, prints debug data explaining the result
*@param result the message result
*/
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
