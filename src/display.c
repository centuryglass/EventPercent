#include <pebble.h>
#include "display.h"
#include "events.h"
#include "keys.h"

//LOCAL VALUE DEFINITIONS
#define DEBUG 0 //Set to 1 to enable display debug logging
#define MAX_EVENTSTR_LENGTH 128  //Maximum number of characters for a single event

//STATIC VARIABLES
static Window *my_window;//Watchface window
//TextLayers
static TextLayer *timeText;//Time text layer
static TextLayer *dateText;//Date text layer
static TextLayer *eventText[NUM_EVENTS];//Event text layers
static TextLayer *phone_battery;//Phone battery text layer
//Strings
static char eventStr[NUM_EVENTS][MAX_EVENTSTR_LENGTH];//Event text string
static char batteryStr [20];//Phone battery string
//Fonts
static GFont coders_crux_16;
static GFont coders_crux_48;

//PUBLIC FUNCTIONS
/**
*initializes all display functionality
*/
void display_init(){
  //window init
  my_window = window_create();
  Layer *window_layer = window_get_root_layer(my_window);
  layer_set_clips(window_layer,false);
  //font init
  coders_crux_16=fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CODERS_CRUX_16));
  coders_crux_48=fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CODERS_CRUX_48));
  //textLayer init
  int i;
  strcpy(batteryStr,"");
  //event text layers
  for(i=0;i<NUM_EVENTS;i++){
    //Load saved event strings
    if(persist_exists(KEY_EVENTS+i)){
      persist_read_string(KEY_EVENTS+i,eventStr[i],MAX_EVENT_LENGTH);
    }else strcpy(eventStr[i],"---");
    eventText[i] = text_layer_create(GRect(20,80+i*40,130,36)); 
    text_layer_set_font(eventText[i],coders_crux_16);
    text_layer_set_background_color(eventText[i],GColorLightGray);
    text_layer_set_text_alignment(eventText[i],GTextAlignmentLeft);
    text_layer_set_text(eventText[i],eventStr[i]);
    layer_add_child(window_layer,text_layer_get_layer(eventText[i]));
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
}
/**
*shuts down all display functionality
*/
void display_deinit(){
  fonts_unload_custom_font(coders_crux_16);
  fonts_unload_custom_font(coders_crux_48);
  int i;
  for(i=0;i<NUM_EVENTS;i++){
    text_layer_destroy(eventText[i]);
  }
  text_layer_destroy(timeText);
  text_layer_destroy(phone_battery);
  window_destroy(my_window);
  //Save event strings
  for(i=0;i<NUM_EVENTS;i++){
    persist_write_string(KEY_EVENTS+i,eventStr[i]);
  }
}
/**
*Updates display text for events
*/
void update_event_display(){
  int i;
  char buf[160];
  for(i=0;i<NUM_EVENTS;i++){
    strcpy(eventStr[i],"");
    if(get_event_title(i,buf,160)!=NULL){
      strcat(eventStr[i],buf);
      strcpy(buf,"");
    }
    if(get_event_time_string(i, buf, 160)!=NULL){
      strcat(eventStr[i],"\n");
      strcat(eventStr[i],buf);
      strcpy(buf,"");
    }
    text_layer_set_text(eventText[i], eventStr[i]);
  }
}
/**
*Sets the displayed time and date
*@param newTime the time data to use to set the time display
*/
void set_time(time_t newTime){
  struct tm *tick_time = localtime(&newTime);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  static char date_buffer[12];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  strftime(date_buffer, sizeof(date_buffer),"%e %b 20%y", tick_time);
  //Display this time on the TextLayer
  text_layer_set_text(timeText, s_buffer);
  text_layer_set_text(dateText, date_buffer);
}
/**
*Updates phone battery display value
*@param battery a cstring containing battery info
*/
void update_phone_battery(char * battery){
  strcpy(batteryStr,"");
    strcat(batteryStr,battery);
    if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"battery is at %s",batteryStr);
    text_layer_set_text(phone_battery,batteryStr);
}