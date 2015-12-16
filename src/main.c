#include <pebble.h>
#include "events.h"
#include "message.h"



Window *my_window;
TextLayer *text_layer1;
TextLayer *text_layer2;
TextLayer *text_layer3;

static char eventStr[256];
static char batteryStr [20];

enum{
  KEY_REQUEST = 0,
  KEY_BATTERY_UPDATE = 1,
  KEY_EVENT_TITLE = 2,
  KEY_EVENT_START = 3,
  KEY_EVENT_END = 4,
  KEY_EVENT_COLOR = 5,
  KEY_EVENT_NUM =6
};

static void updateEventString(){
  int i;
  char buf[160];
  strcpy(eventStr,"");
  for(i=0;i<NUM_EVENTS;i++){
    if(getEventTitle(i,buf,160)!=NULL){
      //APP_LOG(APP_LOG_LEVEL_DEBUG,"Got event %d",i);
      strcat(eventStr,buf);
      strcpy(buf,"");
      strcat(eventStr,"\n");
    }
    //APP_LOG(APP_LOG_LEVEL_DEBUG,"Getting event time %d",i);
    if(getEventTimeString(i, buf, 160)!=NULL){
      //APP_LOG(APP_LOG_LEVEL_DEBUG,"Got event time %d",i);
      strcat(eventStr,buf);
      strcpy(buf,"");
      strcat(eventStr,"\n");
    }
  }
  //APP_LOG(APP_LOG_LEVEL_DEBUG,"Setting text layer to %s",eventStr);
  text_layer_set_text(text_layer3, eventStr);  
}

static void requestEventUpdates(){
  uint8_t buf[DICT_SIZE];
  int i;
  for(i=0;i<NUM_EVENTS;i++){
    DictionaryIterator iter;
    dict_write_begin(&iter,buf,DICT_SIZE);
    dict_write_cstring(&iter, KEY_REQUEST, "Event request");
    dict_write_int16(&iter, KEY_BATTERY_UPDATE, 1);
    dict_write_int16(&iter,KEY_EVENT_NUM,i);
    dict_write_end(&iter);
    add_message(buf);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Setting request for event %d",i);
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Attempting to send message");
  send_message();
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  
  // Display this time on the TextLayer
  text_layer_set_text(text_layer1, s_buffer);
  //update events periodically
  
  if(s_buffer[4] == '5')requestEventUpdates();
}
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}



static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Message received");
  Tuple *request = dict_find(iterator,KEY_REQUEST);
  Tuple *battery = dict_find(iterator,KEY_BATTERY_UPDATE);

  if(request != NULL){
    if(strcmp(request->value->cstring,"Event Sent")){
      Tuple *title = dict_find(iterator,KEY_EVENT_TITLE);
      Tuple *start = dict_find(iterator,KEY_EVENT_START);
      Tuple *end = dict_find(iterator,KEY_EVENT_END);
      Tuple *color = dict_find(iterator,KEY_EVENT_COLOR);
      Tuple *num = dict_find(iterator,KEY_EVENT_NUM);  
      if((title != NULL)&&(start != NULL)&&(end != NULL)&&
         (color != NULL)&&(num != NULL)){
        setEvent(num->value->int16,title->value->cstring,start->value->int32,end->value->int32,color->value->cstring);   
      }
    }
  }
  updateEventString();
  if(battery != NULL){
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Getting battery info");
    
    strcpy(batteryStr,"Phone Battery:");
    strcat(batteryStr,battery->value->cstring);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"battery is at %s",batteryStr);
    text_layer_set_text(text_layer2,batteryStr);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Battery text layer set");
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Ending inboxRecieved");
}
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
  requestEventUpdates();
}
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
  send_message();
}
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
  delete_message();
}

void handle_init(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Beginning init");
  strcpy(eventStr,"");
  strcpy(batteryStr,"");
  my_window = window_create();
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Creating text layers");
  //text layer/window init
  
  Layer *window_layer = window_get_root_layer(my_window);
  GRect bounds = layer_get_bounds(window_layer);
  int x = bounds.origin.x;
  int y = bounds.origin.y;
  int w = bounds.size.w;
  int h = 20;
  text_layer1 = text_layer_create(GRect(x,y,w,h));
  y+=30;
  text_layer_set_text(text_layer1, "time");
  text_layer2 = text_layer_create(GRect(x,y,w,h));
  y+=30;
  text_layer_set_text(text_layer2, "battery");
  text_layer3 = text_layer_create(GRect(x,y,w,140));
  text_layer_set_text(text_layer3, "events");
  layer_add_child(window_layer,text_layer_get_layer(text_layer1));
  layer_add_child(window_layer,text_layer_get_layer(text_layer2));
  layer_add_child(window_layer,text_layer_get_layer(text_layer3));
  window_stack_push(my_window,true);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"registering callbacks");
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
   // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  // Make sure the time is displayed from the start
  update_time();
  // Open AppMessage
  APP_LOG(APP_LOG_LEVEL_DEBUG,"opening AppMessage");
  app_message_open(INBOX_SIZE,OUTBOX_SIZE);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Sending info request");
  requestEventUpdates();
}

void handle_deinit(void) {
  text_layer_destroy(text_layer1);
  text_layer_destroy(text_layer2);
  text_layer_destroy(text_layer3);
  window_destroy(my_window);
//  delete_all_messages();
}

int main(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Starting app");
  handle_init();
  app_event_loop();
  handle_deinit();
}
