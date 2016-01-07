/**
*@File messaging.h
*Handles all functionality related to
*communication between the pebble and phone
*/
#pragma once
#include <pebble.h>

#define DICT_SIZE 256 //AppMessage dictionary size

/**
*Initializes AppMessage functionality
*/
void messaging_init();

/**
*Shuts down AppMessage functionality
*/
void messaging_deinit();

/**
*Requests updated event info from the companion app
*/
void request_event_updates();

/**
*Requests updated battery info from the companion app
*/
void request_battery_update();
