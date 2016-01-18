#include <pebble.h>
#include "display.h"
#include "events.h"
#include "storage_keys.h"
#include "util.h"

//-----LOCAL VALUE DEFINITIONS-----
//#define DEBUG_DISPLAY //uncomment to enable display debug logging

//Color definitions
typedef enum{
  BACKGROUND_COLOR,
  FOREGROUND_COLOR,
  LINE_COLOR,
  TEXT_COLOR
} colorID;

//default colors
enum{
  BACKGROUND_COLOR_DEF = PBL_IF_COLOR_ELSE(0x000000,0xFFFFFF),
  FOREGROUND_COLOR_DEF = PBL_IF_COLOR_ELSE(0x000055,0x555555),
  LINE_COLOR_DEF = PBL_IF_COLOR_ELSE(0x005555,0x000000),
  TEXT_COLOR_DEF = PBL_IF_COLOR_ELSE(0x55FFFF,0x000000)
};


//Distance between event items
#define EVENT_DY PBL_IF_ROUND_ELSE(33,40)

//Display size
#define DISPLAY_WIDTH PBL_IF_ROUND_ELSE(180,144)
#define DISPLAY_HEIGHT PBL_IF_ROUND_ELSE(180,168)


//TextLayer definitions
typedef enum{
  TEXTLAYER_INFOTEXT,//configurable infotext display
  TEXTLAYER_PHONE_BATTERY,//phone battery percentage
  TEXTLAYER_WEATHERTEXT,//Weather info display
  /**
  *The above items are saved in persistant storage, and should have
  *values corresponding to the correct persistant storage keys
  */
  TEXTLAYER_PEBBLE_BATTERY,//pebble battery percentage
  TEXTLAYER_TIME,//current time
  TEXTLAYER_DATE,//current date
  TEXTLAYER_EVENTS_BEGIN,//first index of NUM_EVENTS sequential text layers
  NUM_TEXTLAYERS = 6 + NUM_EVENTS
} TextLayerID;

//TextLayer frame definitions: 
#define INFOTEXT_FRAME        GRect(PBL_IF_ROUND_ELSE(27,17), PBL_IF_ROUND_ELSE(17,1), 70, 18)
#define PHONE_BATTERY_FRAME   GRect(PBL_IF_ROUND_ELSE(111,100), PBL_IF_ROUND_ELSE(11,-5), 35, 18)
#define WEATHER_FRAME         GRect(PBL_IF_ROUND_ELSE(119,108), PBL_IF_ROUND_ELSE(48,43), 35, 18)
#define PEBBLE_BATTERY_FRAME  GRect(PBL_IF_ROUND_ELSE(111,100), PBL_IF_ROUND_ELSE(27,10), 35, 18)
#define TIME_TEXT_FRAME       GRect(PBL_IF_ROUND_ELSE(17,6), PBL_IF_ROUND_ELSE(19,0), 130, 54)
#define DATE_TEXT_FRAME       GRect(PBL_IF_ROUND_ELSE(18,5), PBL_IF_ROUND_ELSE(65,49), 70, 16)
#define FIRST_EVENT_FRAME     GRect(PBL_IF_ROUND_ELSE(23,5), PBL_IF_ROUND_ELSE(81,80), 200, 36)

//BitmapLayer definitions
typedef enum{
  BITMAP_LAYER_WEATHER_ICONS,//weather condition icons
  BITMAP_LAYER_FOREGROUND,//first window decoration layer
  BITMAP_LAYER_LINE,//second window decoration layer
  BITMAP_LAYER_DAY_PROGRESS,//day progress bar
  BITMAP_LAYER_EVENT_PROGRESS_BEGIN, //first index of NUM_EVENTS sequential bitmap layers
  NUM_BITMAP_LAYERS = 4 + NUM_EVENTS
} BitmapLayerID;

//BitmapLayer frame definitions: 
#define WEATHER_ICON_FRAME    GRect(PBL_IF_ROUND_ELSE(138,127), PBL_IF_ROUND_ELSE(64,49), 12, 12)
#define FOREGROUND_FRAME      GRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT)
#define LINE_FRAME            GRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT)
#define DAY_PROGRESS_FRAME    GRect(PBL_IF_ROUND_ELSE(16,3), PBL_IF_ROUND_ELSE(83,68), 132, 5)
#define FIRST_EVENTPROG_FRAME GRect(PBL_IF_ROUND_ELSE(20,2), PBL_IF_ROUND_ELSE(101,100), 141, 19)

//Weather Condition definitions
enum{
  THUNDERSTORM, //GROUP 2XX
  DRIZZLE,  //GROUP 3XX
  RAIN,  //GROUP 5XX
  SNOW,  //GROUP 6XX
  ATMOSPHERE,  //GROUP 7XX
  CLEAR,  //GROUP 800
  CLOUDS,  //GROUP 80X
  EXTREME,  //GROUP 90X
  ADDITIONAL,  //GROUP 9XX
  NUM_WEATHER_CONDITIONS = 9
};

//----------LOCAL VARIABLES----------
Window * window = NULL;//Watchface window

int weatherCondition = 0;//weather condition code
//upper bounds(exclusive) for each weather condition category

int weatherCondCodeBounds [] = {
  300,//thunderstorm-2xx
  400,//drizzle-3xx
  600,//rain-5xx
  800,//atmosphere-7xx
  801,//clear-800
  810,//clouds-80x
  910,//extreme-90x
  1000,//additional-9xx
};

//---TextLayers---
TextLayer * textLayers[NUM_TEXTLAYERS] = {NULL};

//Corresponding strings
char * displayStrings[NUM_TEXTLAYERS] = {NULL};

//---BitmapLayers---
BitmapLayer * bitmapLayers[NUM_BITMAP_LAYERS] = {NULL};
//Corresponding bitmaps
GBitmap * gbitmaps[NUM_BITMAP_LAYERS] = {NULL};

//---Fonts---
GFont coders_crux_16;//small size
GFont coders_crux_48;//large size (for time only)

//---Colors---
GColor colors[NUM_COLORS]; //all colors used
int colors_initialized = 0; //indicates whether initialize_colors() has been run

//----------STATIC FUNCTION DEFINITIONS----------
//---Bitmap Functions---
static void create_bitmapLayers();
  //Creates all BitmapLayers, loads and sets their respective GBitmaps
static void destroy_bitmapLayers();
  //Unloads all BitmapLayers, along with their respective GBitmaps
static BitmapLayer * init_bitmap_layer(GBitmap ** bitmap, int resourceID,GRect bounds);
  //Creates a bitmap layer with the given parameters, and adds it to the main window
static void update_progress(Layer * bar,int percent,int width);
  //Re-sizes a progress bar
static GRect get_bitmap_layer_frame(BitmapLayerID bitmapID);
  //Gets the appropriate frame for initializing a bitmap layer
static uint32_t get_bitmap_layer_resource(BitmapLayerID bitmapID);
  //Gets the appropriate bitmap resource id for initializing a bitmap layer
static colorID get_bitmap_colorID(BitmapLayerID bitmapID);
  //Get the colorID assigned to a bitmap layer

//---TextLayer functions---
static TextLayer * init_text_layer(GRect bounds,char * text,GFont font,GTextAlignment align);
  //Creates a text layer with the given parameters, and adds it to the main window
static void update_weather_condition();
  //Updates weather condition display
static GRect get_text_frame(TextLayerID textID);
  //Gets the appropriate frame for initializing a text layer
static GTextAlignment get_text_align(TextLayerID textID);
  //gets the appropriate alignment for a text layer

//--Color functions---
static void initialize_colors();
  //Loads colors from persistant storage, or from default definitions
static void save_colors();
  //save color values to persistant storage
static void apply_colors();
  //Makes sure all initialized bitmaps are set to the correct colors
static void apply_bitmap_color(GColor color,BitmapLayer *layer);
  //Applies a color to a single bitmap layer
static void apply_text_color(TextLayer *layer);
  //applies the correct colors to a single text layer
static void background_update_callback(Layer *layer, GContext *ctx);
  //Fills the background with the background color
static void gcolor_to_hex_string(char outstring[7], GColor color);
  //Given a GColor, copy its hex value into a buffer string
static GColor hex_string_to_gcolor(char * string);
  //Given a color hex string, return a corresponding GColor

//----------PUBLIC FUNCTIONS----------
//initializes all display functionality
void display_init(){
  //Window init
  window = window_create();
  Layer * window_layer = window_get_root_layer(window);
  layer_set_update_proc(window_layer, background_update_callback);
  //Font init
  coders_crux_16=
    fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CODERS_CRUX_16));
  coders_crux_48=
    fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CODERS_CRUX_48));
    //time text only
  
  //Display string init
  #ifdef DEBUG_DISPLAY
  APP_LOG(APP_LOG_LEVEL_DEBUG,"display_init: initializing strings");
  #endif
  char buf[64] = "---";
  for(int i = 0; i < NUM_TEXTLAYERS; i++){
    if(i<PERSIST_KEY_STRINGS_END && persist_exists(i))
        persist_read_string(i, buf, 64);
    displayStrings[i] = NULL;
    displayStrings[i] = malloc_strcpy(displayStrings[i], buf);
    strcpy(buf,"---");
  }
  //TextLayer init
  #ifdef DEBUG_DISPLAY
  APP_LOG(APP_LOG_LEVEL_DEBUG,"display_init: initializing text layers");
  #endif
  //non-event text layers
  for(int i = 0; i < TEXTLAYER_EVENTS_BEGIN; i++){
    textLayers[i] = 
      init_text_layer(get_text_frame(i),
                      displayStrings[i],
                      coders_crux_16,
                      get_text_align(i));
  }
  //Event text layers
  for(int i=0;i<NUM_EVENTS;i++){
    GRect frame = get_text_frame(TEXTLAYER_EVENTS_BEGIN);
    frame.origin.y += (EVENT_DY * i);
    textLayers[TEXTLAYER_EVENTS_BEGIN + i] = 
      init_text_layer(frame, 
                      displayStrings[TEXTLAYER_EVENTS_BEGIN + i],
                      coders_crux_16,
                      get_text_align(TEXTLAYER_EVENTS_BEGIN)); 
  }
  //set time text to the larger font
  text_layer_set_font(textLayers[TEXTLAYER_TIME], coders_crux_48);
  
  
  //Bitmap init
  create_bitmapLayers();
  window_stack_push(window,true);//display the main window
}

//shuts down all display functionality
void display_deinit(){
  //save display information
  for(int i = 0; i < PERSIST_KEY_STRINGS_END; i++){
    if(displayStrings[i] != NULL)
        persist_write_string(i,displayStrings[i]);
  }
  //Unload text layers and strings
  for(int i=0;i<NUM_TEXTLAYERS;i++){
    text_layer_destroy(textLayers[i]);
    if(displayStrings[i] != NULL){
      free(displayStrings[i]);
      displayStrings[i] = NULL;
    }
  }
  //Unload fonts
  fonts_unload_custom_font(coders_crux_16);
  fonts_unload_custom_font(coders_crux_48);
  //Unload BitmapLayers and GBitmaps
  destroy_bitmapLayers();
  //Unload window
  window_destroy(window);
}

//Updates display data for events
void update_event_display(){
  #ifdef DEBUG_DISPLAY
  APP_LOG(APP_LOG_LEVEL_DEBUG,"update_event_display: starting update");
  #endif
  int i;
  char buf[64];
  for(i=0;i<NUM_EVENTS;i++){//for each event,
    int index = TEXTLAYER_EVENTS_BEGIN + i;
    char eventStr[64];
    if(get_event_title(i,buf,64)!=NULL){//if the event exists set title
      strcpy(eventStr,buf);
      strcpy(buf,"");
      if(get_event_time_string(i, buf, 64)!=NULL){//check for and set event time string
        strcat(eventStr,"\n");
        strcat(eventStr,buf);
        #ifdef DEBUG_DISPLAY
        APP_LOG(APP_LOG_LEVEL_DEBUG,"update_event_display:Setting event text to %s",eventStr);
        #endif     
      }
      #ifdef DEBUG_DISPLAY
      else APP_LOG(APP_LOG_LEVEL_ERROR,"update_event_display:Could not get time info for event %d",i);
      #endif
      displayStrings[index] = 
        malloc_set_text(textLayers[index], displayStrings[index], eventStr);
    }
    #ifdef DEBUG_DISPLAY
    else APP_LOG(APP_LOG_LEVEL_ERROR,"update_event_display:Could not get title for event %d",i);
    #endif
    //finally, update event progress bar,
    BitmapLayer * progressBar = bitmapLayers[BITMAP_LAYER_EVENT_PROGRESS_BEGIN+i];
    update_progress(bitmap_layer_get_layer(progressBar),
                    get_percent_complete(i),
                    get_bitmap_layer_frame(BITMAP_LAYER_EVENT_PROGRESS_BEGIN).size.w);
    
    //set progress bar color if display is in color
    #ifdef PBL_COLOR
    get_event_color(i, buf); 
    #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_event_display:Setting progress bar color to %s",buf);
    #endif
    apply_bitmap_color(hex_string_to_gcolor(buf), progressBar);
    #endif
  }
}

//Directly updates display text
void update_display_text(char * newText, DisplayTextType textType){
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_display_text: New text =%s",newText);
  #endif
  int textLayerIndex = -1;
  switch(textType){
    case TEXT_INFOTEXT:
      textLayerIndex = TEXTLAYER_INFOTEXT;
      break;
    case TEXT_PHONE_BATTERY:
      textLayerIndex = TEXTLAYER_PHONE_BATTERY;
      break;
    case TEXT_PEBBLE_BATTERY:
      textLayerIndex = TEXTLAYER_PEBBLE_BATTERY;
      break;
  }
  if(textLayerIndex == -1){
    APP_LOG(APP_LOG_LEVEL_ERROR,"update_display_text:invalid text field %d!",textType);
    return;
  }
  displayStrings[textLayerIndex] = 
        malloc_set_text(textLayers[textLayerIndex], 
                        displayStrings[textLayerIndex], 
                        newText);
}

//Sets the displayed time and date
void set_time(time_t newTime){
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"set_time: updating time display");
  #endif
  struct tm *tick_time = localtime(&newTime);
  //set display time
  char buffer[12];
  strftime(buffer, sizeof(buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"set_time: setting time string to %s",buffer);
  #endif
  displayStrings[TEXTLAYER_TIME] = 
        malloc_set_text(textLayers[TEXTLAYER_TIME], 
                        displayStrings[TEXTLAYER_TIME], 
                        buffer);
  //set display date
  strftime(buffer, sizeof(buffer),"%e %b 20%y", tick_time);
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"set_time: setting date string to %s",buffer);
  #endif
  displayStrings[TEXTLAYER_DATE] = 
        malloc_set_text(textLayers[TEXTLAYER_DATE], 
                        displayStrings[TEXTLAYER_DATE], 
                        buffer);
  //Update day progress bar
  int minutes_passed_today = tick_time->tm_hour*60 + tick_time->tm_min;
  int dayPercent = minutes_passed_today*100/(60*24);
  update_progress(bitmap_layer_get_layer(bitmapLayers[BITMAP_LAYER_DAY_PROGRESS]),
                  dayPercent,
                  get_bitmap_layer_frame(BITMAP_LAYER_DAY_PROGRESS).size.w);
}

//Updates weather display
void update_weather(int degrees, int condition){
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_weather:Data received: %d degrees, condition code %d",degrees,condition);
  #endif
  //set degrees
  char weatherStr[16];
  snprintf(weatherStr,16,"%d°",degrees);
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_weather:final weather string is %s",weatherStr);
  #endif
  displayStrings[TEXTLAYER_WEATHERTEXT] = 
        malloc_set_text(textLayers[TEXTLAYER_WEATHERTEXT], 
                        displayStrings[TEXTLAYER_WEATHERTEXT], 
                        weatherStr);
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

//---BITMAP FUNCTIONS--
/**
*creates all BitmapLayers, loads and sets their respective GBitmaps,
*and adds them to the main window
*/
static void create_bitmapLayers(){
  char debugBuf[32];
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"create_bitmapLayers:Creating bitmap layers");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "create_bitmapLayers: memory usage is %d",(int)heap_bytes_used());
  #endif
  //create non-event BitmapLayers
  for(int i = 0; i < BITMAP_LAYER_EVENT_PROGRESS_BEGIN; i++){
    bitmapLayers[i] = init_bitmap_layer(&gbitmaps[i],
                                        get_bitmap_layer_resource(i),
                                        get_bitmap_layer_frame(i));
  }
  //create event BitmapLayers
  for(int i=0;i<NUM_EVENTS;i++){
    GRect frame = get_bitmap_layer_frame(BITMAP_LAYER_EVENT_PROGRESS_BEGIN);
    frame.origin.y += (EVENT_DY * i);
    bitmapLayers[BITMAP_LAYER_EVENT_PROGRESS_BEGIN + i] =
        init_bitmap_layer(&gbitmaps[BITMAP_LAYER_EVENT_PROGRESS_BEGIN + i],
                          get_bitmap_layer_resource(BITMAP_LAYER_EVENT_PROGRESS_BEGIN),
                          frame);
  }
 
  initialize_colors();//Load and set colors
  if(persist_exists(PERSIST_KEY_WEATHER_COND)){//load weather condition
    weatherCondition = persist_read_int(PERSIST_KEY_WEATHER_COND);
  }else weatherCondition = 800;//default to clear
  update_weather_condition();
}

/**
*Unloads all BitmapLayers, along with their respective GBitmaps
*@post all GBitmaps and BitmapLayers are unloaded and set to null
*/
static void destroy_bitmapLayers(){
  char debugBuf[32];
  for(int i=0;i<NUM_BITMAP_LAYERS;i++){
    bitmap_layer_destroy(bitmapLayers[i]);
    gbitmap_destroy(gbitmaps[i]);
    bitmapLayers[i] = NULL;
    gbitmaps[i] = NULL;
  }
}

/**
*Creates a bitmap layer with the given parameters, and adds it to the main window
*@param bitmap holds the layer's newly loaded GBitmap
*@param resourceID the bitmap resource to load
*@param bounds layer bounding box
*@return the initialized bitmap layer
*/
static BitmapLayer * init_bitmap_layer(GBitmap ** bitmap, int resourceID,GRect bounds){
  BitmapLayer * bitmapLayer = bitmap_layer_create(bounds);
  *bitmap = gbitmap_create_with_resource(resourceID);
  bitmap_layer_set_bitmap(bitmapLayer,*bitmap);
  bitmap_layer_set_compositing_mode(bitmapLayer, GCompOpSet);
  layer_add_child(window_get_root_layer(window),bitmap_layer_get_layer(bitmapLayer));
  return bitmapLayer;
}

/**
*Re-sizes a progress bar
*@param bar the progress bar
*@param percent new bar percentage
*@param width the width of the progress bar when progress=100%
*@post the progress bar is re-sized
*/
static void update_progress(Layer * bar,int percent,int width){
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_progress:Updating progress bar to %d percent",percent);
  #endif
  GRect bounds = layer_get_bounds(bar);
  if(percent < 3) percent = 3; //always show a bit of the event color
  int newW = width * percent /100;
  //If the event is x% complete, set progress bar width to x% of the default
  layer_set_bounds(bar, GRect(0,0, newW, bounds.size.h));
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_progress:Changing width from %d to %d",width,newW);
  #endif
}

/**
*Gets the appropriate frame for initializing a bitmap layer
*@param bitmapID the target bitmap layer
*@return the frame to assign to the layer
*/
static GRect get_bitmap_layer_frame(BitmapLayerID bitmapID){
  switch(bitmapID){
    case BITMAP_LAYER_WEATHER_ICONS:
      return WEATHER_ICON_FRAME;
    case BITMAP_LAYER_FOREGROUND:
      return FOREGROUND_FRAME;
    case BITMAP_LAYER_LINE:
      return LINE_FRAME;
    case BITMAP_LAYER_DAY_PROGRESS:
      return DAY_PROGRESS_FRAME;
    case BITMAP_LAYER_EVENT_PROGRESS_BEGIN:
      return FIRST_EVENTPROG_FRAME;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR,"get_bitmap_layer_frame:Invalid bitmapID!");
      return GRect(0,0,0,0);
  }
}
  
/**
*Gets the appropriate bitmap resource id for initializing a bitmap layer
*@param bitmapID the target bitmap layer
*@return the resourceID to load
*/
static uint32_t get_bitmap_layer_resource(BitmapLayerID bitmapID){
  switch(bitmapID){
    case BITMAP_LAYER_WEATHER_ICONS:
      return RESOURCE_ID_WEATHER_ICONS;
    case BITMAP_LAYER_FOREGROUND:
      return RESOURCE_ID_BG1;
    case BITMAP_LAYER_LINE:
      return RESOURCE_ID_BG2;
    case BITMAP_LAYER_DAY_PROGRESS:
      return RESOURCE_ID_DAY_PROGRESS;
    case BITMAP_LAYER_EVENT_PROGRESS_BEGIN:
      return RESOURCE_ID_PROGRESS;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR,"get_bitmap_layer_resource:Invalid bitmapID!");
      return -1;
  }
}

/**
*Get the colorID assigned to a bitmap layer
*@param bitmapID the target bitmap layer
*@return the associated colorID
*/
static colorID get_bitmap_colorID(BitmapLayerID bitmapID){
  switch(bitmapID){
    case BITMAP_LAYER_WEATHER_ICONS:
      return TEXT_COLOR;
    case BITMAP_LAYER_FOREGROUND:
      return FOREGROUND_COLOR;
    case BITMAP_LAYER_LINE:
      return LINE_COLOR;
    case BITMAP_LAYER_DAY_PROGRESS:
      return TEXT_COLOR;
    case BITMAP_LAYER_EVENT_PROGRESS_BEGIN:
      return TEXT_COLOR;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR,"get_bitmap_colorID:Invalid bitmapID!");
      return -1;
  }
}
  

//---TEXTLAYER FUNCTIONS---
/**
*Creates a text layer with the given parameters, and adds it to the main window
*@param bounds layer bounding box
*@param text layer initial text
*@param font layer font
*@param align text alignment
*@return the initialized text layer
*/
static TextLayer * init_text_layer(GRect bounds,char * text,GFont font,GTextAlignment align){
  TextLayer * textLayer = text_layer_create(bounds);
  text_layer_set_font(textLayer,font);
  text_layer_set_text_alignment(textLayer,align);
  text_layer_set_text(textLayer, text);
  layer_add_child(window_get_root_layer(window),text_layer_get_layer(textLayer));
  return textLayer;
}

/**
*Updates weather condition display
*/
static void update_weather_condition(){
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_weather_condition:Setting weather condition icon for id %d",weatherCondition);
  #endif
  //set condition icon
  for(int i = 0; i < NUM_WEATHER_CONDITIONS; i++){
    if(weatherCondition < weatherCondCodeBounds[i]){
      //condition found, set bitmap bounds to section i of 9
      int x = (weatherCondition % 3) * 12;
      int y = (weatherCondition / 3) * 12;
      gbitmap_set_bounds(gbitmaps[BITMAP_LAYER_WEATHER_ICONS], GRect(x,y,12,12));
      layer_mark_dirty(bitmap_layer_get_layer(bitmapLayers[BITMAP_LAYER_WEATHER_ICONS]));
      break;
    }
  }
}

/**
*Gets the appropriate frame for initializing a text layer
*@param textID the target text layer
*@return the frame to assign to the layer
*/
static GRect get_text_frame(TextLayerID textID){
  switch(textID){
    case TEXTLAYER_INFOTEXT:
      return INFOTEXT_FRAME;
    case TEXTLAYER_PHONE_BATTERY:
      return PHONE_BATTERY_FRAME;
    case TEXTLAYER_WEATHERTEXT:
      return WEATHER_FRAME;
    case TEXTLAYER_PEBBLE_BATTERY:
      return PEBBLE_BATTERY_FRAME;
    case TEXTLAYER_TIME:
      return TIME_TEXT_FRAME;
    case TEXTLAYER_DATE:
      return DATE_TEXT_FRAME;
    case TEXTLAYER_EVENTS_BEGIN:
      return FIRST_EVENT_FRAME;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR,"get_text_frame:Invalid textID!");
      return GRect(0,0,0,0);
  }
}

/**
*gets the appropriate alignment for a text layer
*@param textID the target text layer
*return the alignment to assign to the layer
*/
static GTextAlignment get_text_align(TextLayerID textID){
  switch(textID){
    case TEXTLAYER_INFOTEXT:
      return GTextAlignmentLeft;
    case TEXTLAYER_PHONE_BATTERY:
      return GTextAlignmentRight;
    case TEXTLAYER_WEATHERTEXT:
      return GTextAlignmentLeft;
    case TEXTLAYER_PEBBLE_BATTERY:
      return GTextAlignmentRight;
    case TEXTLAYER_TIME:
      return GTextAlignmentLeft;
    case TEXTLAYER_DATE:
      return GTextAlignmentLeft;
    case TEXTLAYER_EVENTS_BEGIN:
      return GTextAlignmentLeft;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR,"get_text_align:Invalid textID!");
      return GTextAlignmentLeft;
  }
}


//--COLOR FUNCTIONS---
/**
*Loads colors from persistant storage, or from default definitions
*if no stored colors are found
*@post all colors are initialized and applied to layers
*/
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

/**
*save color values to persistant storage
*@post all NUM_COLORS colors are saved
*/
static void save_colors(){
  if(!colors_initialized)initialize_colors();
  int i;
  for(i=0;i<NUM_COLORS;i++){
    char buf [7];
    gcolor_to_hex_string(buf, colors[i]);
    persist_write_string(PERSIST_KEY_COLORS_BEGIN+i,buf);
  }
}

/**
*Makes sure all initialized layers are set to the correct colors
*@post each layer has its colors set to a value stored in (color)
*note: this excludes event progress bars, which are set at event color
*/
static void apply_colors(){
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"apply_colors:Applying color updates");
  #endif
  //Update background color fill
  layer_mark_dirty(window_get_root_layer(window));
  
  //update bitmap layer colors
  for(int i = 0; i < BITMAP_LAYER_EVENT_PROGRESS_BEGIN; i++){
    apply_bitmap_color(colors[get_bitmap_colorID(i)],bitmapLayers[i]);
  }
  //set non-event text layer colors
  for(int i=0; i<TEXTLAYER_EVENTS_BEGIN; i++){
    apply_text_color(textLayers[i]);
  }
  //Set event colors
  for(int i=0;i<NUM_EVENTS;i++){
    apply_text_color(textLayers[TEXTLAYER_EVENTS_BEGIN + i]);
    #ifdef PBL_COLOR
      //also, set event progress bar color to event color
      char buf [7];
      gcolor_to_hex_string(buf,colors[3]);//set default in case get_event_color fails
      get_event_color(i, buf);
      #ifdef DEBUG_DISPLAY
        APP_LOG(APP_LOG_LEVEL_DEBUG,"apply_colors:Setting progress bar color to %s",buf);
      #endif
      apply_bitmap_color(hex_string_to_gcolor(buf), bitmapLayers[BITMAP_LAYER_EVENT_PROGRESS_BEGIN + i]);
    #endif
  }
}

/**
*applies a color to a single bitmap layer
*@param color the new color to set
*@param layer an instantiated BitmapLayer set to a 2-bit palletized Gbitmap
*@post all colors in layer besides GColor are set to (color)
*/
static void apply_bitmap_color(GColor color,BitmapLayer *layer){
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

/**
*applies the correct colors to a single text layer
*@param layer an instantiated TextLayer
*/
static void apply_text_color(TextLayer *layer){
  text_layer_set_text_color(layer, colors[TEXT_COLOR]);
  text_layer_set_background_color(layer, colors[BACKGROUND_COLOR]);
  layer_mark_dirty(text_layer_get_layer(layer));
}

/**
*Fills the background with the background color
*Called automatically whenever the layer is marked as dirty
*@param layer the main window layer
*@param ctx the graphics context
*/
static void background_update_callback(Layer *layer, GContext *ctx){
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"background_update_callback:Background color set");
  #endif
  graphics_context_set_fill_color(ctx, colors[BACKGROUND_COLOR]);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornersAll);
}

/**
*Given a color hex string, returns a corresponding GColor
*@param string a cstring set to a valid six digit hex color value
*@return the correct GColor
*/
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

/**
*Given a GColor, copies its hex value into a buffer string
*@param outstring a buffer of at least size 7
*@param color the GColor to convert to a string
*/
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
















