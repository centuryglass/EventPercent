#include <pebble.h>
#include "message_handler.h"
#include "display_handler.h"
#include "display_elements.h"
#include "core_messaging.h"
#include "events.h"
#include "util.h"
#include "storage_keys.h"

//----------LOCAL VALUE DEFINITIONS----------
//#define DEBUG_MESSAGING //Uncomment to enable messaging debug logging
#define DEFAULT_UPDATE_FREQ 300 //default update frequency(seconds)
//----------APPMESSAGE KEY DEFINITIONS----------
enum{
  KEY_MESSAGE_CODE,
    //int32: identifying the purpose of the message, bi-directional
  KEY_BATTERY_UPDATE,
    //cstring: containing the sender's battery life percentage, bi-directional
  KEY_EVENT_TITLE,
    //cstring: an event's title string, sent from Android
  KEY_EVENT_START,
    //int32: event start time, sent from Android
  KEY_EVENT_END,
    //int32: event end time, sent from Android
  KEY_EVENT_COLOR,
    //cstring: event color, sent from Android
  KEY_EVENT_NUM,
    //int32: event index number, sent from Android
  KEY_INFOTEXT,
    //cstring: configurable information string, sent from Android
  KEY_UPTIME,
    //int32: current watchface uptime in seconds, sent from Pebble
  KEY_TOTAL_UPTIME,
    //int32: total watchface uptime in seconds, sent from Pebble
  KEY_PEBBLE_MODEL,
    //int32: pebble model as an enum WatchInfoModel, sent from Pebble
  KEY_PEBBLE_COLOR,
    //int32: pebble color as an enum WatchInfoColor, sent from Pebble
  KEY_MODE_12_OR_24,
    //int32: whether the pebble is set to 12 or 24 hour mode(as 12 or 24), sent from Pebble
  KEY_TEMPERATURE,
    //int32: current temperature, sent from Android
  KEY_WEATHER_COND,
    //int32: an OpenWeatherAPI condition code, sent by Android
  KEY_MEMORY_USED,
    //int32: amount of memory used by the watchapp, sent from Pebble
  KEY_MEMORY_FREE,
    //int32: amount of memory available for watchapp, sent from Pebble
  KEY_DATE_FORMAT,
    //cstring: date format recognizable by strftime
  KEY_FUTURE_EVENT_TIME_FORMAT,
    //int32: index of the enum FutureEventFormat type selected, sent from Android
  KEY_UPDATE_FREQS_BEGIN = 30,
    //int32: First update frequency(seconds), sent from Android
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

//----------LOCAL VARIABLES----------

time_t lastUpdate[NUM_UPDATE_TYPES] = {0};
  //Last data update times
static int updateFreq[NUM_UPDATE_TYPES] = {DEFAULT_UPDATE_FREQ};
  //Update frequencies sent from android

static void process_message(DictionaryIterator *iterator);
static int appContacted = 0;//1 if the companion app has been reached
//----------PUBLIC FUNCTIONS----------
//Initializes AppMessage functionality
void message_handler_init(){
  //see if companion app has been contacted
  if(persist_exists(PERSIST_KEY_COMPANION_APP_CONTACTED)){
    appContacted = persist_read_int(PERSIST_KEY_COMPANION_APP_CONTACTED);
  }
  //Load last update times and update frequencies
  for(int i=0;i< NUM_UPDATE_TYPES; i++){
    if(persist_exists(PERSIST_KEY_LAST_UPDATE_TIMES_BEGIN + i))
      lastUpdate[i] = (time_t) persist_read_int(PERSIST_KEY_LAST_UPDATE_TIMES_BEGIN + i);   
    if(persist_exists(PERSIST_KEY_UPDATE_FREQS_BEGIN+i))
        updateFreq[i] = persist_read_int(PERSIST_KEY_UPDATE_FREQS_BEGIN + i);
  }
  open_messaging();
  register_inbox_handler(process_message);
}

//Shuts down AppMessage functionality
void messaging_deinit(){
  
    //save persistent values
    for(int i=0;i< NUM_UPDATE_TYPES; i++){
      persist_write_int(PERSIST_KEY_LAST_UPDATE_TIMES_BEGIN+i,(int)lastUpdate[i]);
      persist_write_int(PERSIST_KEY_UPDATE_FREQS_BEGIN+i,updateFreq[i]);
    }
    persist_write_int(PERSIST_KEY_COMPANION_APP_CONTACTED,appContacted);
    close_messaging();
}

//Requests updated info from the companion app
void request_update(UpdateType updateType){
  if(appContacted == 0)return;//Don't request updates until connected
  
  uint8_t buf[DICT_SIZE] = {0};//default buffer values to 0 to avoid 
    //junk data overwriting legitimate keys
  DictionaryIterator iter;
  //request all events:
  dict_write_begin(&iter,buf,DICT_SIZE);
  switch(updateType){
    case UPDATE_TYPE_EVENT:
      dict_write_int32(&iter, KEY_MESSAGE_CODE, CODE_EVENT_REQUEST);
      #ifdef DEBUG_MESSAGING
      APP_LOG(APP_LOG_LEVEL_DEBUG,"request_update:Requesting event update");
      #endif
      break;
    case UPDATE_TYPE_BATTERY:
      dict_write_int32(&iter, KEY_MESSAGE_CODE, CODE_BATTERY_REQUEST);
      #ifdef DEBUG_MESSAGING
      APP_LOG(APP_LOG_LEVEL_DEBUG,"request_update:Requesting battery update");
      #endif
      break;
    case UPDATE_TYPE_INFOTEXT:
      dict_write_int32(&iter, KEY_MESSAGE_CODE, CODE_INFOTEXT_REQUEST);
      #ifdef DEBUG_MESSAGING
      APP_LOG(APP_LOG_LEVEL_DEBUG,"request_update:Requesting infoText update");
      #endif
      break;
    case UPDATE_TYPE_WEATHER:
      dict_write_int32(&iter, KEY_MESSAGE_CODE, CODE_WEATHER_REQUEST);
      #ifdef DEBUG_MESSAGING
      APP_LOG(APP_LOG_LEVEL_DEBUG,"request_update:Requesting weather update");
      #endif
      break;
    case UPDATE_TYPE_PEBBLE_STATS:
      dict_write_int32(&iter, KEY_MESSAGE_CODE, CODE_PEBBLE_STATS_RESPONSE);
      #ifdef DEBUG_MESSAGING
      APP_LOG(APP_LOG_LEVEL_DEBUG,"request_update:sending stats update");
      #endif
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR,"request_update:invalid request type");
      return;
  }
  //Append extra pebble data to dictionary
  char batteryBuf [6];
  getPebbleBattery(batteryBuf);
  dict_write_cstring(&iter, KEY_BATTERY_UPDATE, batteryBuf);
  dict_write_int32(&iter, KEY_UPTIME, getUptime());
  dict_write_int32(&iter,KEY_TOTAL_UPTIME,getTotalUptime());
  dict_write_int32(&iter,KEY_MODE_12_OR_24,clock_is_24h_style() ? 24 : 12);
  dict_write_int32(&iter, KEY_PEBBLE_MODEL, (int) watch_info_get_model());
  dict_write_int32(&iter, KEY_PEBBLE_COLOR, (int) watch_info_get_color());
  dict_write_int32(&iter, KEY_MEMORY_USED, (int)heap_bytes_used());
  dict_write_int32(&iter, KEY_MEMORY_FREE, (int)heap_bytes_free());
  dict_write_end(&iter);
  #ifdef DEBUG_MESSAGING
    APP_LOG(APP_LOG_LEVEL_DEBUG,"request_update:Attempting to send request");
  #endif
  add_message(buf);
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

static void process_message(DictionaryIterator *iterator){
  if(appContacted == 0){//First contact, send info and request updates
    appContacted = 1;
    request_update(UPDATE_TYPE_PEBBLE_STATS);
    request_update(UPDATE_TYPE_EVENT);
    request_update(UPDATE_TYPE_BATTERY);
    request_update(UPDATE_TYPE_INFOTEXT);
    request_update(UPDATE_TYPE_WEATHER);
  }
  Tuple *message_code = dict_find(iterator,KEY_MESSAGE_CODE);
  if(message_code != NULL){
    switch((AndroidMessageCode) message_code->value->int32){
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
        add_event(num->value->int32,
                 title->value->cstring,
                 start->value->int32,
                 end->value->int32,
                 color->value->cstring);   
        }
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
          update_text(battery->value->cstring,TEXT_PHONE_BATTERY);
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
          update_text(infoText->value->cstring,TEXT_INFOTEXT);
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
          update_weather(temp->value->int32,cond->value->int32);
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
            strncpy(newColors[i],color->value->cstring,sizeof(newColors[i]));
          }else strncpy (newColors[i],"",sizeof(newColors[i]));//default to empty string if not updating color
        }
        update_colors(newColors);
        break;
      }
      case CODE_PEBBLE_STATS_REQUEST:{
        //Send out a message just carrying extra data
        #ifdef DEBUG_MESSAGING
          APP_LOG(APP_LOG_LEVEL_DEBUG,"inbox_received_callback:Recieved stats request");
        #endif
        request_update(UPDATE_TYPE_PEBBLE_STATS);
        break;
    }
  }   
  
    //Save new update frequencies, if received
    for(int i=0;i<NUM_UPDATE_TYPES;i++){
      Tuple * newFreq = dict_find(iterator,KEY_UPDATE_FREQS_BEGIN+i);
      if(newFreq != NULL)
        updateFreq[i] = newFreq->value->int32;
    }
    //Save date format, if received
    Tuple * dateFormat = dict_find(iterator,KEY_DATE_FORMAT);
    if(dateFormat != NULL)
      set_date_format(dateFormat->value->cstring); 
    Tuple * futureEventTimeFormat = dict_find(iterator,KEY_FUTURE_EVENT_TIME_FORMAT);
    if(futureEventTimeFormat != NULL)
      setFutureEventTimeFormat(futureEventTimeFormat->value->int32);  
  }
  else APP_LOG(APP_LOG_LEVEL_ERROR, "inbox_dropped_callback:Received message with no message code!");
}
