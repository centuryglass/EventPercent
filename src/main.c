#include <pebble.h>
#include "events.h"
#include "message.h"
#include "util.h"



static Window *my_window;
static TextLayer *timeText;
static TextLayer *dateText;
static TextLayer *event_title[NUM_EVENTS];
//static TextLayer *event_time[NUM_EVENTS];
static TextLayer *phone_battery;

static char eventStr[NUM_EVENTS][256];
static char batteryStr [20];

static GFont coders_crux_16;
static GFont coders_crux_48;

static int updateFreq;//update frequency in seconds
static long lastUpdate;
#define DEFAULT_UPDATE_FREQ 3600


//Updates display text for events
static void updateEventStrings(){
  int i;
  char buf[160];
  for(i=0;i<NUM_EVENTS;i++){
    strcpy(eventStr[i],"");
    if(getEventTitle(i,buf,160)!=NULL){
      strcat(eventStr[i],buf);
      strcpy(buf,"");
    }
    if(getEventTimeString(i, buf, 160)!=NULL){
      strcat(eventStr[i],"\n");
      strcat(eventStr[i],buf);
      strcpy(buf,"");
    }
    text_layer_set_text(event_title[i], eventStr[i]);
  }
}
static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  static char date_buffer[12];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  strftime(date_buffer, sizeof(date_buffer),"%e %b 20%y", tick_time);
  //Display this time on the TextLayer
  text_layer_set_text(timeText, s_buffer);
  text_layer_set_text(dateText, date_buffer);
  //update events periodically
  tick
  if(s_buffer[4] == '5')
    requestEventUpdates();
  else 
    updateEventStrings();
}
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}
//AppMessage callback functions
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Message received");
  //debugDictionary(iterator);
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
        setEvent(num->value->int16,
                 title->value->cstring,
                 start->value->int32,
                 end->value->int32,
                 color->value->cstring);   
      }
    }
    updateEventStrings();
  }
  if(battery != NULL){
    //APP_LOG(APP_LOG_LEVEL_DEBUG,"Getting battery info");    
    strcpy(batteryStr,"");
    strcat(batteryStr,battery->value->cstring);
    //APP_LOG(APP_LOG_LEVEL_DEBUG,"battery is at %s",batteryStr);
    text_layer_set_text(phone_battery,batteryStr);
  }
  send_message();
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
  updateFreq = DEFAULT_UPDATE_FREQ;
  lastUpdate = 0;
  //window init
  my_window = window_create();
  Layer *window_layer = window_get_root_layer(my_window);
  layer_set_clips(window_layer,false);
  //font init
  coders_crux_16=fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CODERS_CRUX_16));
  coders_crux_48=fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CODERS_CRUX_48));
  /*graphics init
  prototype = gbitmap_create_with_resource(RESOURCE_ID_PROTOTYPE);
  prototypeLayer = bitmap_layer_create(GRect(0,0,144,168));
  bitmap_layer_set_compositing_mode(prototypeLayer, GCompOpSet);
  bitmap_layer_set_bitmap(prototypeLayer,prototype);
  layer_add_child(window_layer,bitmap_layer_get_layer(prototypeLayer));
  */
  //textLayer init
  int i;
  strcpy(batteryStr,"");
  //event text layers
  for(i=0;i<NUM_EVENTS;i++){
    strcpy(eventStr[i],"Loading...");
    event_title[i] = text_layer_create(GRect(20,80+i*40,130,36)); 
    text_layer_set_font(event_title[i],coders_crux_16);
    text_layer_set_background_color(event_title[i],GColorLightGray);
    text_layer_set_text_alignment(event_title[i],GTextAlignmentLeft);
    text_layer_set_text(event_title[i],eventStr[i]);
    layer_add_child(window_layer,text_layer_get_layer(event_title[i]));
  }
  //clock text layer
  timeText = text_layer_create(GRect(20,0,130,54));
  text_layer_set_background_color(timeText,GColorLightGray);
  text_layer_set_text_alignment(timeText,GTextAlignmentLeft);
  dateText = text_layer_create(GRect(20,50,130,18));
  text_layer_set_background_color(dateText,GColorLightGray);
  text_layer_set_text_alignment(dateText,GTextAlignmentLeft);
  
  text_layer_set_font(timeText,coders_crux_48);
  text_layer_set_font(dateText,coders_crux_16);
  text_layer_set_text(timeText, "time");
  text_layer_set_text(dateText, "date");
  //phone battery text layer
  phone_battery = text_layer_create(GRect(100,0,55,18));
  
  text_layer_set_font(phone_battery,coders_crux_16);
  text_layer_set_background_color(phone_battery,GColorLightGray);
  text_layer_set_text_alignment(phone_battery,GTextAlignmentLeft);
  text_layer_set_text(phone_battery, "?%");
  layer_add_child(window_layer,text_layer_get_layer(timeText));
  layer_add_child(window_layer,text_layer_get_layer(dateText));
  layer_add_child(window_layer,text_layer_get_layer(phone_battery));
  window_stack_push(my_window,true);
  // Register AppMessage callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  // Open AppMessage
  app_message_open(INBOX_SIZE,OUTBOX_SIZE);
  requestEventUpdates();
   // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  // Make sure the time is displayed from the start
  update_time(); 
  APP_LOG(APP_LOG_LEVEL_DEBUG,"INIT SUCCESS");
}

void handle_deinit(void) {
  fonts_unload_custom_font(coders_crux_16);
  fonts_unload_custom_font(coders_crux_48);
  int i;
  for(i=0;i<NUM_EVENTS;i++){
    text_layer_destroy(event_title[i]);
  }
  text_layer_destroy(timeText);
  text_layer_destroy(phone_battery);
  window_destroy(my_window);
  //delete_all_messages();
  delete_all_messages();
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
