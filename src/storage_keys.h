/**
*@File keys.h
*Holds storage key definitions
*/

#pragma once

//----------PERSISTANT STORAGE KEYS---------
enum{
  PERSIST_KEY_WEATHER_COND,//int: weather condition code
  PERSIST_KEY_UPTIME, //int: total watchface uptime in seconds
  PERSIST_KEY_DATE_FORMAT, //string: date format for strftime
  PERSIST_KEY_FUTURE_EVENT_FORMAT,//int: FutureEventFormat value
  PERSIST_KEY_COMPANION_APP_CONTACTED,//int: 1 if the companion app has been found
  PERSIST_KEY_THEME,//int: display theme choice
  
  /**
  *string: first display string
  *beginning of a sequence of strings holding last saved
  *display values, in the order defined in display_elements.h
  */
  PERSIST_KEY_DISPLAY_STRINGS_BEGIN = 40,
  
  /**
  *int: First update frequency(seconds)
  *This begins a series of keys holding update frequencies for all update types
  *Update types are defined in order in messaging.h
  */
  PERSIST_KEY_UPDATE_FREQS_BEGIN = 50,
    
  /**
  *int: time_t first saved latest update time
  *This begins a series of keys holding latest update time for all update types 
  *Update types are defined in order in messaging.h
  */
  PERSIST_KEY_LAST_UPDATE_TIMES_BEGIN = 60,
    
  /**
  *string: The first display color string
  *This is the start of the first sequence of NUM_COLORS color keys
  *where each theme has its own color keys
  *NUM_COLORS is defined in display_elements.h
  *
  */
  PERSIST_KEY_COLORS_BEGIN = 70,
    
  /**
  *data: EventStruct data structure from events.c,
  *saved across as many sequential keys as needed
  */
  PERSIST_KEY_EVENT_DATA_BEGIN = 200
    
};
