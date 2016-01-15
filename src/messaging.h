/**
*@File messaging.h
*Handles all functionality related to
*communication between the pebble and phone
*/
#pragma once
#include <pebble.h>

#define DICT_SIZE 256 //AppMessage dictionary size

enum REQUEST{//valid request types
  event_request,
  battery_request,
  infotext_request,
  weather_request,
  color_update,
  event_sent,
  weather_sent,
  watch_info_request
};

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
*@param requestType a valid request type 
*/
void request_update(enum REQUEST requestType);

/**
*Gets the update frequency for a given request
*@param requestType a request type
*@return update frequency in seconds
*/
int get_update_frequency(enum REQUEST requestType);

/**
*Gets the last time a given request was made
*@param requestType a request type
*@return update time
*/
time_t get_request_time(enum REQUEST requestType);


