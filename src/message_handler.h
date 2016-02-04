/**
*@File message_handler.h
*Handles application-specific messaging
*functionality
*/
#pragma once
#include <pebble.h>



//All valid data update types
typedef enum{
  UPDATE_TYPE_EVENT,
  UPDATE_TYPE_BATTERY,
  UPDATE_TYPE_INFOTEXT,
  UPDATE_TYPE_WEATHER,
  UPDATE_TYPE_PEBBLE_STATS,
  NUM_UPDATE_TYPES = 5
} UpdateType;

/**
*Initializes AppMessage functionality
*/
void message_handler_init();

/**
*Shuts down AppMessage functionality
*/
void messaging_deinit();

/**
*Requests updated info from the companion app
*@param updateType the appropriate update request code
*/
void request_update(UpdateType updateType);

/**
*Gets the update frequency for a given request
*@param updateType the appropriate update request code
*@return update frequency in seconds
*/
int get_update_frequency(UpdateType updateType);

/**
*Gets the last time a given update was received
*@param UpdateType the appropriate update request code
*@return update time
*/
time_t get_update_time(UpdateType updateType);


