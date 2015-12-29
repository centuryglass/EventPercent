/**
*@File events.h
*Handles all functionality related to
*storing, retrieving, and formatting event data
*/

#pragma once
#include <pebble.h>

#define MAX_EVENT_LENGTH 64 //Maximum number of characters allowed in an event title
#define NUM_EVENTS 2  //Number of events stored

/**
*initializes event functionality 
*/
void events_init();

/**
*Shuts down event functionality
*/
void events_deinit();
/**
*Stores an event
*@param numEvent the event slot to set
*@param title the event title
*@param start the event start time
*@param end the event end time
*@param color the event color string
*/
void add_event(int numEvent,char *title,long start,long end,char* color);

/**
*Gets one of the stored event titles
*@param numEvent the event number
*@param buffer the buffer where the event string will be stored
*@param bufSize number of bytes allocated to the buffer
*@return buffer if operation succeeds, NULL otherwise
*/
char *get_event_title(int numEvent,char *buffer,int bufSize);

/**
*Gets one of the stored events' time info as a formatted string
*Either Days/Hours/Minutes until event, or percent complete
*@param numEvent the event number
*@param buffer the buffer where the time string will be stored
*@param bufSize number of bytes allocated to the buffer
*@return buffer if operation succeeds, NULL otherwise
*/
char *get_event_time_string(int numEvent,char *buffer,int bufSize);

/**
*Gets the event update frequency
*@return the update frequency in seconds
*/
int get_event_update_freq();

/**
*Sets the event update frequency
*@param freq the update frequency in seconds
*/
void set_event_update_freq(int freq);

/**
*Gets the last time event data was updated
*@return the last update time
*/
time_t get_last_update_time();