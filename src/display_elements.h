/**
*@File display_elements.h
*defines all application-specific
*display elements
*/

//TextLayer definitions
typedef enum{
  TEXTLAYER_EVENT_1,//second event text layer
  TEXTLAYER_EVENT_0,//first event text layer
  TEXTLAYER_DATE,//current date 
  TEXTLAYER_TIME,//current time
  TEXTLAYER_WEATHERTEXT,//Weather info display
  TEXTLAYER_PEBBLE_BATTERY,//pebble battery percentage
  TEXTLAYER_PHONE_BATTERY,//phone battery percentage
  TEXTLAYER_INFOTEXT,//configurable infotext display
} TextLayerID;
#define NUM_TEXTLAYERS 8

//BitmapLayer definitions
typedef enum{
  BITMAP_LAYER_WEATHER_ICONS,//weather condition icons
  BITMAP_LAYER_FOREGROUND,//first window decoration layer
  BITMAP_LAYER_LINE,//second window decoration layer
  BITMAP_LAYER_DAY_PROGRESS,//day progress bar
  BITMAP_LAYER_EVENT_0_PROGRESS, //first event progress bar
  BITMAP_LAYER_EVENT_1_PROGRESS, //second event progress bar
} BitmapLayerID;
#define NUM_BITMAP_LAYERS 6

//Font resource index
typedef enum{
  FONT_CRUX_16,
  FONT_CRUX_48
} FontIndex;
#define NUM_FONTS 2

//Image resource index
typedef enum{
  PNG_FOREGROUND,
  PNG_LINE,
  PNG_DAY_PROGRESS,
  PNG_EVENT_PROGRESS,
  PNG_WEATHER_ICONS
} ImageIndex;
#define NUM_PNG 5

//Color definitions
typedef enum{
  BACKGROUND_COLOR,
  FOREGROUND_COLOR,
  LINE_COLOR,
  TEXT_COLOR,
  EVENT_0_COLOR,
  EVENT_1_COLOR
} ColorID;
#define NUM_COLORS 6