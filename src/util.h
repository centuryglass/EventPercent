#pragma once
#include <pebble.h>
/**
*@File util.h
*Defines miscellaneous utility functions,
*type conversion functions, debug functions, etc.
*/

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
*Returns the long value of a char string
*@param str the string
*@param strlen length to convert
*@return the represented long's value
*/
long stol(char * str,int strlen);

/**
*Copies a long's value into a string
*@param val the long to copy
*@param str the buffer string to copy into
*@param strlen the length of the buffer
*@return a pointer to str if conversion succeeds, null if it fails 
*/
char *ltos(long val, char *str,int strlen);

/**
*Debug method that outputs all values of a dictionary
*@param it a pointer to a DictionaryIterator
*that currently has a dictionary open for reading.
*/
void debugDictionary(DictionaryIterator * it);

//Tests the ltos Function
void ltosTest();