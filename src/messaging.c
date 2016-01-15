#include <pebble.h>
#include "messaging.h"
#include "display.h"
#include "events.h"
#include "util.h"
#include "keys.h"

//----------LOCAL VALUE DEFINITIONS----------
#define DEBUG_MESSAGING 1 //Uncomment to 1 to enable messaging debug logging
#define DEFAULT_EVENT_UPDATE_FREQ 3600 //default event update frequency(seconds)
#define DEFAULT_BATTERY_UPDATE_FREQ 300 //default battery update frequency(seconds)
#define DEFAULT_INFOTEXT_UPDATE_FREQ 1200 //default info text update frequency(seconds)

//----------MESSAGE STACK STRUCTURE----------
struct msgStack{
  uint8_t buffer[DICT_SIZE];//Message dictionary buffer
  struct msgStack * next;//Link to the next message
};

//----------STATIC VARIABLES----------
static struct msgStack * messageStack = NULL; //Unsent message stack
static int init = 0;//Equals 1 iff messaging_init has been run
static time_t lastEventRequest,lastBatteryRequest,lastInfoRequest,lastWeatherRequest;
  //Last data update times
static int eventUpdateFreq,batteryUpdateFreq,infoUpdateFreq,weatherUpdateFreq;
  //Update frequencies sent from android

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
     //Load last update request times
    if(persist_exists(PERSIST_KEY_LAST_EVENT_REQUEST))
      lastEventRequest = (time_t) persist_read_int(PERSIST_KEY_LAST_EVENT_REQUEST);
    else lastEventRequest = (time_t) 0;
    if(persist_exists(PERSIST_KEY_LAST_BATTERY_REQUEST))
      lastBatteryRequest = (time_t) persist_read_int(PERSIST_KEY_LAST_BATTERY_REQUEST);
    else lastBatteryRequest = (time_t) 0;
    if(persist_exists(PERSIST_KEY_LAST_INFOTEXT_REQUEST))
      lastInfoRequest = (time_t) persist_read_int(PERSIST_KEY_LAST_INFOTEXT_REQUEST);
    else lastInfoRequest = (time_t) 0;
    
    //load update frequencies
    if(persist_exists(PERSIST_KEY_EVENT_UPDATE_FREQ))
      eventUpdateFreq = persist_read_int(PERSIST_KEY_EVENT_UPDATE_FREQ);
    else eventUpdateFreq = DEFAULT_EVENT_UPDATE_FREQ;
    if(persist_exists(PERSIST_KEY_BATTERY_UPDATE_FREQ))
      batteryUpdateFreq = persist_read_int(PERSIST_KEY_BATTERY_UPDATE_FREQ);
    else batteryUpdateFreq = DEFAULT_BATTERY_UPDATE_FREQ;
    if(persist_exists(PERSIST_KEY_INFOTEXT_UPDATE_FREQ))
      infoUpdateFreq = persist_read_int(PERSIST_KEY_INFOTEXT_UPDATE_FREQ);
    else infoUpdateFreq = DEFAULT_INFOTEXT_UPDATE_FREQ;
    if(persist_exists(PERSIST_KEY_WEATHER_UPDATE_FREQ))
      weatherUpdateFreq = persist_read_int(PERSIST_KEY_INFOTEXT_UPDATE_FREQ);
    else weatherUpdateFreq = DEFAULT_INFOTEXT_UPDATE_FREQ;
    
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
    persist_write_int(PERSIST_KEY_LAST_EVENT_REQUEST,(int)lastEventRequest);
    persist_write_int(PERSIST_KEY_LAST_BATTERY_REQUEST,(int)lastBatteryRequest);
    persist_write_int(PERSIST_KEY_LAST_INFOTEXT_REQUEST,(int)lastInfoRequest);
    persist_write_int(PERSIST_KEY_LAST_WEATHER_REQUEST,(int)lastWeatherRequest);
    
    persist_write_int(PERSIST_KEY_EVENT_UPDATE_FREQ,eventUpdateFreq);
    persist_write_int(PERSIST_KEY_BATTERY_UPDATE_FREQ,batteryUpdateFreq);
    persist_write_int(PERSIST_KEY_INFOTEXT_UPDATE_FREQ,infoUpdateFreq);
    persist_write_int(PERSIST_KEY_WEATHER_UPDATE_FREQ,weatherUpdateFreq);
    init = 0;
  }
}

//Requests updated info from the companion app
void request_update(enum REQUEST requestType){
  if(!init)messaging_init();
  uint8_t buf[DICT_SIZE];
  DictionaryIterator iter;
  //request all events:
  dict_write_begin(&iter,buf,DICT_SIZE);
  switch(requestType){
    case event_request:
      lastEventRequest = time(NULL);
      dict_write_cstring(&iter, KEY_REQUEST, "Event Request");
      dict_write_int32(&iter,KEY_EVENT_NUM,NUM_EVENTS);
      break;
    case battery_request:
      lastBatteryRequest = time(NULL);
      dict_write_cstring(&iter, KEY_REQUEST, "Battery Request");
      break;
    case infotext_request:
      lastInfoRequest = time(NULL);
      dict_write_cstring(&iter, KEY_REQUEST, "InfoText Request");
      break;
    case weather_request:
      lastWeatherRequest = time(NULL);
      dict_write_cstring(&iter, KEY_REQUEST, "Weather Request");
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
int get_update_frequency(enum REQUEST requestType){
  switch(requestType){
    case event_request:
        return eventUpdateFreq;
    case battery_request:
        return batteryUpdateFreq;
    case infotext_request:
      return infoUpdateFreq;
    case weather_request:
      return weatherUpdateFreq;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR,"get_update_frequency:Invalid request type! returning 999");
      return 999;   
  }
}

//Gets the last time a given request type was made
time_t get_request_time(enum REQUEST requestType){
  switch(requestType){
    case event_request:
        return lastEventRequest;
    case battery_request:
        return lastBatteryRequest;
    case infotext_request:
      return lastInfoRequest;
    case weather_request:
      return lastWeatherRequest;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR,"get_update_time:Invalid request type! returning current time");
      return time(NULL);
  }
}

//----------STATIC FUNCTIONS----------
/**
*adds a new message to the queue
*/
static void add_message(uint8_t dictBuf[DICT_SIZE]){
  if(!init)messaging_init();
  #ifdef DEBUG_MESSAGING
    APP_LOG(APP_LOG_LEVEL_DEBUG,"add_message:New message content:");
    DictionaryIterator read;
    dict_read_begin_from_buffer(&read, dictBuf, DICT_SIZE);
    debugDictionary(&read);
  #endif
  
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
    if(dictItem->type == TUPLE_BYTE_ARRAY){
      #ifdef DEBUG_MESSAGING
        APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Writing byte array for key %d",(int)key);
      #endif
      dict_write_data(send,key,dictItem->value->data,size);
    }
    else if(dictItem->type == TUPLE_CSTRING){
      #ifdef DEBUG_MESSAGING
        APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Writing string %s for key %d"
                       ,dictItem->value->cstring,(int)key);
      #endif
      dict_write_cstring(send, key, dictItem->value->cstring);
    }
    if(dictItem->type == TUPLE_UINT){
      uint32_t  val = dictItem->value->uint32;
      #ifdef DEBUG_MESSAGING
        APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Writing int for key %d",(int)key);
      #endif
      dict_write_int(send,key,(int *)&val,size,false);
    }
    if(dictItem->type == TUPLE_INT){
      #ifdef DEBUG_MESSAGING
        APP_LOG(APP_LOG_LEVEL_DEBUG,"send_message:Writing int for key %d",(int)key);
      #endif
      int32_t val = dictItem->value->int32;
      dict_write_int(send,key,(int *)&val,size,true);
    }  
    dictItem = dict_read_next(&read);
  }
  //Append extra pebble data to dictionary
  dict_write_int32(send, KEY_UPTIME, getUptime());
  dict_write_int32(send,KEY_TOTAL_UPTIME,getTotalUptime());
  dict_write_int32(send,KEY_12_OR_24_MODE,clock_is_24h_style() ? 24 : 12);
  dict_write_int32(send, KEY_PEBBLE_MODEL, (int) watch_info_get_model());
  dict_write_int32(send, KEY_PEBBLE_COLOR, (int) watch_info_get_color());
  dict_write_int32(send, KEY_MEMORY_USED, (int)heap_bytes_used());
  dict_write_int32(send, KEY_MEMORY_FREE, (int)heap_bytes_free());
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
    struct msgStack * old = messageStack;
    messageStack = messageStack->next;
    free(old);
    #ifdef DEBUG_MESSAGING
      APP_LOG(APP_LOG_LEVEL_DEBUG,"delete_message:old message deleted");
    #endif
  }
}

//Automatically called whenever a message is recieved
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO,"inbox_received_callback:Message received");
  Tuple *request = dict_find(iterator,KEY_REQUEST);

  if(request != NULL){
    if(strcmp(request->value->cstring,"Event sent") == 0){
      #ifdef DEBUG_MESSAGING
        APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Recieved event data");
      #endif
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
    else if(strcmp(request->value->cstring,"Color update") == 0){
      #ifdef DEBUG_MESSAGING
        APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Recieved color data");
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
    }
    else if(strcmp(request->value->cstring,"Weather sent") == 0){
      #ifdef DEBUG_MESSAGING
        APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Recieved weather data");
      #endif
      Tuple * temp = dict_find(iterator,KEY_TEMP);
      Tuple * cond = dict_find(iterator,KEY_WEATHER_COND);
      if(temp != NULL && cond != NULL){
        update_weather(temp->value->int32,cond->value->int32);
      }
    }
    else if(strcmp(request->value->cstring,"Watch info request") == 0){
      //Send out a message carrying extra data
        uint8_t buf[DICT_SIZE];
        DictionaryIterator iter;
        dict_write_begin(&iter,buf,DICT_SIZE);
        dict_write_cstring(&iter, KEY_REQUEST, "Watch info request");
        dict_write_end(&iter);
        add_message(buf);
        send_message();
    }
  }
  //Update other misc data, if received
  Tuple *battery = dict_find(iterator,KEY_BATTERY_UPDATE);
  Tuple *infoText = dict_find(iterator,KEY_INFOTEXT);
  Tuple *eventFreq = dict_find(iterator,KEY_EVENT_UPDATE_FREQ);
  Tuple *batteryFreq = dict_find(iterator,KEY_EVENT_UPDATE_FREQ);
  Tuple *infoFreq = dict_find(iterator,KEY_INFOTEXT_UPDATE_FREQ);
  if(battery != NULL){
    #ifdef DEBUG_MESSAGING
      APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Getting battery info");
    #endif
    update_phone_battery(battery->value->cstring);
  }
  if(infoText != NULL){
    update_infoText(infoText->value->cstring);
  }
  if(eventFreq != NULL){
     eventUpdateFreq = eventFreq->value->int32;
  }
  if(batteryFreq != NULL){
    batteryUpdateFreq = batteryFreq->value->int32;
  }
  if(infoFreq != NULL){
    infoUpdateFreq = infoFreq->value->int32;
  }
}

//Automatically called whenever recieving a message fails
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "inbox_dropped_callback:Message dropped");
    log_result_info(reason);
}

//Automatically calls whenever sending a message fails
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "outbox_failed_callback:Outbox send failed");
    log_result_info(reason);
    APP_LOG(APP_LOG_LEVEL_ERROR, "outbox_failed_callback:Attempting to re-send message");
  send_message();
}

//Automatically calls whenever sending a message succeeds
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "outbox_sent_callback:Outbox send success!");
  delete_message();//delete successfully sent message
  send_message();//send the next message in the queue, if there is one
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