/**
*@File events.h
*Holds global key definitions
*/

#pragma once

//----------APPMESSAGE KEY DEFINITIONS----------
//Holds a string containing a request/response type
#define KEY_REQUEST 0

/**
*Used by Pebble to request phone battery info, and by Android to
*provide that info as a string
*/
#define KEY_BATTERY_UPDATE 1

//Holds an event's title string, sent from Android
#define KEY_EVENT_TITLE 2

//Holds an event's starting time, sent from Android
#define KEY_EVENT_START 3

//Holds an event's end time, sent from Android
#define KEY_EVENT_END 4

//Holds an event's display color, sent from Android
#define KEY_EVENT_COLOR 5

//Holds the number of events requested by Pebble
#define KEY_EVENT_NUM 6

//Holds the event update frequency in seconds, sent from Android
#define KEY_UPDATE_FREQ 7

//Holds the battery update frequency in seconds, sent from Android
#define KEY_BATT_UPDATE_FREQ 8

/**
*Holds the first color string, sent by Android
*This is the first of a sequence of NUM_COLORS color keys
*NUM_COLORS is defined in display.h
*/
#define KEY_COLORS_BEGIN 20

//----------PERSISTANT STORAGE KEYS----------
//Last event update time, stored as (int)time_t
#define KEY_LASTUPDATE 0

//Event update frequency, stored as int
#define KEY_UPDATEFREQ 1

/**
*The first display color string
*This is the first of a sequence of NUM_COLORS color keys
*NUM_COLORS is defined in display.h
*/
#define KEY_COLORS_BEGIN 20

/**
*the EventStruct data structure from events.c, 
*stored across as many sequential keys as are needed
*/
#define KEY_EVENT_DATA_BEGIN 99