#include <pebble.h>
#include "display_core.h"
#include "storage_keys.h"
#include "util.h"
#include "debug.h"


//-----LOCAL VALUE DEFINITIONS-----
//#define DEBUG_DISPLAY
//Defines valid layer types
typedef enum{
  TEXT_LAYER_TYPE,
  IMAGE_LAYER_TYPE,
}DisplayType;

//Layer data struct
typedef struct dLayer{
  Layer * layer;
  DisplayType type;
  ColorID colorID;
  ColorID colorID2;
  int dataIndex;
} DisplayLayer;
//----------LOCAL VARIABLES----------
static GFont * fonts = NULL;

static GColor * colors = NULL;
static int8_t numColors = 0;

Window * window = NULL;
static bool initialized = false;

DisplayLayer * displayLayers = NULL;

static char ** displayStrings = NULL;
static int numDisplayStrings = 0;

static int themeResID = 0;
static int themeID = 0;

//----------STATIC FUNCTION DECLARATIONS----------
//Initialization functions:
static void load_colors();
  //Load saved or default display colors
static void load_text(int16_t layerIndices[NUM_LAYERS]);
  //Initialize display strings and load saved display text
static TextLayer * init_text_layer(GRect bounds,char * text,GFont font,GTextAlignment align,int marginHeight);
  //Creates a text layer with the given parameters, and adds it to the main window
static Layer * init_image_layer(GRect frame);
  //Creates an image layer with the given parameters, and adds it to the main window

//Color functions:
static void save_colors();
  //save color values to persistant storage
static void apply_colors();
  //Makes sure all initialized bitmaps are set to the correct colors
static void image_update_callback(Layer *layer, GContext *ctx);
  //Re-draws an image layer
static void background_update_callback(Layer *layer, GContext *ctx);
  //Fills the background with the background color
static void gcolor_to_hex_string(char outstring[7], GColor color);
  //Given a GColor, copy its hex value into a buffer string
static GColor hex_string_to_gcolor(char * string);
  //Given a color hex string, return a corresponding GColor

//----------PUBLIC FUNCTIONS----------
//initializes all display functionality
void display_create(int themeResource,int theme ,int fontIDs[],int fontMargins[]){
  if(initialized)return;
  themeResID = themeResource;
  themeID = theme;
  
   //Window init
  window_stack_pop_all(true);
  if(window == NULL) window = window_create();
  //Font init
  if(fonts == NULL)fonts = malloc(sizeof(GFont) * NUM_FONTS);
  for(int i = 0; i < NUM_FONTS; i++){
    fonts[i] = fonts_load_custom_font(resource_get_handle(fontIDs[i]));
  } 
  //prepare theme resource
  ResHandle themeRes = resource_get_handle(themeResource);
  int index = 0;
  //read default colors
  index += resource_load_byte_range(themeRes, index, (uint8_t *)&numColors, 1);
  load_colors();
  index += numColors*6;
  #ifdef DEBUG_DISPLAY
  APP_LOG(APP_LOG_LEVEL_DEBUG,"display_create:Found numColors = %d",(int)numColors);
  #endif
  //read layer indices
  int8_t numLayers;
  index += resource_load_byte_range(themeRes, index, (uint8_t *)&numLayers, 1);
  #ifdef DEBUG_DISPLAY
  APP_LOG(APP_LOG_LEVEL_DEBUG,"display_create:Found numLayers = %d",(int)numLayers);
  #endif
  int16_t layerIndices [NUM_LAYERS];
  resource_load_byte_range(themeRes, index, (uint8_t *)layerIndices, sizeof(int16_t) * numLayers);
  
  load_text(layerIndices);
  int textLayerNum = 0;//for assigning strings to text layers
  //create each layer
  if(displayLayers == NULL){
    displayLayers = malloc(sizeof(DisplayLayer) * numLayers);
  }
  for(int i = 0; i < numLayers; i++){
     #ifdef DEBUG_DISPLAY
        APP_LOG(APP_LOG_LEVEL_DEBUG,"display_create: creating layer %d",i);
        #endif
    index = layerIndices[i];
    int16_t frameVals[4];
    index += resource_load_byte_range(themeRes, index, (uint8_t *)&(displayLayers[i].type),1);
    index += resource_load_byte_range(themeRes, index, (uint8_t *)frameVals,8);
    index += resource_load_byte_range(themeRes, index, (uint8_t *)&(displayLayers[i].colorID),1);
    GRect frame = GRect(frameVals[0],frameVals[1],frameVals[2],frameVals[3]);
    #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"display_create:index = %d, layer type = %d, colorID = %d",
            (int)index, (int)displayLayers[i].type, (int)displayLayers[i].colorID);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"display_create:frame=%d,%d,%d,%d",
            (int)frameVals[0],(int)frameVals[1],(int)frameVals[2],(int)frameVals[3]);
    #endif
    
    switch(displayLayers[i].type){
      case TEXT_LAYER_TYPE:
      {
        int8_t align, bgColorID,fontID,strLen;
        index += resource_load_byte_range(themeRes, index, (uint8_t *)&align,1);
        index += resource_load_byte_range(themeRes, index, (uint8_t *)&bgColorID,1);
        index += resource_load_byte_range(themeRes, index, (uint8_t *)&fontID,1);
        index += resource_load_byte_range(themeRes, index, (uint8_t *)&strLen,1);
        #ifdef DEBUG_DISPLAY
        APP_LOG(APP_LOG_LEVEL_DEBUG,"display_create:alignment = %d, bgColorID = %d, fontID = %d, string length=%d",
            (int)align, (int)bgColorID, (int)fontID,(int)strLen);
        #endif
        //allocate string if not already loaded
        if(displayStrings[textLayerNum] == NULL){
          displayStrings[textLayerNum] = malloc(strLen + 1);
          resource_load_byte_range(themeRes, index, (uint8_t *)displayStrings[textLayerNum],strLen);
          displayStrings[textLayerNum][strLen] = 0;
          #ifdef DEBUG_DISPLAY
          APP_LOG(APP_LOG_LEVEL_DEBUG,"display_create:default string:%s",displayStrings[textLayerNum]);
          #endif
        }
        displayLayers[i].colorID2 = bgColorID;
        displayLayers[i].dataIndex = textLayerNum;
        displayLayers[i].layer = 
          (Layer *)init_text_layer(frame,displayStrings[displayLayers[i].dataIndex],fonts[fontID],align,fontMargins[fontID]);
        textLayerNum++;
        #ifdef DEBUG_DISPLAY
        APP_LOG(APP_LOG_LEVEL_DEBUG,"display_create:created text layer");
        #endif
        break;
      }
      case IMAGE_LAYER_TYPE:
        displayLayers[i].dataIndex = index;
        displayLayers[i].layer = init_image_layer(frame);
        #ifdef DEBUG_DISPLAY
        APP_LOG(APP_LOG_LEVEL_DEBUG,"display_create:created image layer");
        #endif
    }
  }
  layer_set_update_proc(window_get_root_layer(window), background_update_callback);//assign window background color updater
  layer_set_clips(window_get_root_layer(window),true);
  apply_colors();
  window_stack_push(window,true);//display the main window
  initialized = true;
  #ifdef DEBUG_DISPLAY
  APP_LOG(APP_LOG_LEVEL_DEBUG,"display_create:Init succeeded!");
  #endif
}

//shuts down all display functionality
void display_destroy(){
  if(!initialized)return;
  //save display information
  for(int i = 0;i < numDisplayStrings; i++){
    if(displayStrings[i] != NULL)
        persist_write_string(PERSIST_KEY_DISPLAY_STRINGS_BEGIN + i,displayStrings[i]);
  }
  save_colors();
  if(colors != NULL){
    free(colors);
    colors = NULL;
  }
   //Unload strings
  if(displayStrings != NULL){
    for(int i = 0; i < numDisplayStrings; i++){
      if(displayStrings[i]!=NULL)free(displayStrings[i]);
    }
    free(displayStrings);
    displayStrings = NULL;
  }
  numDisplayStrings = 0;
  //Unload Layers
  for(int i=0;i<NUM_LAYERS;i++){
    if(displayLayers[i].layer != NULL){
      if(displayLayers[i].type == TEXT_LAYER_TYPE){
        text_layer_destroy((TextLayer * )displayLayers[i].layer);
      }
      else layer_destroy(displayLayers[i].layer);
      displayLayers[i].layer = NULL;
    }
  }
  if(displayLayers != NULL){
    free(displayLayers);
    displayLayers = NULL;
  }
  //Unload window
  if(window != NULL){
    window_destroy(window);
    window = NULL;
  }
  if(fonts != NULL){
    for(int i = 0; i < NUM_FONTS; i++){
      fonts_unload_custom_font(fonts[i]);
    } 
    free(fonts);
    fonts = NULL;
  }
  initialized = false;
}

//Directly updates display text
void update_display_text(char * newText, LayerID textID){ 
  if(!initialized){
    #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_display_text:Display not initialized!");
    #endif
    return;
  }   
  if(displayLayers[textID].type != TEXT_LAYER_TYPE){
    #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_display_text:Layer is not a text layer!");
    #endif
    return;
  }
  #ifdef DEBUG_DISPLAY
  APP_LOG(APP_LOG_LEVEL_DEBUG,"update_display_text:Setting text on layer %d to %s",textID,newText);
  #endif
  int stringID = displayLayers[textID].dataIndex;
  displayStrings[stringID] = 
        malloc_set_text((TextLayer *) displayLayers[textID].layer, 
                        displayStrings[stringID], 
                        newText);  
  layer_mark_dirty(text_layer_get_layer((TextLayer *) displayLayers[textID].layer));
}

//update one of the color values
void update_color(char colorString[7], ColorID colorID){
  if(!initialized){
    #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_color:Display not initialized!");
    #endif
    return;
  } 
  colors[colorID] = hex_string_to_gcolor(colorString);
  apply_colors();
}

//Get the default frame of a display object
GRect get_default_frame(int layerID){
  if(!initialized){
    #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"get_default_frame:Display not initialized!");
    #endif
    return GRect(0,0,0,0);
  }
  ResHandle themeRes = resource_get_handle(themeResID);
  int metaIndex = 2 + NUM_COLORS*6 + layerID*2;
  int16_t index;
  resource_load_byte_range(themeRes, metaIndex, (uint8_t *)&index,2);
  index++;
  int16_t frameVals[4];
  resource_load_byte_range(themeRes, index, (uint8_t *)frameVals,8);
  return GRect(frameVals[0],frameVals[1],frameVals[2],frameVals[3]);  
}

//Get the bounds of a display layer
GRect get_bounds(int layerID){
  if(!initialized){
    #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"get_bounds:Display not initialized!");
    #endif
    return GRect(0,0,0,0);
  }
  switch(displayLayers[layerID].type){
    case TEXT_LAYER_TYPE:
      return layer_get_bounds(text_layer_get_layer(
        (TextLayer *)(displayLayers[layerID].layer)));
    case IMAGE_LAYER_TYPE:
      return layer_get_bounds(displayLayers[layerID].layer);
    default:
      #ifdef DEBUG_DISPLAY
      APP_LOG(APP_LOG_LEVEL_ERROR,"get_bounds:invalid layer type found!");
      #endif
      return GRect(0,0,0,0);
  }
}

//Get the bounds of a display layer
GRect get_frame(int layerID){
  if(!initialized){
    #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"get_frame:Display not initialized!");
    #endif
    return GRect(0,0,0,0);
  }
  switch(displayLayers[layerID].type){
    case TEXT_LAYER_TYPE:
      return layer_get_frame(text_layer_get_layer(
        (TextLayer *)(displayLayers[layerID].layer)));
    case IMAGE_LAYER_TYPE:
      return layer_get_frame(displayLayers[layerID].layer);
    default:
      #ifdef DEBUG_DISPLAY
      APP_LOG(APP_LOG_LEVEL_ERROR,"get_frame:invalid layer type found!");
      #endif
      return GRect(0,0,0,0);
  }
}

//set the bounds of a display layer
void set_bounds(GRect bounds, LayerID layerID){
  if(!initialized){
    #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"set_bounds:Display not initialized!");
    #endif
    return;
  }
  switch(displayLayers[layerID].type){
    case TEXT_LAYER_TYPE:
      layer_set_bounds(text_layer_get_layer(
        (TextLayer *)(displayLayers[layerID].layer)),bounds);
      break;
    case IMAGE_LAYER_TYPE:
      layer_set_bounds(displayLayers[layerID].layer, bounds);
  }
}

//set the frame of a display layer
void set_frame(GRect frame, LayerID layerID){
  if(!initialized){
    #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"set_bounds:Display not initialized!");
    #endif
    return;
  }
  switch(displayLayers[layerID].type){
    case TEXT_LAYER_TYPE:
      layer_set_frame(text_layer_get_layer(
        (TextLayer *)(displayLayers[layerID].layer)),frame);
      break;
    case IMAGE_LAYER_TYPE:
      layer_set_frame(displayLayers[layerID].layer, frame);
  }
}

  
//----------STATIC FUNCTIONS----------
  
//Load saved display colors
static void load_colors(){
  if(colors == NULL) colors = malloc(sizeof(GColor) * numColors);
  for(int i=0;i<numColors; i++){
    char buf [7];
    if(persist_exists(PERSIST_KEY_COLORS_BEGIN+i+(themeID*NUM_COLORS))){//replace default value with saved value
      persist_read_string(PERSIST_KEY_COLORS_BEGIN+i+(themeID*NUM_COLORS), buf, 7);
      colors[i] = hex_string_to_gcolor(buf);
    
    }else{
      int16_t rgb[3];
      ResHandle themeRes = resource_get_handle(themeResID);
      resource_load_byte_range(themeRes, 1 + (6*i), (uint8_t *)rgb, 6);
      colors[i] =  GColorFromRGB(rgb[0],rgb[1],rgb[2]);
      #ifdef DEBUG_DISPLAY
      APP_LOG(APP_LOG_LEVEL_DEBUG,"load_colors:default color %d: r=%d,g=%d,b=%d",i,
            (int)rgb[0],(int)rgb[1],(int)rgb[2]);
      #endif
    }
  } 
}

//Initialize strings
static void load_text(int16_t layerIndices[NUM_LAYERS]){
  ResHandle themeRes = resource_get_handle(themeResID);
  for(int i = 0; i < NUM_LAYERS; i++){
    int8_t layerType;
    resource_load_byte_range(themeRes,layerIndices[i], (uint8_t *)&layerType, 1);
    if(layerType == TEXT_LAYER_TYPE)numDisplayStrings++;
  }
  //load text
  if(displayStrings == NULL){
    displayStrings = malloc(sizeof(char *) * numDisplayStrings);
  }
  #ifdef DEBUG_DISPLAY
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Allocated pointers for %d display strings",numDisplayStrings);
  #endif
  for(int i = 0;i < numDisplayStrings; i++){
    displayStrings[i] = NULL;
    if(persist_exists(PERSIST_KEY_DISPLAY_STRINGS_BEGIN + i)){
      char buf[64];
      persist_read_string(PERSIST_KEY_DISPLAY_STRINGS_BEGIN + i, buf, sizeof(buf));
      #ifdef DEBUG_DISPLAY
      APP_LOG(APP_LOG_LEVEL_DEBUG,"loaded string %s into strings from key %d",buf, PERSIST_KEY_DISPLAY_STRINGS_BEGIN + i);
      #endif
      displayStrings[i] = malloc_strcpy(displayStrings[i], buf); 
    }
  }
  #ifdef DEBUG_DISPLAY
  APP_LOG(APP_LOG_LEVEL_DEBUG,"All saved strings loaded");
  #endif
}

/**
*Creates a text layer with the given parameters, and adds it to the main window
*@param bounds layer bounding box
*@param text layer initial text
*@param font layer font
*@param align text alignment
*@return the initialized text layer
*/
static TextLayer * init_text_layer(GRect frame,char * text,GFont font,GTextAlignment align,int marginHeight){
  frame.origin.y -= marginHeight;
  frame.size.h += marginHeight;
  TextLayer * textLayer = text_layer_create(frame);
  text_layer_set_font(textLayer,font);
  text_layer_set_text_alignment(textLayer,align);
  text_layer_set_text(textLayer, text);
  layer_add_child(window_get_root_layer(window),text_layer_get_layer(textLayer));
  return textLayer;
}

/**
*Creates an image layer with the given parameters, and adds it to the main window
*@param bitmap holds the layer's newly loaded GBitmap
*@param resourceID the bitmap resource to load
*@param frame layer frame
*@return the initialized bitmap layer
*/
static Layer * init_image_layer(GRect frame){
  Layer * imageLayer = layer_create(frame);
  layer_set_update_proc(imageLayer,image_update_callback);
  layer_set_frame(imageLayer,frame);
  layer_add_child(window_get_root_layer(window),imageLayer);
  return imageLayer;
}

/**
*save color values to persistant storage
*@post all NUM_COLORS colors are saved
*/
static void save_colors(){
  for(int i=0;i<NUM_COLORS;i++){
    if(colors[i].argb!=GColorClearARGB8){
      char buf [7];
      gcolor_to_hex_string(buf, colors[i]);
      persist_write_string(PERSIST_KEY_COLORS_BEGIN+i+(themeID*NUM_COLORS),buf);
    }
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
  if(window != NULL)
    layer_mark_dirty(window_get_root_layer(window));
  #ifdef DEBUG_DISPLAY
  else APP_LOG(APP_LOG_LEVEL_ERROR,"apply_colors: window is null!");
  #endif
  
  //update layer colors
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"apply_colors:Updating layer colors");
  #endif
  for(int i = 0; i < NUM_LAYERS; i++){
    if(displayLayers[i].layer == NULL)
      APP_LOG(APP_LOG_LEVEL_ERROR,"apply_colors: layer is null!");
    else{
      if(displayLayers[i].type == TEXT_LAYER_TYPE){
        text_layer_set_text_color((TextLayer *) displayLayers[i].layer,
                                  colors[displayLayers[i].colorID]);
        text_layer_set_background_color((TextLayer *) displayLayers[i].layer,
                                        colors[displayLayers[i].colorID2]);
      }
      else{
        layer_mark_dirty(displayLayers[i].layer);
      }
    }
  }
}

/**
*Re-draws an image layer
*Called automatically whenever the layer is marked as dirty
*@param layer the image layer
*@param ctx the graphics context
*/
static void image_update_callback(Layer *layer, GContext *ctx){
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"image_update_callback:Drawing image");
  #endif
  for(int i = 0; i < NUM_LAYERS; i++){
    if(layer == displayLayers[i].layer &&
       displayLayers[i].type == IMAGE_LAYER_TYPE){
      graphics_context_set_fill_color(ctx,colors[displayLayers[i].colorID]);
      ResHandle rectRes = resource_get_handle(themeResID);
      int index = displayLayers[i].dataIndex;
      int16_t rectCount;
      index += resource_load_byte_range(rectRes,index,(uint8_t *)&rectCount,2);
      uint8_t rect[4];
      #ifdef DEBUG_DISPLAY
      APP_LOG(APP_LOG_LEVEL_DEBUG,"Found %d rectangles",rectCount);
      #endif
      for(int i = 0; i<rectCount; i++){
        index += resource_load_byte_range(rectRes,index,(uint8_t *)rect,sizeof(int8_t)*4);
        //#ifdef DEBUG_DISPLAY
        //APP_LOG(APP_LOG_LEVEL_DEBUG,"Drawing rectangle:x:%d y:%d w:%d h:%d",rect[0],rect[1],rect[2],rect[3]);
        //#endif
        graphics_fill_rect(ctx,GRect(rect[0],rect[1],rect[2],rect[3]),0,GCornersAll);
      }
    }
  }
}

/**
*Fills the background with the background color
*Called automatically whenever the layer is marked as dirty
*@param layer the main window layer
*@param ctx the graphics context
*/
static void background_update_callback(Layer *layer, GContext *ctx){
  if(layer == NULL){
    #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_ERROR,"background_update_callback:Layer is null!");
    #endif
    return;
  }
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"background_update_callback:setting background color");
    char debug_color [7];
    gcolor_to_hex_string(debug_color,colors[BACKGROUND_COLOR]);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Background color is %s",debug_color);
  #endif
  
  graphics_context_set_fill_color(ctx, colors[BACKGROUND_COLOR]);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornersAll);
  
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"background_update_callback:Background color set");
  #endif
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

