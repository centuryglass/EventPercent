/**
*@File messaging.h
*Handles all functionality related to
*communication between the pebble and phone
*/
#pragma once
#include <pebble.h>

#define DICT_SIZE 128//AppMessage dictionary size

//All valid data update types
typedef enum{
  UPDATE_TYPE_EVENT,
  UPDATE_TYPE_BATTERY,
  UPDATE_TYPE_INFOTEXT,
  UPDATE_TYPE_WEATHER,
  NUM_UPDATE_TYPES = 4
} UpdateType;

/**
*Initializes AppMessage functionality
*/
void messaging_init();

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


