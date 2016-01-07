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

/**
*Updates phone battery display value
*@param battery a cstring containing phone battery info
*/
void update_phone_battery(char * battery);

/**
*Updates pebble battery display value
*@param battery a cstring containing pebble battery info
*/
void update_watch_battery(char * battery);

/**
*Sets the displayed time and date
*@param newTime the time data to use to set the time display
*/
void set_time(time_t newTime);

/**
*change color values to the ones stored in a color array
*@param colorArray an array of NUM_COLOR color strings,
*each 7 bits long
*/
void update_colors(char colorArray[NUM_COLORS][7]);