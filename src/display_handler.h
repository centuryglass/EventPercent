/**
*@File display_handler.h
*Handles application-specific usage of display fields
*/

#pragma once
#include <pebble.h>

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
*@param eventNum event index
*@param event_title event display title to set
*@param event_time event time string to set
*@param eventPercent what percent of the event has already passed
*@param event_color the event's display color
*/
void update_event_display(int eventNum, char * event_title, 
                          char * event_time, int eventPercent, char * event_color);

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
*@param colorArray an array of NUM_COLORS color strings,
*each 7 bits long. An empty string can be used to indicate
*that a color won't be changed
*/
void update_colors(char colorArray[][7]);

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
void update_text(char * newText, DisplayTextType textType);

/**
*set the strftime date format
*@param format the new strftime format string
*/
void set_date_format(char * format);
