/**
*@File display.h
*Handles all functionality related to
*creating, destroying, and manipulating display data
*/

#pragma once
#include <pebble.h>

#define NUM_COLORS 4

/**
*initializes all display functionality
*/
void display_init();

/**
*shuts down all display functionality
*/
void display_deinit();

/**
*Updates display text for events
*/
void update_event_display();

//defines types of display text that can be directly set
typedef enum{
  TEXT_PHONE_BATTERY,//phone battery percentage
  TEXT_PEBBLE_BATTERY,//pebble battery percentage
  TEXT_INFOTEXT//configurable infotext display
} DisplayTextType;

/**
*Directly updates display text
*@param newText the updated text
*@param textType which text layer to set
*/
void update_display_text(char * newText, DisplayTextType textType);

/**
*Sets the displayed time and date
*@param newTime the time data to use to set the time display
*/
void set_time(time_t newTime);

/**
*Updates weather display
*@param degrees the temperature
*@param condition an OpenWeatherAPI condition code
*/
void update_weather(int degrees, int condition);

/**
*change color values to the ones stored in a color array
*@param colorArray an array of NUM_COLOR color strings,
*each 7 bits long
*/
void update_colors(char colorArray[NUM_COLORS][7]);
