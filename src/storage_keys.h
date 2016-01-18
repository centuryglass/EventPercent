/**
*@File keys.h
*Holds storage key definitions
*/

#pragma once

//----------PERSISTANT STORAGE KEYS---------
enum{
  //Display string keys
  PERSIST_KEY_INFOTEXT,
    //string: infoText data
  PERSIST_KEY_PHONE_BATTERY,
    //string: phone battery display string
  PERSIST_KEY_WEATHER,
    //string: weather temperature display
  PERSIST_KEY_STRINGS_END,
    //marks the end of the display string keys
  
  PERSIST_KEY_WEATHER_COND,
    //int: weather condition code
  PERSIST_KEY_UPTIME,
    //int: total watchface uptime in seconds
  PERSIST_KEY_UPDATE_FREQS_BEGIN = 20,
    //int: First update frequency(seconds)
    //This begins a series of keys holding update frequencies for all update types
    //Update types are defined in order in messaging.h
  PERSIST_KEY_LAST_UPDATE_TIMES_BEGIN = 30,
    //int: time_t first saved latest update time
    //This begins a series of keys holding latest update time for all update types 
    //Update types are defined in order in messaging.h
  PERSIST_KEY_COLORS_BEGIN = 50,
    //string: The first display color string
    //This is the first of a sequence of NUM_COLORS color keys
    //NUM_COLORS is defined in display.h
  PERSIST_KEY_EVENT_DATA_BEGIN = 99
    //data: EventStruct data structure from events.c,
    //saved across as many sequential keys as needed
};
