#include <pebble.h>
#include "messaging.h"
#include "display.h"
#include "events.h"
#include "util.h"
#include "storage_keys.h"

//----------LOCAL VALUE DEFINITIONS----------
//#define DEBUG_MESSAGING //Uncomment to enable messaging debug logging
#define DEFAULT_UPDATE_FREQ 300 //default update frequency(seconds)

//----------APPMESSAGE KEY DEFINITIONS----------
enum{
  KEY_MESSAGE_CODE,
    //uint8: identifying the purpose of the message, bi-directional
  KEY_BATTERY_UPDATE,
    //uint8: containing the sender's battery life percentage, bi-directional
  KEY_EVENT_TITLE,
    //cstring: an event's title string, sent from Android
  KEY_EVENT_START,
    //uint16: event start time, sent from Android
  KEY_EVENT_END,
    //uint16: event end time, sent from Android
  KEY_EVENT_COLOR,
    //cstring: event color, sent from Android
  KEY_EVENT_NUM,
    //uint8: event index number, sent from Android
  KEY_INFOTEXT,
    //cstring: configurable information string, sent from Android
  KEY_UPTIME,
    //uint16: current watchface uptime in seconds, sent from Pebble
  KEY_TOTAL_UPTIME,
    //uint16: total watchface uptime in seconds, sent from Pebble
  KEY_PEBBLE_MODEL,
    //uint8: pebble model as an enum WatchInfoModel, sent from Pebble
  KEY_PEBBLE_COLOR,
    //uint8: pebble color as an enum WatchInfoColor, sent from Pebble
  KEY_MODE_12_OR_24,
    //uint8: whether the pebble is set to 12 or 24 hour mode(as 12 or 24), sent from Pebble
  KEY_TEMPERATURE,
    //int8: current temperature, sent from Android
  KEY_WEATHER_COND,
    //uint16: an OpenWeatherAPI condition code, sent by Android
  KEY_MEMORY_USED,
    //uint16: amount of memory used by the watchapp, sent from Pebble
  KEY_MEMORY_FREE,
    //uint16: amount of memory available for watchapp, sent from Pebble
  KEY_UPDATE_FREQS_BEGIN = 30,
    //uint16: First update frequency(seconds), sent from Android
    //This begins a series of keys holding update frequencies for all update types
    //Update types are defined in order in messaging.h
  KEY_COLORS_BEGIN = 50
    //cstring: Holds the first color string, sent by Android
    //This is the first of a sequence of NUM_COLORS color keys
    //NUM_COLORS is defined in display.h
};

//----------APPMESSAGE MESSAGE CODES----------
//Valid message codes for messages sent from Pebble
typedef enum{
  CODE_EVENT_REQUEST,
    //Message requesting updated event info
  CODE_BATTERY_REQUEST,
    //Message requesting updated battery percentage
  CODE_INFOTEXT_REQUEST,
    //Message requesting updated infoText data
  CODE_WEATHER_REQUEST,
    //Message requesting updated weather data
  CODE_PEBBLE_STATS_RESPONSE
    //Message providing requested Pebble information
} PebbleMessageCode;

//Valid message codes for messages received from Android
typedef enum{
  CODE_EVENT_RESPONSE,
    //Message providing updated event info
  CODE_BATTERY_RESPONSE,
    //Message providing updated battery percentage
  CODE_INFOTEXT_RESPONSE,
    //Message providing updated infoText data
  CODE_WEATHER_RESPONSE,
    //Message providing updated weather data
  CODE_COLOR_UPDATE,
    //Message providing updated color data
  CODE_PEBBLE_STATS_REQUEST
    //Message requesting assorted Pebble information
} AndroidMessageCode;

//----------MESSAGE STACK STRUCTURE----------
typedef struct msgStack{
  uint8_t buffer[DICT_SIZE];//Message dictionary buffer
  struct msgStack * next;//Link to the next message
}MessageStack;

//----------LOCAL VARIABLES----------
MessageStack * messageStack = NULL; //Unsent message stack
int init = 0;//Equals 1 iff messaging_init has been run

time_t lastUpdate[NUM_UPDATE_TYPES] = {0};
  //Last data update times
static int updateFreq[NUM_UPDATE_TYPES] = {DEFAULT_UPDATE_FREQ};
//Update frequencies sent from android

//----------STATIC FUNCTION DECLARATIONS----------
static void add_message(uint8_t dictBuf[DICT_SIZE]);
  //adds a new message to the queue
static void send_message();
  //Sends the first message in the queue
static void delete_message();
  //Removes the first message in the queue

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

//----------PUBLIC FUNCTIONS----------
//Initializes AppMessage functionality
void messaging_init(){
  if(!init){
     //Load last update times and update frequencies
    for(int i=0;i< NUM_UPDATE_TYPES; i++){
      if(persist_exists(PERSIST_KEY_LAST_UPDATE_TIMES_BEGIN + i)){
        lastUpdate[i] = (time_t) persist_read_int(PERSIST_KEY_LAST_UPDATE_TIMES_BEGIN + i);
      }      
      if(persist_exists(PERSIST_KEY_UPDATE_FREQS_BEGIN+i)){
        updateFreq[i] = persist_read_int(PERSIST_KEY_UPDATE_FREQS_BEGIN + i);
      }
    }
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
    //delete remaining messages
    while(messageStack != NULL){
      delete_message();
    }
    //save persistent values
    for(int i=0;i< NUM_UPDATE_TYPES; i++){
      persist_write_int(PERSIST_KEY_LAST_UPDATE_TIMES_BEGIN+i,(int)lastUpdate[i]);
      persist_write_int(PERSIST_KEY_UPDATE_FREQS_BEGIN+i,updateFreq[i]);
    }
    app_message_deregister_callbacks();
    init = 0;
  }
}

//Requests updated info from the companion app
void request_update(UpdateType updateType){
  if(!init)messaging_init();
  uint8_t buf[DICT_SIZE];
  DictionaryIterator iter;
  //request all events:
  dict_write_begin(&iter,buf,DICT_SIZE);
  switch(updateType){
    case UPDATE_TYPE_EVENT:
      dict_write_uint8(&iter, KEY_MESSAGE_CODE, CODE_EVENT_REQUEST);
      break;
    case UPDATE_TYPE_BATTERY:
      dict_write_uint8(&iter, KEY_MESSAGE_CODE, CODE_BATTERY_REQUEST);
      break;
    case UPDATE_TYPE_INFOTEXT:
      dict_write_uint8(&iter, KEY_MESSAGE_CODE, CODE_INFOTEXT_REQUEST);
      break;
    case UPDATE_TYPE_WEATHER:
      dict_write_uint8(&iter, KEY_MESSAGE_CODE, CODE_WEATHER_REQUEST);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR,"request_update:invalid request type");
      return;
  }
  dict_write_end(&iter);
  add_message(buf);
  #ifdef DEBUG_MESSAGING
    APP_LOG(APP_LOG_LEVEL_DEBUG,"request_update:Attempting to send request");
  #endif
  send_message();
}

//Gets the update frequency for a given request
int get_update_frequency(UpdateType updateType){
  return updateFreq[updateType];
}

//Gets the last time a given update was received
time_t get_update_time(UpdateType updateType){
  return lastUpdate[updateType];
}

//----------STATIC FUNCTIONS----------
/**
*adds a new message to the queue
*param dictBuf a dictionary buffer containing the new message
*/
static void add_message(uint8_t dictBuf[DICT_SIZE]){
  if(!init)messaging_init();
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
  #ifdef DEBUG_MESSAGING
    APP_LOG(APP_LOG_LEVEL_DEBUG,"add_message:Added message to queue");
  #endif
}

/**
*Sends the first message in the queue
*/
static void send_message(){
  if(!init)messaging_init();
  if(messageStack == NULL)return;
  #ifdef DEBUG_MESSAGING
    APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Retrieved message from queue");
  #endif
  DictionaryIterator read,*send;
  app_message_outbox_begin(&send);
  #ifdef DEBUG_MESSAGING
    APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Copying message data");
  #endif
  Tuple * dictItem = dict_read_begin_from_buffer(&read, messageStack->buffer, DICT_SIZE);
  while(dictItem != NULL){
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
  //Append extra pebble data to dictionary
  dict_write_uint16(send, KEY_UPTIME, getUptime());
  dict_write_uint16(send,KEY_TOTAL_UPTIME,getTotalUptime());
  dict_write_uint8(send,KEY_MODE_12_OR_24,clock_is_24h_style() ? 24 : 12);
  dict_write_uint8(send, KEY_PEBBLE_MODEL, (int) watch_info_get_model());
  dict_write_uint8(send, KEY_PEBBLE_COLOR, (int) watch_info_get_color());
  dict_write_uint16(send, KEY_MEMORY_USED, (int)heap_bytes_used());
  dict_write_uint16(send, KEY_MEMORY_FREE, (int)heap_bytes_free());
  #ifdef DEBUG_MESSAGING
    APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Sending message");
    debugDictionary(&read);
  #endif
  app_message_outbox_send();
}

/**
*Removes the first message in the queue
*/
static void delete_message(){
  if(!init)messaging_init();
  if(messageStack != NULL){
    MessageStack * old = messageStack;
    messageStack = messageStack->next;
    free(old);
    #ifdef DEBUG_MESSAGING
      APP_LOG(APP_LOG_LEVEL_DEBUG,"delete_message:old message deleted");
    #endif
  }
}


/**
*Automatically called whenever a message is recieved
*@param iterator the message data iterator
*@param context is required but unused
*@post message data is read and processed
*/
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO,"inbox_received_callback:Message received");
  Tuple *message_code = dict_find(iterator,KEY_MESSAGE_CODE);
  if(message_code != NULL){
    switch((AndroidMessageCode) message_code->value->uint8){
      case CODE_EVENT_RESPONSE:{
        #ifdef DEBUG_MESSAGING
          APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Recieved CODE_EVENT_RESPONSE");
        #endif
        lastUpdate[UPDATE_TYPE_EVENT] = time(NULL);//set last event update time
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
        break;
      }     
      case CODE_BATTERY_RESPONSE:{
        #ifdef DEBUG_MESSAGING
          APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Recieved CODE_BATTERY_RESPONSE");
        #endif
        lastUpdate[UPDATE_TYPE_BATTERY] = time(NULL);
        Tuple *battery = dict_find(iterator,KEY_BATTERY_UPDATE);
        if(battery != NULL){
          #ifdef DEBUG_MESSAGING
            APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Getting battery info");
          #endif
          update_display_text(battery->value->cstring,TEXT_PHONE_BATTERY);
        }
        break;
      }    
      case CODE_INFOTEXT_RESPONSE:{
        #ifdef DEBUG_MESSAGING
          APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Recieved CODE_INFOTEXT_RESPONSE");
        #endif
        lastUpdate[UPDATE_TYPE_INFOTEXT] = time(NULL);
        Tuple *infoText = dict_find(iterator,KEY_INFOTEXT);
        if(infoText != NULL){
          #ifdef DEBUG_MESSAGING
            APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Getting infoText");
          #endif
          update_display_text(infoText->value->cstring,TEXT_INFOTEXT);
        }
        break;
      }  
      case CODE_WEATHER_RESPONSE:{
        #ifdef DEBUG_MESSAGING
          APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Recieved CODE_WEATHER_RESPONSE");
        #endif
        lastUpdate[UPDATE_TYPE_WEATHER] = time(NULL);//set last weather update time
        Tuple * temp = dict_find(iterator,KEY_TEMPERATURE);
        Tuple * cond = dict_find(iterator,KEY_WEATHER_COND);
  
        if(temp != NULL && cond != NULL){
          #ifdef DEBUG_MESSAGING
            APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Getting weather info");
          #endif
          update_weather(temp->value->int8,cond->value->uint16);
        }
        break;
      }
      case CODE_COLOR_UPDATE:{
        #ifdef DEBUG_MESSAGING
          APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Recieved CODE_COLOR_UPDATE");
        #endif
        int i;
        char newColors [NUM_COLORS][7];
        for(i=0;i<NUM_COLORS;i++){
          Tuple * color = dict_find(iterator,KEY_COLORS_BEGIN+i);
          if(color != NULL){
            strcpy(newColors[i],color->value->cstring);
          }else strcpy (newColors[i],"FF0000");//default to white, this should never happen
        }
        update_colors(newColors);
        break;
      }
      case CODE_PEBBLE_STATS_REQUEST:{
        //Send out a message just carrying extra data
        uint8_t buf[DICT_SIZE];
        DictionaryIterator iter;
        dict_write_begin(&iter,buf,DICT_SIZE);
        dict_write_uint8(&iter, KEY_MESSAGE_CODE, CODE_PEBBLE_STATS_RESPONSE);
        dict_write_end(&iter);
        add_message(buf);
        send_message();
        break;
    }
  }   
  
    //Save new update frequencies, if received
    for(int i=0;i<NUM_UPDATE_TYPES;i++){
      Tuple * newFreq = dict_find(iterator,KEY_UPDATE_FREQS_BEGIN+i);
      if(newFreq != NULL)
        updateFreq[i] = newFreq->value->uint16;
    }
  }
  else APP_LOG(APP_LOG_LEVEL_ERROR, "inbox_dropped_callback:Received message with no message code!");
}

/**
*Automatically called whenever recieving a message fails
*@param reason the failure reason
*@param context is required but unused
*@post new update data is requested
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
    APP_LOG(APP_LOG_LEVEL_ERROR, "outbox_failed_callback:Attempting to re-send message");
  send_message();
}

/**
*Automatically calls whenever sending a message succeeds
*@param iterator data from the sent message
*@param context is required but unused
*@post the sent message is removed from the message queue
*/
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "outbox_sent_callback:Outbox send success!");
  delete_message();//delete successfully sent message
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