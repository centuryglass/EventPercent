/**
*@File events.h
*Holds global key definitions
*/

#pragma once

//----------APPMESSAGE KEY DEFINITIONS----------
/**
*Holds a string containing a request/response type
*Valid request strings:
* "Event request"
* "Battery request"
* "InfoText request"
* "Weather request"
* "Color update"
* "Event sent"
* "Weather sent"
* "Watch info request"
*/
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
#define KEY_EVENT_UPDATE_FREQ 7

//Holds the battery update frequency in seconds, sent from Android
#define KEY_BATT_UPDATE_FREQ 8

//Holds the infotext update frequency in seconds, sent from Android
#define KEY_INFOTEXT_UPDATE_FREQ 9

//Holds the weather update frequency in seconds, sent from Android
#define KEY_WEATHER_UPDATE_FREQ 10

//Holds a configurable information string, sent from Android
#define KEY_INFOTEXT 11

//Holds the current watchface uptime in seconds, sent from Pebble
#define KEY_UPTIME 12

//Holds the total watchface uptime in seconds, sent from Pebble
#define KEY_TOTAL_UPTIME 13

//Holds the pebble model as an enum WatchInfoModel int, sent from pebble
#define KEY_PEBBLE_MODEL 14

//Holds the pebble color as an enum WatchInfoColor int, sent from pebble
#define KEY_PEBBLE_COLOR 15

//Holds whether the pebble is set to 12 or 24 hour mode(as int=12 or int=24), sent from pebble
#define KEY_12_OR_24_MODE 16

//Holds a temperature int, sent by Android
#define KEY_TEMP 17

//Holds an OpenWeatherAPI condition code, sent by Android
#define KEY_WEATHER_COND 18

//Holds the amount of memory used by the watchapp, sent from Pebble
#define KEY_MEMORY_USED 19

//Holds the amount of memory available for watchapp, sent from Pebble
#define KEY_MEMORY_FREE 20

/**
*Holds the first color string, sent by Android
*This is the first of a sequence of NUM_COLORS color keys
*NUM_COLORS is defined in display.h
*/
#define KEY_COLORS_BEGIN 50

//----------PERSISTANT STORAGE KEYS----------

//Event update frequency(seconds), stored as int
#define PERSIST_KEY_EVENT_UPDATE_FREQ 0

//Last event update request time, stored as (int)time_t
#define PERSIST_KEY_LAST_EVENT_REQUEST 1

//Batterry update frequency(seconds), stored as int
#define PERSIST_KEY_BATTERY_UPDATE_FREQ 2

//Last battery update request time, stored as (int)time_t
#define PERSIST_KEY_LAST_BATTERY_REQUEST 3

//Information text update frequency(seconds), stored as int
#define PERSIST_KEY_INFOTEXT_UPDATE_FREQ 4

//Last info text update request time, stored as (int)time_t
#define PERSIST_KEY_LAST_INFOTEXT_REQUEST 5

//Weather update frequency(seconds), stored as int
#define PERSIST_KEY_WEATHER_UPDATE_FREQ 6

//Last weather update request time, stored as (int)time_t
#define PERSIST_KEY_LAST_WEATHER_REQUEST 7

//Info text string
#define PERSIST_KEY_INFOTEXT 8

//Total watchface uptime(seconds), stored as (int)time_t
#define PERSIST_KEY_UPTIME 9

//Last saved phone battery string
#define PERSIST_KEY_PHONE_BATTERY 10

//Last saved weather string
#define PERSIST_KEY_WEATHER 11

//Last saved weather condition code
#define PERSIST_KEY_WEATHER_COND 12



/**
*The first display color string
*This is the first of a sequence of NUM_COLORS color keys
*NUM_COLORS is defined in display.h
*/
#define PERSIST_KEY_COLORS_BEGIN 20

/**
*the EventStruct data structure from events.c, 
*stored across as many sequential keys as are needed
*/
#define KEY_EVENT_DATA_BEGIN 99