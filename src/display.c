#include <pebble.h>
#include "display.h"
#include "events.h"
#include "keys.h"
#include "util.h"

//-----LOCAL VALUE DEFINITIONS-----
#define DEBUG_DISPLAY //uncomment to enable display debug logging
#define MAX_EVENTSTR_LENGTH 70  //Maximum number of characters for a single event

//Color definitions
#define BACKGROUND_COLOR 0
#define FOREGROUND_COLOR 1
#define LINE_COLOR 2
#define TEXT_COLOR 3

#define BACKGROUND_COLOR_DEF PBL_IF_COLOR_ELSE(0x000000,0xFFFFFF)
#define FOREGROUND_COLOR_DEF PBL_IF_COLOR_ELSE(0x000055,0x555555)
#define LINE_COLOR_DEF PBL_IF_COLOR_ELSE(0x005555,0x000000)
#define TEXT_COLOR_DEF PBL_IF_COLOR_ELSE(0x55FFFF,0x000000)

//Text layer size and positioning
//Time text layer
#define TIME_X PBL_IF_ROUND_ELSE(17,6)
#define TIME_Y PBL_IF_ROUND_ELSE(19,0)
#define TIME_W 130
#define TIME_H 54
  
//Date text layer
#define DATE_X PBL_IF_ROUND_ELSE(18,5)
#define DATE_Y PBL_IF_ROUND_ELSE(65,49)
#define DATE_W 70
#define DATE_H 16
  
//Phone battery text layer
#define PH_BAT_X PBL_IF_ROUND_ELSE(111,100)
#define PH_BAT_Y PBL_IF_ROUND_ELSE(11,-5)
#define PH_BAT_W 35
#define PH_BAT_H 18
  
//Pebble battery text layer
#define PEB_BAT_X PBL_IF_ROUND_ELSE(111,100)
#define PEB_BAT_Y PBL_IF_ROUND_ELSE(27,10)
#define PEB_BAT_W 35
#define PEB_BAT_H 18

//Configurable info text layer
#define INFO_X PBL_IF_ROUND_ELSE(27,17)
#define INFO_Y PBL_IF_ROUND_ELSE(17,1)
#define INFO_W 70
#define INFO_H 18

//Weather text layer
#define WEATHER_X PBL_IF_ROUND_ELSE(119,108)
#define WEATHER_Y PBL_IF_ROUND_ELSE(48,33)
#define WEATHER_W 35
#define WEATHER_H 18

//Weather icon layer
#define WEATHER_ICON_X PBL_IF_ROUND_ELSE(138,127)
#define WEATHER_ICON_Y PBL_IF_ROUND_ELSE(64,49)
#define WEATHER_ICON_W 12
#define WEATHER_ICON_H 12
  
//Event text layers
#define EVENT_X PBL_IF_ROUND_ELSE(23,5)
#define EVENT_Y PBL_IF_ROUND_ELSE(81,80)
#define EVENT_DY PBL_IF_ROUND_ELSE(33,40)
#define EVENT_W 200
#define EVENT_H 36
  
//Progress bar positioning
#define PROGRESS_X PBL_IF_ROUND_ELSE(20,2)
#define PROGRESS_Y PBL_IF_ROUND_ELSE(101,100)
#define PROGRESS_DY PBL_IF_ROUND_ELSE(33,40)
#define PROGRESS_W 141
#define PROGRESS_H 19
  
//Day progress bar positioning
#define DAYPROG_X PBL_IF_ROUND_ELSE(16,3)
#define DAYPROG_Y PBL_IF_ROUND_ELSE(83,68)
#define DAYPROG_W 132
#define DAYPROG_H 5



//----------STATIC VARIABLES----------
static Window *my_window = NULL;//Watchface window
static Layer *window_layer = NULL;//the Layer holding my_window

//TextLayers
static TextLayer *timeText = NULL;//Time text layer
static TextLayer *dateText = NULL;//Date text layer
static TextLayer *eventText[NUM_EVENTS];//Event text layers
static TextLayer *phone_battery = NULL;//Phone battery text layer
static TextLayer *watch_battery = NULL;//Watch battery text layer
static TextLayer *infoText = NULL;//Configurable info text layer
static TextLayer *weatherText = NULL;//Weather text layer


//Strings
static char eventStr[NUM_EVENTS][MAX_EVENTSTR_LENGTH];//Event text string
static char phoneBatteryStr [16];//Phone battery string
static char watchBatteryStr [16];//Watch battery string
static char infoStr [16];//Configurable info string
static char weatherStr [16];//Weather string


//Fonts
static GFont coders_crux_16;//small size
static GFont coders_crux_48;//large size (for time)
//Colors
static GColor colors[NUM_COLORS]; //all colors used
static int colors_initialized = 0; //indicates whether initialize_colors() has been run
//bitmaps
static BitmapLayer * BGLayers[2] = {NULL,NULL}; //holds the two background bitmaps
static BitmapLayer * dayProg = NULL; //progress bar showing how much of the day is complete
static BitmapLayer * eventProg[NUM_EVENTS]; //event completion progress bar
static BitmapLayer * weatherIcon; //weather condition icon

static GBitmap * BGbitmaps[2] = {NULL,NULL}; // background images
static GBitmap * dayProgressBitmap = NULL; //day progress bar image
static GBitmap * progressBitmap[NUM_EVENTS]; //event progress bar image
static GBitmap * weatherIcons; //all weather icons

static int weatherCondition;//weather condition code

//----------STATIC FUNCTION DEFINITIONS----------
/**
*creates all BitmapLayers, loads and sets their respective GBitmaps,
*and adds them to the main window
*/
static void create_bitmapLayers();
/**
*Unloads all BitmapLayers, along with their respective GBitmaps
*@post all GBitmaps and BitmapLayers are unloaded and set to null
*/
static void destroy_bitmapLayers();
/**
*Creates a text layer with the given parameters, and adds it to the main window
*@param bounds layer bounding box
*@param text layer initial text
*@param font layer font
*@param align text alignment
*@return the initialized text layer
*/
static TextLayer * init_text_layer(GRect bounds,char * text,GFont font,GTextAlignment align);
/**
*Creates a bitmap layer with the given parameters, and adds it to the main window
*@param bitmap the GBitmap to initialize
*@param resourceID the bitmap resource to load
*@param bounds layer bounding box
*@return the initialized bitmap lYer
*/
static BitmapLayer * init_bitmap_layer(GBitmap ** bitmap,int resourceID,GRect bounds);
/**
*Re-sizes a progress bar
*@param bar the progress bar
*@param percent new bar percentage
*@param width the width of the progress bar when progress=100%
*@post the progress bar is re-sized
*/
static void update_progress(Layer * bar,int percent,int width);
/**
*Updates weather condition display
*/
static void update_weather_condition();
/**
*Fills the background with the background color
*Called automatically whenever the layer is marked as dirty
*@param layer the main window layer
*@param ctx the graphics context
*/
static void background_update_callback(Layer *layer, GContext *ctx);

/**
*Given a GColor, copies its hex value into a buffer string
*@param outstring a buffer of at least size 7
*@param color the GColor to convert to a string
*/
static void gcolor_to_hex_string(char outstring[7], GColor color);
/**
*Given a color hex string, returns a corresponding GColor
*@param string a cstring set to a valid six digit hex color value
*@return the correct GColor
*/
static GColor hex_string_to_gcolor(char * string);
  
/**
*Makes sure all initialized layers are set to the correct colors
*@post each layer has its colors set to a value stored in (color)
*note: this excludes event progress bars, which are set at event color
*/
static void apply_colors();
/**
*Loads colors from persistant storage, or from default definitions
*if no stored colors are found
*@post all colors are initialized and applied to layers
*/
static void initialize_colors();
/**
*save color values to persistant storage
*@post all NUM_COLORS colors are saved
*/
static void save_colors();
/**
*applies a color to a single bitmap layer
*@param color the new color to set
*@param layer an instantiated BitmapLayer set to a 2-bit palletized Gbitmap
*@post all colors in layer besides GColor are set to (color)
*/
static void apply_bitmap_color(GColor color,BitmapLayer *layer);
/**
*applies the correct colors to a single text layer
*@param layer an instantiated TextLayer
*/
static void apply_text_color(TextLayer *layer);


//----------PUBLIC FUNCTIONS----------
//initializes all display functionality
void display_init(){
  int i;
  //Window init
  my_window = window_create();
  window_layer = window_get_root_layer(my_window);
  layer_set_update_proc(window_layer, background_update_callback);
  
  //Font init
  coders_crux_16=fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CODERS_CRUX_16));
  coders_crux_48=fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CODERS_CRUX_48));
  
  //TextLayer init:
  //Load saved text
  if(persist_exists(PERSIST_KEY_PHONE_BATTERY)){
    persist_read_string(PERSIST_KEY_PHONE_BATTERY, phoneBatteryStr, 16);
  }else strcpy(phoneBatteryStr,"?%");
  if(persist_exists(PERSIST_KEY_INFOTEXT)){
    persist_read_string(PERSIST_KEY_INFOTEXT,infoStr,16);
  }else  strcpy(infoStr,"loading");
  if(persist_exists(PERSIST_KEY_WEATHER)){
    persist_read_string(PERSIST_KEY_WEATHER,weatherStr,16);
  }else strcpy(weatherStr,"?°");
  strcpy(watchBatteryStr,"?%");
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"display_init: initializing text layers");
  #endif
  
  //Event text layers
  for(i=0;i<NUM_EVENTS;i++){
    strcpy(eventStr[i],"-----\n-----");//just copy this in as a default 
    eventText[i] = init_text_layer(GRect(EVENT_X,EVENT_Y+i*EVENT_DY,EVENT_W,EVENT_H),eventStr[i],coders_crux_16,GTextAlignmentLeft); 
  }
  //Clock, date, battery, info, & weather layers:
  dateText = init_text_layer(GRect(DATE_X,DATE_Y,DATE_W,DATE_H),"loading date",coders_crux_16,GTextAlignmentLeft); 
  timeText = init_text_layer(GRect(TIME_X,TIME_Y,TIME_W,TIME_H),"00:00",coders_crux_48,GTextAlignmentLeft); 
  watch_battery = init_text_layer(GRect(PEB_BAT_X,PEB_BAT_Y,PEB_BAT_W,PEB_BAT_H),watchBatteryStr,coders_crux_16,GTextAlignmentRight);
  phone_battery = init_text_layer(GRect(PH_BAT_X,PH_BAT_Y,PH_BAT_W,PH_BAT_H),phoneBatteryStr,coders_crux_16,GTextAlignmentRight);
  infoText = init_text_layer(GRect(INFO_X,INFO_Y,INFO_W,INFO_H),infoStr,coders_crux_16,GTextAlignmentLeft);
  weatherText = init_text_layer(GRect(WEATHER_X,WEATHER_Y,WEATHER_W,WEATHER_H),weatherStr,coders_crux_16,GTextAlignmentLeft);
  
  //BitmapLayer and GBitmap init
  create_bitmapLayers();
  window_stack_push(my_window,true);//display the main window
}

//shuts down all display functionality
void display_deinit(){
  //save display information
  persist_write_string(PERSIST_KEY_PHONE_BATTERY, phoneBatteryStr);
  persist_write_string(PERSIST_KEY_INFOTEXT, infoStr);
  persist_write_string(PERSIST_KEY_WEATHER, weatherStr);
  persist_write_int(PERSIST_KEY_WEATHER_COND, weatherCondition);
  //Unload text layers
  int i;
  for(i=0;i<NUM_EVENTS;i++){
    text_layer_destroy(eventText[i]);
  }
  text_layer_destroy(timeText);
  text_layer_destroy(phone_battery);
  text_layer_destroy(watch_battery);
  text_layer_destroy(infoText);
  text_layer_destroy(weatherText);
  //Unload fonts
  fonts_unload_custom_font(coders_crux_16);
  fonts_unload_custom_font(coders_crux_48);
  //Unload BitmapLayers and GBitmaps
  destroy_bitmapLayers();
  //Unload window
  window_destroy(my_window);
}

//Updates display data for events
void update_event_display(){
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_event_display: starting update");
  #endif
  int i;
  char buf[160];
  for(i=0;i<NUM_EVENTS;i++){//for each event,
    if(get_event_title(i,buf,160)!=NULL){//if the event exists set title
      strcpy(eventStr[i],buf);
      strcpy(buf,"");
      if(get_event_time_string(i, buf, 160)!=NULL){//check for and set event time string
        strcat(eventStr[i],"\n");
        strcat(eventStr[i],buf);
        #ifdef DEBUG_DISPLAY
          APP_LOG(APP_LOG_LEVEL_DEBUG,"update_event_display:Setting event text to %s",eventStr[i]);
        #endif
      }
      #ifdef DEBUG_DISPLAY
        else APP_LOG(APP_LOG_LEVEL_ERROR,"update_event_display:Could not get time info for event %d",i);
      #endif
      text_layer_set_text(eventText[i], eventStr[i]); //Set event text if at least there is a valid title
    }
    #ifdef DEBUG_DISPLAY
      else APP_LOG(APP_LOG_LEVEL_ERROR,"update_event_display:Could not get title for event %d",i);
    #endif
    //finally, update event progress bar,
    update_progress(bitmap_layer_get_layer(eventProg[i]),get_percent_complete(i),PROGRESS_W);
    #ifdef PBL_COLOR
      gcolor_to_hex_string(buf,colors[TEXT_COLOR]);//set default in case get_event_color fails
      get_event_color(i, buf);
      #ifdef DEBUG_DISPLAY
        APP_LOG(APP_LOG_LEVEL_DEBUG,"update_event_display:Setting progress bar color to %s",buf);
      #endif
      apply_bitmap_color(hex_string_to_gcolor(buf), eventProg[i]);
    #endif
  }
}

//Updates phone battery display value
void update_phone_battery(char * battery){
  strcpy(phoneBatteryStr,battery);
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_phone_battery:phone battery is at %s",phoneBatteryStr);
  #endif
  text_layer_set_text(phone_battery,phoneBatteryStr);
}

//Updates pebble battery display value
void update_watch_battery(char * battery){
  strcpy(watchBatteryStr,battery);
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_watch_battery:watch battery is at %s",watchBatteryStr);
  #endif
  text_layer_set_text(watch_battery,watchBatteryStr); 
}

//Sets the displayed time and date
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
  //Update day progress bar
  int minutes_passed_today = tick_time->tm_hour*60 + tick_time->tm_min;
  int dayPercent = minutes_passed_today*100/(60*24);
  update_progress(bitmap_layer_get_layer(dayProg),dayPercent,DAYPROG_W);
}

/**
*Updates configurable info text
*@param newText a valid cstring with new display text
*/
void update_infoText(char * newText){
  strcpy(infoStr,newText);
  text_layer_set_text(infoText,infoStr);
}

/**
*Updates weather display
*@param degrees the temperature
*@param condition an OpenWeatherAPI condition code
*/
void update_weather(int degrees, int condition){
  //set degrees
  snprintf(weatherStr,16,"%d°",degrees);
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_weather:final weather string is %s",weatherStr);
  #endif
  text_layer_set_text(weatherText,weatherStr);
  weatherCondition = condition;
  update_weather_condition();
}


//change color values to the ones stored in a color array
void update_colors(char colorArray[NUM_COLORS][7]){
  int i;
  for(i=0;i<NUM_COLORS;i++){
    colors[i] = hex_string_to_gcolor(colorArray[i]);
  }
  apply_colors();
  save_colors();
}


//----------STATIC FUNCTIONS----------
//Creates all BitmapLayers, loads and sets their respective GBitmaps
static void create_bitmapLayers(){
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"create_bitmapLayers:Creating bitmap layers");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "create_bitmapLayers: memory usage is %d",(int)heap_bytes_used());
  #endif
  //Create weather icon BitmapLayer
  weatherIcon = init_bitmap_layer(&weatherIcons, RESOURCE_ID_WEATHER_ICONS,
                                 GRect(WEATHER_ICON_X, WEATHER_ICON_Y,WEATHER_ICON_W,WEATHER_ICON_H));
  //Create background BitmapLayers
  BGLayers[0] = init_bitmap_layer(&BGbitmaps[0], RESOURCE_ID_BG1,layer_get_bounds(window_layer));
  BGLayers[1] = init_bitmap_layer(&BGbitmaps[1], RESOURCE_ID_BG2,layer_get_bounds(window_layer));
  
  //Create event progress bars
  int i;
  for(i=0;i<NUM_EVENTS;i++) eventProg[i] = init_bitmap_layer(&progressBitmap[i], RESOURCE_ID_PROGRESS,
                        GRect(PROGRESS_X, PROGRESS_Y + PROGRESS_DY*i,PROGRESS_W,PROGRESS_H));

  //Create day progress bar BitmapLayer
  dayProg =  init_bitmap_layer(&dayProgressBitmap, RESOURCE_ID_DAY_PROGRESS,
                        GRect(DAYPROG_X, DAYPROG_Y,DAYPROG_W,DAYPROG_H));
  
  initialize_colors();//Load and set colors
  
  if(persist_exists(PERSIST_KEY_WEATHER_COND)){//load weather condition
    weatherCondition = persist_read_int(PERSIST_KEY_WEATHER_COND);
  }else weatherCondition = 800;//default to clear
  update_weather_condition();
}

//Unloads all BitmapLayers, along with their respective GBitmaps
static void destroy_bitmapLayers(){
  int i;
  //Unload background bitmaps and layers
  for(i=0;i<2;i++){
    bitmap_layer_destroy(BGLayers[i]);
    gbitmap_destroy(BGbitmaps[i]);
    BGLayers[i]=NULL;
    BGbitmaps[i]=NULL;
  }
  //Unload event progress bar bitmaps and layers
  for(i=0;i<NUM_EVENTS;i++){
    bitmap_layer_destroy(eventProg[i]);
    eventProg[i] = NULL;
    gbitmap_destroy(progressBitmap[i]);
    progressBitmap[i] = NULL;
  }
  //Unload day progress bar bitmaps and layer
  bitmap_layer_destroy(dayProg);
  dayProg = NULL;
  gbitmap_destroy(dayProgressBitmap);
  dayProgressBitmap = NULL;
  //Unload weather icon bitmap and layer
  bitmap_layer_destroy(weatherIcon);
  weatherIcon = NULL;
  gbitmap_destroy(weatherIcons);
  weatherIcons = NULL;
}

//Creates a text layer with the given parameters, and adds it to the main window
static TextLayer * init_text_layer(GRect bounds,char * text,GFont font,GTextAlignment align){
  TextLayer * textLayer = text_layer_create(bounds);
  text_layer_set_font(textLayer,font);
  text_layer_set_text_alignment(textLayer,align);
  text_layer_set_text(textLayer, text);
  layer_add_child(window_layer,text_layer_get_layer(textLayer));
  return textLayer;
}

//Creates a bitmap layer with the given parameters, and adds it to the main window
static BitmapLayer * init_bitmap_layer(GBitmap ** bitmap,int resourceID,GRect bounds){
  BitmapLayer * bitmapLayer = bitmap_layer_create(bounds);
  *bitmap = gbitmap_create_with_resource(resourceID);
  bitmap_layer_set_bitmap(bitmapLayer,*bitmap);
  bitmap_layer_set_compositing_mode(bitmapLayer, GCompOpSet);
  layer_add_child(window_layer,bitmap_layer_get_layer(bitmapLayer));
  return bitmapLayer;
}

//Re-sizes a progress bar
static void update_progress(Layer * bar,int percent,int width){
  GRect bounds = layer_get_bounds(bar);
  if(percent < 3) percent = 3; //always show a bit of the event color
  int newW = width * percent /100;
  //If the event is x% complete, set progress bar width to x% of the default
  layer_set_bounds(bar, GRect(0,0, newW, bounds.size.h));
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_progress:Changing width from %d to %d",width,newW);
  #endif
}

//Updates weather condition display
static void update_weather_condition(){
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_weather_condition:Setting weather condition icon for id %d",weatherCondition);
  #endif
  //set condition icon
  if(weatherCondition < 300){//group 2xx: thunderstorm
    gbitmap_set_bounds(weatherIcons, GRect(0,0,12,12));
    layer_mark_dirty(bitmap_layer_get_layer(weatherIcon));
    #ifdef DEBUG_DISPLAY
      APP_LOG(APP_LOG_LEVEL_DEBUG,"update_weather_condition:thunderstorm");
    #endif
  }
  else if(weatherCondition < 400){//group 3xx: drizzle
    gbitmap_set_bounds(weatherIcons, GRect(12,0,12,12));
    layer_mark_dirty(bitmap_layer_get_layer(weatherIcon));
    #ifdef DEBUG_DISPLAY
      APP_LOG(APP_LOG_LEVEL_DEBUG,"update_weather_condition:drizzle");
    #endif
  }
  else if(weatherCondition < 600){//group 5xx: rain
    gbitmap_set_bounds(weatherIcons, GRect(24,0,12,12));
    layer_mark_dirty(bitmap_layer_get_layer(weatherIcon));
    #ifdef DEBUG_DISPLAY
      APP_LOG(APP_LOG_LEVEL_DEBUG,"update_weather_condition:rain");
    #endif
  }
  else if(weatherCondition < 700){//group 6xx: snow
    gbitmap_set_bounds(weatherIcons, GRect(0,12,12,12));
    layer_mark_dirty(bitmap_layer_get_layer(weatherIcon));
    #ifdef DEBUG_DISPLAY
      APP_LOG(APP_LOG_LEVEL_DEBUG,"update_weather_condition:snow");
    #endif
  }
  else if(weatherCondition < 800){//group 7xx: atmosphere
    gbitmap_set_bounds(weatherIcons, GRect(12,12,12,12));
    layer_mark_dirty(bitmap_layer_get_layer(weatherIcon));
    #ifdef DEBUG_DISPLAY
      APP_LOG(APP_LOG_LEVEL_DEBUG,"update_weather_condition:atmosphere");
    #endif
  }
  else if(weatherCondition == 800){//group 800: clear
    gbitmap_set_bounds(weatherIcons, GRect(24,12,12,12));
    layer_mark_dirty(bitmap_layer_get_layer(weatherIcon));
    #ifdef DEBUG_DISPLAY
      APP_LOG(APP_LOG_LEVEL_DEBUG,"update_weather_condition:clear");
    #endif
  }
  else if(weatherCondition < 810){//group 80x: clouds
    gbitmap_set_bounds(weatherIcons, GRect(0,24,12,12));
    layer_mark_dirty(bitmap_layer_get_layer(weatherIcon));
    #ifdef DEBUG_DISPLAY
      APP_LOG(APP_LOG_LEVEL_DEBUG,"update_weather_condition:clouds");
    #endif
  }
  else if(weatherCondition < 910){//group 90x: extreme
    gbitmap_set_bounds(weatherIcons, GRect(12,24,12,12));
    layer_mark_dirty(bitmap_layer_get_layer(weatherIcon));
    #ifdef DEBUG_DISPLAY
      APP_LOG(APP_LOG_LEVEL_DEBUG,"update_weather_condition:extreme");
    #endif
  }
  else{//group 9xx:additional
    gbitmap_set_bounds(weatherIcons, GRect(24,24,12,12));
    layer_mark_dirty(bitmap_layer_get_layer(weatherIcon));
    #ifdef DEBUG_DISPLAY
      APP_LOG(APP_LOG_LEVEL_DEBUG,"update_weather_condition:additional");
    #endif
  }
}

//Fills the background with the background color
static void background_update_callback(Layer *layer, GContext *ctx){
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"background_update_callback:Background color set");
  #endif
  graphics_context_set_fill_color(ctx, colors[BACKGROUND_COLOR]);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornersAll);
}


//Given a color hex string, return a corresponding GColor
static GColor hex_string_to_gcolor(char * string){
  int color,base,i;
  color = 0;
  base = 1;
  for(i=1;i<=6;i++){
      char iChar = string[6-i];
      if(('a' <= iChar) && (iChar <= 'f')) color += ((int) iChar - 'a' + 10) * base;
      else if(('A' <= iChar) && (iChar <= 'F')) color += ((int) iChar - 'A' + 10) * base;
      else if(('0' <= iChar) && (iChar <= '9')) color += ((int) iChar - '0') * base;  
      base *= 16;
    }
  GColor gcolor = GColorFromHEX(color);
  return gcolor;
}

//Given a GColor, copy its hex value into a buffer string
static void gcolor_to_hex_string(char outstring[7], GColor color){
  int colorElem[3] = {color.r,color.g,color.b};
  int i;
  for(i=0;i<3;i++){
    char cval = '0';
    if(colorElem[i] == 0) cval = '0';
    else if(colorElem[i] == 1) cval = '5';
    else if(colorElem[i] == 2) cval = 'A';
    else if(colorElem[i] == 3) cval = 'F';
    outstring[i*2] = cval;
    outstring[(i*2)+1] = cval;
  }
  outstring[6] = '\0';
}

//Makes sure all initialized bitmaps are set to the correct colors
static void apply_colors(){
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"apply_colors:Applying color updates");
  #endif
  //Update background color fill
  if(window_layer != NULL){
    layer_mark_dirty(window_layer);
  }
  apply_bitmap_color(colors[FOREGROUND_COLOR],BGLayers[0]);
  apply_bitmap_color(colors[LINE_COLOR],BGLayers[1]);
  apply_bitmap_color(colors[TEXT_COLOR],dayProg);
  apply_bitmap_color(colors[TEXT_COLOR],weatherIcon);

  //Set text layer colors
  int i;
  for(i=0;i<NUM_EVENTS;i++){
    apply_text_color(eventText[i]);
    #ifdef PBL_COLOR
      //also, set event progress bar color to event color
      char buf [7];
      gcolor_to_hex_string(buf,colors[3]);//set default in case get_event_color fails
      get_event_color(i, buf);
      #ifdef DEBUG_DISPLAY
        APP_LOG(APP_LOG_LEVEL_DEBUG,"apply_colors:Setting progress bar color to %s",buf);
      #endif
      apply_bitmap_color(hex_string_to_gcolor(buf), eventProg[i]);
    #endif
  }
  apply_text_color(timeText);
  apply_text_color(dateText);
  apply_text_color(phone_battery);
  apply_text_color(watch_battery);
  apply_text_color(infoText);
  apply_text_color(weatherText);
}

//Loads colors from persistant storage, or from default definitions
static void initialize_colors(){
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"initialize_colors:initializing");
  #endif
  colors[BACKGROUND_COLOR]=GColorFromHEX(BACKGROUND_COLOR_DEF);
  colors[TEXT_COLOR]=GColorFromHEX(TEXT_COLOR_DEF);
  colors[FOREGROUND_COLOR]=GColorFromHEX(FOREGROUND_COLOR_DEF);
  colors[LINE_COLOR]=GColorFromHEX(LINE_COLOR_DEF);
  colors_initialized=1;
  int i;
  for(i=0;i<NUM_COLORS;i++){
    char buf [7] = "";
    if(persist_exists(PERSIST_KEY_COLORS_BEGIN+i)){//replace default value with saved value
      persist_read_string(PERSIST_KEY_COLORS_BEGIN+i, buf, 7);
      colors[i] = hex_string_to_gcolor(buf);
    }
  }
  apply_colors();//apply the loaded colors to all layers  
}

//save color values to persistant storage
static void save_colors(){
  if(!colors_initialized)initialize_colors();
  int i;
  for(i=0;i<NUM_COLORS;i++){
    char buf [7];
    gcolor_to_hex_string(buf, colors[i]);
    persist_write_string(PERSIST_KEY_COLORS_BEGIN+i,buf);
  }
}

//Applies a color to a single bitmap layer
static void apply_bitmap_color(GColor color,BitmapLayer *layer){
  #ifdef DEBUG_DISPLAY
    char buf[7];
    gcolor_to_hex_string(buf,color);
    if((layer == eventProg[0] || (layer == eventProg[1])))
        APP_LOG(APP_LOG_LEVEL_DEBUG,"apply_color:applying color %s to event progress bar",buf);
    else APP_LOG(APP_LOG_LEVEL_DEBUG,"apply_color:applying color %s to layer",buf);
  #endif
  //load the bitmap's palette (must be 1 bit palletized)
  GColor * palette = gbitmap_get_palette(bitmap_layer_get_bitmap(layer));
  char col1[7],col2[7];
  gcolor_to_hex_string(col1,palette[0]);
  gcolor_to_hex_string(col2,palette[1]);
  //Replace all palette colors except GColorClear
  if(!gcolor_equal(palette[0], GColorClear))
    palette[0].argb = color.argb;
  if(!gcolor_equal(palette[1], GColorClear))
    palette[1].argb = color.argb;
  layer_mark_dirty(bitmap_layer_get_layer(layer));
}

//applies the correct colors to a single text layer
static void apply_text_color(TextLayer *layer){
  text_layer_set_text_color(layer, colors[TEXT_COLOR]);
  text_layer_set_background_color(layer, colors[BACKGROUND_COLOR]);
  layer_mark_dirty(text_layer_get_layer(layer));
}




