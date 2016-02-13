/**
*@File display_elements.h
*defines all application-specific
*display elements
*/

//Layer definitions
typedef enum{
  TEXTLAYER_EVENT_1,//second event text layer
  TEXTLAYER_EVENT_0,//first event text layer
  TEXTLAYER_DATE,//current date 
  TEXTLAYER_TIME,//current time
  TEXTLAYER_WEATHERTEXT,//Weather info display
  TEXTLAYER_PEBBLE_BATTERY,//pebble battery percentage
  TEXTLAYER_PHONE_BATTERY,//phone battery percentage
  TEXTLAYER_INFOTEXT,//configurable infotext display
  IMAGE_LAYER_WEATHER_ICONS,//weather condition icons
  IMAGE_LAYER_FOREGROUND,//first window decoration layer
  IMAGE_LAYER_LINE,//second window decoration layer
  IMAGE_LAYER_DAY_PROGRESS,//day progress bar
  IMAGE_LAYER_EVENT_0_PROGRESS, //first event progress bar
  IMAGE_LAYER_EVENT_1_PROGRESS, //second event progress bar
}LayerID;
#define NUM_LAYERS 14

//Font resource index
typedef enum{
  FONT_SMALL,
  FONT_LARGE
} FontIndex;
#define NUM_FONTS 2

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

