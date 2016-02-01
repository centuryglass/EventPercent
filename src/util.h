#pragma once
#include <pebble.h>
/**
*@File util.h
*Defines miscellaneous utility functions,
*type conversion functions, debug functions, etc.
*/

/**
*Copies the pebble's remaining battery percentage into a buffer
*@param buffer a buffer of at least 6 bytes
*/
void getPebbleBattery(char * buffer);

/**
*Gets the time since the watchface last launched
*@return uptime in seconds
*/
int getUptime();

/**
*Gets the total uptime since installation
*@return  total uptime in seconds
*/
int getTotalUptime();

/**
*Sets the total uptime from previous watchface instances
*@param uptime in seconds
*/
void setSavedUptime(int uptime);

/**
*Sets the last watchface launch time
*@param launchTime time set on init
*/
void setLaunchTime(time_t launchTime);

/**
*Copies a string to a char pointer, re-allocating memory for the
*destination pointer so it is exactly large enough.
*@param dest the destination pointer
*@param src the source string
*@pre dest is either NULL or was previously passed to this function
*@return dest, or NULL if memory allocation fails
*/
char * malloc_strcpy(char * dest, char * src);

/**
*Allocates and sets a new display string for textlayer, then
*de-allocates the old string
*@param textLayer the layer to update
*@param src the source string
*@param oldString the string textLayer was previously set to
*@pre textLayer was set to a dynamically allocated string
*that can be passed to free()
*@return a pointer to the new string, or null on failure
*/
char * malloc_set_text(TextLayer * textLayer,char * oldString, char * src);

/**
*Returns the long value of a char string
*@param str the string
*@param strlen length to convert
*@return the represented long's value
*/
long stol(char * str,int strlen);


