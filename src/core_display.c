#include <pebble.h>
#include "core_display.h"
#include "storage_keys.h"
#include "util.h"


//-----LOCAL VALUE DEFINITIONS-----
//#define DEBUG_DISPLAY
//TextLayer specification struct
typedef struct tModel{
  char defaultText [24];
  GRect bounds;
  GTextAlignment alignment;
  ColorID textColorID;
  ColorID backgroundColorID;
  FontIndex fontID;
} TextModel;

//BitmapLayer specification struct
typedef struct iModel{
  GRect bounds;
  ColorID colorID;
  ImageIndex imageResource;
} ImageModel;
//----------LOCAL VARIABLES----------
static GFont fonts[NUM_FONTS];
static GColor colors[NUM_COLORS] = {{GColorClearARGB8}};
Window * window = NULL;
static bool initialized = false;

BitmapLayer * bitmapLayers[NUM_BITMAP_LAYERS] = {NULL};
static GBitmap * bitmaps[NUM_BITMAP_LAYERS] = {NULL};
static int bitmapColorIDs [NUM_BITMAP_LAYERS] = {0};

static TextLayer * textLayers[NUM_TEXTLAYERS] = {NULL};
static char * displayStrings[NUM_TEXTLAYERS] = {NULL};
static int textColorIDs[NUM_TEXTLAYERS] = {0};
static int textBackgroundColorIDs[NUM_TEXTLAYERS] = {0};


//----------STATIC FUNCTION DECLARATIONS----------
//Initialization functions:
static void init_colors();
  //Initialize display colors
static void init_text();
  //Initialize all TextLayers
static void init_images();
  //Initialize all BitmapLayers
static TextLayer * init_text_layer(GRect bounds,char * text,GFont font,GTextAlignment align);
  //Creates a text layer with the given parameters, and adds it to the main window
static BitmapLayer * init_bitmap_layer(GBitmap ** bitmap, int resourceID,GRect bounds);
  //Creates a bitmap layer with the given parameters, and adds it to the main window

//Resource functions:
uint32_t get_bitmap_id(ImageIndex index);
  //Get the resource ID of an image from its index
uint32_t get_font_id(FontIndex index);
  //Get the resource ID of a font from its index

//Color functions:
static void save_colors();
  //save color values to persistant storage
static void apply_colors();
  //Makes sure all initialized bitmaps are set to the correct colors
static void apply_bitmap_color(GColor color,BitmapLayer *layer);
  //Applies a color to a single bitmap layer
static void background_update_callback(Layer *layer, GContext *ctx);
  //Fills the background with the background color
static void gcolor_to_hex_string(char outstring[7], GColor color);
  //Given a GColor, copy its hex value into a buffer string
static GColor hex_string_to_gcolor(char * string);
  //Given a color hex string, return a corresponding GColor

//----------PUBLIC FUNCTIONS----------
//initializes all display functionality
void display_create(){
   //Window init
  if(window == NULL) window = window_create();
  layer_set_clips(window_get_root_layer(window),true);
  //Font init
  for(int i = 0; i < NUM_FONTS; i++){
    fonts[i] = fonts_load_custom_font(resource_get_handle(get_font_id(i)));
  } 
  init_colors();
  init_text();
  init_images();
  layer_set_update_proc(window_get_root_layer(window), background_update_callback);//assign window background color updater
  apply_colors();
  window_stack_push(window,true);//display the main window
  initialized = true;
}

//shuts down all display functionality
void display_destroy(){
  //save display information
  for(int i = PERSIST_KEY_DISPLAY_STRINGS_BEGIN;
      i < PERSIST_KEY_DISPLAY_STRINGS_BEGIN + NUM_TEXTLAYERS; i++){
    if(displayStrings[i - PERSIST_KEY_DISPLAY_STRINGS_BEGIN] != NULL)
        persist_write_string(i,displayStrings[i - PERSIST_KEY_DISPLAY_STRINGS_BEGIN]);
  }
  save_colors();
   //Unload text layers and strings
  for(int i=0;i<NUM_TEXTLAYERS;i++){
    if(textLayers[i]!=NULL){
      text_layer_destroy(textLayers[i]);
      textLayers[i] = NULL;
    }
    if(displayStrings[i] != NULL){
      free(displayStrings[i]);
      displayStrings[i] = NULL;
    }
  }
  //destroy bitmap layers and gbitmaps
  for(int i=0;i<NUM_BITMAP_LAYERS;i++){
    if (bitmapLayers[i] != NULL) bitmap_layer_destroy(bitmapLayers[i]);
    if (bitmaps[i] != NULL) gbitmap_destroy(bitmaps[i]);
    bitmapLayers[i] = NULL;
    bitmaps[i] = NULL;
  }
  //Unload window
  window_destroy(window);
  initialized = false;
}

//Directly updates display text
void update_display_text(char * newText, TextLayerID textID){  
  if(!initialized){
    #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"update_display_text:Display not initialized! creating display");
    #endif
    display_create();
  }    
  #ifdef DEBUG_DISPLAY
  APP_LOG(APP_LOG_LEVEL_DEBUG,"update_display_text:Setting text on layer %d to %s",textID,newText);
  #endif
  displayStrings[textID] = 
        malloc_set_text(textLayers[textID], 
                        displayStrings[textID], 
                        newText);  
  layer_mark_dirty(text_layer_get_layer(textLayers[textID]));
}

//update one of the color values
void update_color(char colorString[7], ColorID colorID){
  colors[colorID] = hex_string_to_gcolor(colorString);
  apply_colors();
}

//Sets the default color values before initialization
void set_default_colors(GColor defaultColors[NUM_COLORS]){
  if(initialized){
    APP_LOG(APP_LOG_LEVEL_ERROR,"set_default_colors:attempting to set colors after initialization, don't do that.");
    return;
  }
  for(int i = 0; i < NUM_COLORS; i++){
    colors[i] = defaultColors[i];
  }
}

//Get the bounds of a display object
GRect get_bounds(DisplayType type,int layerID){
  switch(type){
    case BITMAP_LAYER_TYPE:
      return gbitmap_get_bounds(bitmaps[layerID]);
    case TEXTLAYER_TYPE:
      return layer_get_bounds(text_layer_get_layer(textLayers[layerID]));
    case GBITMAP_TYPE:
      return layer_get_bounds(bitmap_layer_get_layer(bitmapLayers[layerID]));
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR,"get_bounds:Invalid displayType!");
      return GRect(0,0,0,0);
  }
}

//set the bounds of a display object
void set_bounds(GRect bounds, DisplayType type,int layerID){
  switch(type){
    case BITMAP_LAYER_TYPE:
      layer_set_bounds(bitmap_layer_get_layer(bitmapLayers[layerID]), bounds);
      break;
    case TEXTLAYER_TYPE:
      layer_set_bounds(text_layer_get_layer(textLayers[layerID]),bounds);
      break;
    case GBITMAP_TYPE:
      gbitmap_set_bounds(bitmaps[layerID], bounds);
  }
  layer_mark_dirty(window_get_root_layer(window));
}
  
//----------STATIC FUNCTIONS----------
  
//Initialize display colors
static void init_colors(){
  for(int i=0;i<NUM_COLORS;i++){
    char buf [7];
    if(persist_exists(PERSIST_KEY_COLORS_BEGIN+i)){//replace default value with saved value
      persist_read_string(PERSIST_KEY_COLORS_BEGIN+i, buf, 7);
      colors[i] = hex_string_to_gcolor(buf);
    }
  } 
}

//Initialize all TextLayers
static void init_text(){
  //Load layer specification resource
  #ifdef DEBUG_DISPLAY
  APP_LOG(APP_LOG_LEVEL_DEBUG,"init_text: Loading resource layouts");
  #endif
  ResHandle textModelResource = resource_get_handle(RESOURCE_ID_TEXT_MODELS);
  
  //check resource size for correctness
  if(resource_size(textModelResource) != sizeof(TextModel)*NUM_TEXTLAYERS)
    APP_LOG(APP_LOG_LEVEL_ERROR, 
            "display_create: Error, text layout resource was %d bytes, expected %d",
            (int) resource_size(textModelResource),(int) (sizeof(TextModel)*NUM_TEXTLAYERS));
  
  //initialize text layers
  for(int i = 0; i < NUM_TEXTLAYERS; i++){
    TextModel model;
    int bytesCopied = 
      resource_load_byte_range(textModelResource, sizeof(TextModel)*i, (uint8_t *)&model, sizeof(TextModel));
    if(bytesCopied != sizeof(TextModel))
      APP_LOG(APP_LOG_LEVEL_ERROR, "display_create: Failed loading TextModel, loaded %d bytes of %d",
              bytesCopied,(int) sizeof(TextModel));
    //load text
    if(displayStrings[i] == NULL){
      if(persist_exists(PERSIST_KEY_DISPLAY_STRINGS_BEGIN + i)){
        char buf[64];
        persist_read_string(PERSIST_KEY_DISPLAY_STRINGS_BEGIN + i, buf, sizeof(buf));
        displayStrings[i] = malloc_strcpy(displayStrings[i], buf);
      }
      else displayStrings[i] = malloc_strcpy(displayStrings[i], model.defaultText);
    }
    //create text layer
    if(textLayers[i] == NULL)
      textLayers[i] = init_text_layer(model.bounds,
                                      displayStrings[i],
                                      fonts[model.fontID],
                                      model.alignment);
    else APP_LOG(APP_LOG_LEVEL_ERROR,"Text layer %d wasn't null!",i);
    #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"init_text:text layer:%d bounds: x:%d y:%d w:%d h:%d",i,
            model.bounds.origin.x,model.bounds.origin.y,model.bounds.size.w,model.bounds.size.h);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"   default string:%s font resource index:%d",
            model.defaultText,model.fontID);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"   text color id:%d background color id:%d",
            model.textColorID,model.backgroundColorID);
    #endif
    //save color IDs
    textColorIDs[i] = model.textColorID;
    textBackgroundColorIDs[i] = model.backgroundColorID;
     #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"init_text:text layer %d created, memory use is %d",i,(int)heap_bytes_used());
    #endif
  }
  
}

//Initialize all BitmapLayers
static void init_images(){
  //Load layer specification resource
  #ifdef DEBUG_DISPLAY
  APP_LOG(APP_LOG_LEVEL_DEBUG,"init_images: Loading resource layouts");
  #endif
  ResHandle imageModelResource = resource_get_handle(RESOURCE_ID_IMAGE_MODELS);
  
  //check resource size for correctness
  if(resource_size(imageModelResource) != sizeof(ImageModel) * NUM_BITMAP_LAYERS)
    APP_LOG(APP_LOG_LEVEL_ERROR, 
            "init_images: Error, image layout resource was %d bytes, expected %d",
            (int) resource_size(imageModelResource),(int) (sizeof(ImageModel)*NUM_BITMAP_LAYERS));
  //initialize image layers
  for(int i = 0; i < NUM_BITMAP_LAYERS; i++){
    ImageModel model;
    int bytesCopied = 
      resource_load_byte_range(imageModelResource, sizeof(ImageModel)*i, (uint8_t *)&model, sizeof(ImageModel));
    if(bytesCopied != sizeof(ImageModel))
      APP_LOG(APP_LOG_LEVEL_ERROR, "init_images: Failed loading ImageModel, loaded %d bytes of %d",
              bytesCopied,(int) sizeof(ImageModel));
    if(bitmapLayers[i] == NULL)
      bitmapLayers[i] = init_bitmap_layer(&bitmaps[i],get_bitmap_id(model.imageResource),model.bounds);
    bitmapColorIDs[i] = model.colorID;
    #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"init_images:image %d bounds: x:%d y:%d w:%d h:%d",i,
            model.bounds.origin.x,model.bounds.origin.y,model.bounds.size.w,model.bounds.size.h);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"init_images:color id:%d image resource index:%d",
            model.colorID,model.imageResource);
    #endif
  }
  #ifdef DEBUG_DISPLAY
  APP_LOG(APP_LOG_LEVEL_DEBUG,"init_images: image init complete");
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
static TextLayer * init_text_layer(GRect bounds,char * text,GFont font,GTextAlignment align){
  TextLayer * textLayer = text_layer_create(bounds);
  text_layer_set_font(textLayer,font);
  text_layer_set_text_alignment(textLayer,align);
  text_layer_set_text(textLayer, text);
  layer_add_child(window_get_root_layer(window),text_layer_get_layer(textLayer));
  return textLayer;
}

/**
*Creates a bitmap layer with the given parameters, and adds it to the main window
*@param bitmap holds the layer's newly loaded GBitmap
*@param resourceID the bitmap resource to load
*@param bounds layer bounding box
*@return the initialized bitmap layer
*/
static BitmapLayer * init_bitmap_layer(GBitmap ** bitmap, int resourceID,GRect bounds){
  if(*bitmap != NULL){
    APP_LOG(APP_LOG_LEVEL_ERROR,"init_bitmap_layer:error, target GBitmap pointer is not null!");
    return NULL;
  }
  BitmapLayer * bitmapLayer = bitmap_layer_create(bounds);
  *bitmap = gbitmap_create_with_resource(resourceID);
  bitmap_layer_set_bitmap(bitmapLayer,*bitmap);
  bitmap_layer_set_compositing_mode(bitmapLayer, GCompOpSet);
  layer_add_child(window_get_root_layer(window),bitmap_layer_get_layer(bitmapLayer));
  return bitmapLayer;
}

//Get the resource ID of an image from its index
uint32_t get_bitmap_id(ImageIndex index){
  switch(index){
    case PNG_FOREGROUND:
      return RESOURCE_ID_BG1;
    case PNG_LINE:
      return RESOURCE_ID_BG2;
    case PNG_DAY_PROGRESS:
      return RESOURCE_ID_DAY_PROGRESS;
    case PNG_EVENT_PROGRESS:
      return RESOURCE_ID_PROGRESS;
    case PNG_WEATHER_ICONS:
      return RESOURCE_ID_WEATHER_ICONS;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR,"get_bitmap_id: invalid index!");
      return -1;
  }
}
//Get the resource ID of a font from its index
uint32_t get_font_id(FontIndex index){
  switch(index){
    case FONT_CRUX_16:
      return RESOURCE_ID_CODERS_CRUX_16;
    case FONT_CRUX_48:
      return RESOURCE_ID_CODERS_CRUX_48;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR,"get_font_id: invalid index!");
      return -1;
  }
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
      persist_write_string(PERSIST_KEY_COLORS_BEGIN+i,buf);
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
  else APP_LOG(APP_LOG_LEVEL_ERROR,"apply_colors: window is null!");
  //update bitmap layer colors
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"apply_colors:Updating bitmap layer colors");
  #endif
  for(int i = 0; i < NUM_BITMAP_LAYERS; i++){
    if(bitmapLayers[i] == NULL)
      APP_LOG(APP_LOG_LEVEL_ERROR,"apply_colors: bitmap layer is null!");
    else apply_bitmap_color(colors[bitmapColorIDs[i]], bitmapLayers[i]);
  }
  #ifdef DEBUG_DISPLAY
    APP_LOG(APP_LOG_LEVEL_DEBUG,"apply_colors:Updating text layer colors");
  #endif
  //update text layer colors
  for(int i = 0; i < NUM_TEXTLAYERS; i++){
    if(textLayers[i] == NULL)
      APP_LOG(APP_LOG_LEVEL_ERROR,"apply_colors: text layer %d is null!",i);
    else{
      text_layer_set_text_color(textLayers[i], colors[textColorIDs[i]]);
      text_layer_set_background_color(textLayers[i], colors[textBackgroundColorIDs[i]]);
    }
  }
}


/**
*applies a color to a single bitmap layer
*@param color the new color to set
*@param layer an instantiated BitmapLayer set to a 2-bit palletized Gbitmap
*@post all colors in layer besides GColor are set to (color)
*/
static void apply_bitmap_color(GColor color,BitmapLayer *layer){
  if(layer == NULL){
    APP_LOG(APP_LOG_LEVEL_ERROR,"apply_bitmap_color:BitmapLayer is null!");
    return;
  }
  //load the bitmap's palette (must be 2 bit palletized)
  GColor * palette = gbitmap_get_palette(bitmap_layer_get_bitmap(layer));
  if(palette == NULL)APP_LOG(APP_LOG_LEVEL_ERROR,"apply_bitmap_color:Image palette is null!");
  //Replace all palette colors except GColorClear
  if(!gcolor_equal(palette[0], GColorClear))
    palette[0].argb = color.argb;
  if(!gcolor_equal(palette[1], GColorClear))
    palette[1].argb = color.argb;
  layer_mark_dirty(bitmap_layer_get_layer(layer));
}

/**
*Fills the background with the background color
*Called automatically whenever the layer is marked as dirty
*@param layer the main window layer
*@param ctx the graphics context
*/
static void background_update_callback(Layer *layer, GContext *ctx){
  if(layer == NULL){
    APP_LOG(APP_LOG_LEVEL_ERROR,"background_update_callback:Layer is null!");
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