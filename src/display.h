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
*/
void update_event_display();
/**
*Updates phone battery display value
*@param battery a cstring containing battery info
*/
void update_phone_battery(char * battery);
/**
*Sets the displayed time and date
*@param newTime the time data to use to set the time display
*/
void set_time(time_t newTime);