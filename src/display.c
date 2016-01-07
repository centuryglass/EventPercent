#include <pebble.h>
#include "display.h"
#include "events.h"
#include "keys.h"

//-----LOCAL VALUE DEFINITIONS-----
#define DEBUG 0 //Set to 1 to enable display debug logging
#define MAX_EVENTSTR_LENGTH 70  //Maximum number of characters for a single event

//Default color definitions
#define COLOR_0_DEF 0x550000 //background color
#define COLOR_1_DEF 0x555555 //background shading color
#define COLOR_2_DEF 0x000000 //line color
#define COLOR_3_DEF 0xFFFF55 //text color


//Text layer size and positioning
//Time text layer
#define TIME_X 5
#define TIME_Y 0
#define TIME_W 130
#define TIME_H 54

//Date text layer
#define DATE_X 5
#define DATE_Y 48
#define DATE_W 70
#define DATE_H 16

//Phone battery text layer
#define PH_BAT_X 100
#define PH_BAT_Y -5
#define PH_BAT_W 35
#define PH_BAT_H 18

//Pebble battery text layer
#define PEB_BAT_X 100
#define PEB_BAT_Y 10
#define PEB_BAT_W 35
#define PEB_BAT_H 18

//Event text layers
#define EVENT_X 5
#define EVENT_Y 80
#define EVENT_DY 40
#define EVENT_W 200
#define EVENT_H 36

//Progress bar positioning
#define PROGRESS_X 2
#define PROGRESS_Y 100
#define PROGRESS_DY 40
#define PROGRESS_W 141
#define PROGRESS_H 19

//Day progress bar positioning
#define DAYPROG_X 3
#define DAYPROG_Y 68
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
//Strings
static char eventStr[NUM_EVENTS][MAX_EVENTSTR_LENGTH];//Event text string
static char phoneBatteryStr [16];//Phone battery string
static char watchBatteryStr [16];//Watch battery string
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

static GBitmap * BGbitmaps[2] = {NULL,NULL}; // background images
static GBitmap * dayProgressBitmap = NULL; //day progress bar image
static GBitmap * progressBitmap[NUM_EVENTS]; //event progress bar image


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
*Re-sizes an event progress bar to correctly mark event progress
*@param bar the progress bar
*@param numEvent the event the progress bar represents
*@param width the width of the progress bar when progress=100%
*@post the progress bar is re-sized
*/
static void update_progress(Layer * bar,int numEvent,int width);
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
*applies a color to a single bitmap layer
*@param color the new color to set
*@param layer an instantiated BitmapLayer set to a 1-bit palletized Gbitmap
*@post all colors in layer besides GColor are set to (color)
*/
static void apply_color(GColor color,BitmapLayer *layer);
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
*Fills the background with the background color
*Called automatically whenever the layer is marked as dirty
*@param layer the main window layer
*@param ctx the graphics context
*/
static void background_update_callback(Layer *layer, GContext *ctx);

//----------PUBLIC FUNCTIONS----------
//initializes all display functionality
void display_init(){
  int i;
  //Window init
  my_window = window_create();
  window_layer = window_get_root_layer(my_window);
  layer_set_clips(window_layer,false);
  layer_set_update_proc(window_layer, background_update_callback);
  //Font init
  coders_crux_16=fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CODERS_CRUX_16));
  coders_crux_48=fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CODERS_CRUX_48));
  //TextLayer init
  strcpy(phoneBatteryStr,"");
  strcpy(watchBatteryStr,"");
  //Event text layers
  for(i=0;i<NUM_EVENTS;i++){
    strcpy(eventStr[i],"-----\n-----");//just copy this in as a default 
    eventText[i] = text_layer_create(GRect(EVENT_X,EVENT_Y+i*EVENT_DY,EVENT_W,EVENT_H)); 
    text_layer_set_font(eventText[i],coders_crux_16);
    text_layer_set_text_alignment(eventText[i],GTextAlignmentLeft);
    text_layer_set_text(eventText[i],eventStr[i]);
    layer_add_child(window_layer,text_layer_get_layer(eventText[i]));
  }
  //Clock, date, and battery layers:
  //Create text layers
  timeText = text_layer_create(GRect(TIME_X,TIME_Y,TIME_W,TIME_H));
  dateText = text_layer_create(GRect(DATE_X,DATE_Y,DATE_W,DATE_H));
  phone_battery = text_layer_create(GRect(PH_BAT_X,PH_BAT_Y,PH_BAT_W,PH_BAT_H));
  watch_battery = text_layer_create(GRect(PEB_BAT_X,PEB_BAT_Y,PEB_BAT_W,PEB_BAT_H));
  //Set font
  text_layer_set_font(timeText,coders_crux_48);
  text_layer_set_font(dateText,coders_crux_16);
  text_layer_set_font(phone_battery,coders_crux_16);
  text_layer_set_font(watch_battery,coders_crux_16);
  //Set text alignment
  text_layer_set_text_alignment(timeText,GTextAlignmentLeft);
  text_layer_set_text_alignment(dateText,GTextAlignmentLeft);
  text_layer_set_text_alignment(phone_battery,GTextAlignmentRight);
  text_layer_set_text_alignment(watch_battery,GTextAlignmentRight);
  //Set default text
  text_layer_set_text(timeText, "time");
  text_layer_set_text(dateText, "date");
  text_layer_set_text(phone_battery, "?");
  text_layer_set_text(watch_battery, "?");
  //Add text layers to window
  layer_add_child(window_layer,text_layer_get_layer(dateText));
  layer_add_child(window_layer,text_layer_get_layer(timeText));
  layer_add_child(window_layer,text_layer_get_layer(watch_battery));
  layer_add_child(window_layer,text_layer_get_layer(phone_battery));
  //BitmapLayer and GBitmap init
  create_bitmapLayers();
  window_stack_push(my_window,true);//display the main window
}

//shuts down all display functionality
void display_deinit(){
  //Unload fonts
  fonts_unload_custom_font(coders_crux_16);
  fonts_unload_custom_font(coders_crux_48);
  //Unload text layers
  int i;
  for(i=0;i<NUM_EVENTS;i++){
    text_layer_destroy(eventText[i]);
  }
  text_layer_destroy(timeText);
  text_layer_destroy(phone_battery);
  text_layer_destroy(watch_battery);
  //Unload BitmapLayers and GBitmaps
  destroy_bitmapLayers();
  //Unload window
  window_destroy(my_window);
}

//Updates display data for events
void update_event_display(){
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"update_event_display");
  int i;
  char buf[160];
  for(i=0;i<NUM_EVENTS;i++){//for each event,
    if(get_event_title(i,buf,160)!=NULL){//if the event exists set title
      strcpy(eventStr[i],buf);
      strcpy(buf,"");
      if(get_event_time_string(i, buf, 160)!=NULL){//check for and set event time string
        strcat(eventStr[i],"\n");
        strcat(eventStr[i],buf);
        if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"update_event_display:Setting event text to %s",eventStr[i]);
      }else if(DEBUG)APP_LOG(APP_LOG_LEVEL_ERROR,"update_event_display:Could not get time info for event %d",i);
      text_layer_set_text(eventText[i], eventStr[i]); //Set event text if at least there is a valid title
    }else if(DEBUG)APP_LOG(APP_LOG_LEVEL_ERROR,"update_event_display:Could not get title for event %d",i);
    //finally, update event progress bar,
    gcolor_to_hex_string(buf,colors[3]);//set default in case get_event_color fails
    update_progress(bitmap_layer_get_layer(eventProg[i]),i,PROGRESS_W);
    get_event_color(i, buf);
    if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"update_event_display:Setting progress bar color to %s",buf);
    apply_color(hex_string_to_gcolor(buf), eventProg[i]);
  }
}

//Updates phone battery display value
void update_phone_battery(char * battery){
  strcpy(phoneBatteryStr,battery);
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"update_phone_battery:phone battery is at %s",phoneBatteryStr);
  text_layer_set_text(phone_battery,phoneBatteryStr);
}

//Updates pebble battery display value
void update_watch_battery(char * battery){
  strcpy(watchBatteryStr,battery);
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"update_watch_battery:watch battery is at %s",watchBatteryStr);
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
  int minutes_per_day = 60*24;
  int minutes_passed_today = tick_time->tm_hour*60 + tick_time->tm_min;
  int newW = DAYPROG_W*minutes_passed_today/minutes_per_day;
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"set_time:day is %d percent complete",minutes_passed_today*100/minutes_per_day);
  layer_set_bounds(bitmap_layer_get_layer(dayProg), GRect(0,0,newW,DAYPROG_H));
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
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"create_bitmapLayers:Creating bitmap layers");
  //Load all GBitmaps
  BGbitmaps[0] = gbitmap_create_with_resource(RESOURCE_ID_BG1);
  BGbitmaps[1] = gbitmap_create_with_resource(RESOURCE_ID_BG2);
  dayProgressBitmap = gbitmap_create_with_resource(RESOURCE_ID_DAY_PROGRESS);
  int i;
  //Create background BitmapLayers
  for(i=0;i<2;i++){
    BGLayers[i] = bitmap_layer_create(layer_get_bounds(window_layer));
    bitmap_layer_set_bitmap(BGLayers[i],BGbitmaps[i]);
    bitmap_layer_set_compositing_mode(BGLayers[i], GCompOpSet);
    layer_add_child(window_layer, bitmap_layer_get_layer(BGLayers[i]));
  }
  //Create event progress bars
  for(i=0;i<NUM_EVENTS;i++){
    progressBitmap[i] = gbitmap_create_with_resource(RESOURCE_ID_PROGRESS);
    eventProg[i] = bitmap_layer_create(GRect(PROGRESS_X, PROGRESS_Y + PROGRESS_DY*i,
                                      PROGRESS_W,PROGRESS_H));
    bitmap_layer_set_bitmap(eventProg[i],progressBitmap[i]);
    bitmap_layer_set_compositing_mode(eventProg[i], GCompOpSet);
    layer_add_child(window_layer,bitmap_layer_get_layer(eventProg[i]));
  }
  //Create day progress bar BitmapLayer
  dayProg = bitmap_layer_create(GRect(DAYPROG_X, DAYPROG_Y,
                               DAYPROG_W,DAYPROG_H));
  bitmap_layer_set_bitmap(dayProg,dayProgressBitmap);
  bitmap_layer_set_compositing_mode(dayProg, GCompOpSet);
  layer_add_child(window_layer,bitmap_layer_get_layer(dayProg));
  initialize_colors();//Load and set colors
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
}

//Re-sizes an event progress bar to correctly mark event progress
static void update_progress(Layer * bar,int numEvent,int width){
  GRect bounds = layer_get_bounds(bar);
  int percent = -1;
  if(numEvent != -1)percent = get_percent_complete(numEvent);
  if(percent < 3) percent = 3; //always show a bit of the event color
  int newW = width * percent /100;
  //If the event is x% complete, set progress bar width to x% of the default
  layer_set_bounds(bar, GRect(0,0, newW, bounds.size.h));
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"update_progress:Changing event %d width from %d to %d",numEvent,width,newW);
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

//Applies a color to a single bitmap layer
static void apply_color(GColor color,BitmapLayer *layer){
  if(DEBUG){
    char buf[7];
    gcolor_to_hex_string(buf,color);
    if((layer == eventProg[0] || (layer == eventProg[1])))
        APP_LOG(APP_LOG_LEVEL_DEBUG,"apply_color:applying color %s to event progress bar",buf);
    else APP_LOG(APP_LOG_LEVEL_DEBUG,"apply_color:applying color %s to layer",buf);
  }
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

//Makes sure all initialized bitmaps are set to the correct colors
static void apply_colors(){
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"apply_colors:Applying color updates");
  //Update background color fill
  if(window_layer != NULL){
    layer_mark_dirty(window_layer);
  }
  apply_color(colors[1],BGLayers[0]);
  apply_color(colors[2],BGLayers[1]);
  apply_color(colors[3],dayProg);
  //Set text layer colors
  int i;
  for(i=0;i<NUM_EVENTS;i++){
    text_layer_set_text_color(eventText[i], colors[3]);
    text_layer_set_background_color(eventText[i], colors[0]);
    //also, set event progress bar color to event color
    char buf [7];
    gcolor_to_hex_string(buf,colors[3]);//set default in case get_event_color fails
    get_event_color(i, buf);
    if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"apply_colors:Setting progress bar color to %s",buf);
    apply_color(hex_string_to_gcolor(buf), eventProg[i]);
  }
  text_layer_set_text_color(timeText, colors[3]);
  text_layer_set_text_color(dateText, colors[3]);
  text_layer_set_text_color(phone_battery, colors[3]);
  text_layer_set_text_color(watch_battery, colors[3]);
  text_layer_set_background_color(timeText, colors[0]);
  text_layer_set_background_color(dateText, colors[0]);
  text_layer_set_background_color(phone_battery, colors[0]);
  text_layer_set_background_color(watch_battery, colors[0]);
  layer_mark_dirty(text_layer_get_layer(timeText));
  layer_mark_dirty(text_layer_get_layer(dateText));
  layer_mark_dirty(text_layer_get_layer(phone_battery));
  layer_mark_dirty(text_layer_get_layer(watch_battery));
  
}

//Loads colors from persistant storage, or from default definitions
static void initialize_colors(){
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"initialize_colors:initializing");
  colors[0]=GColorFromHEX(COLOR_0_DEF);
  colors[1]=GColorFromHEX(COLOR_1_DEF);
  colors[2]=GColorFromHEX(COLOR_2_DEF);
  colors[3]=GColorFromHEX(COLOR_3_DEF);
  colors_initialized=1;
  int i;
  for(i=0;i<NUM_COLORS;i++){
    char buf [7] = "";
    if(persist_exists(KEY_COLORS_BEGIN+i)){//replace default value with saved value
      persist_read_string(KEY_COLORS_BEGIN+i, buf, 7);
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
    persist_write_string(KEY_COLORS_BEGIN+i,buf);
  }
}

//Fills the background with the background color
static void background_update_callback(Layer *layer, GContext *ctx){
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"background_update_callback:Background color set");
  graphics_context_set_fill_color(ctx, colors[0]);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornersAll);
}

