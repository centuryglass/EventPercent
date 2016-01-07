#pragma once
#include <pebble.h>
/**
*@File util.h
*Defines miscellaneous utility functions,
*type conversion functions, debug functions, etc.
*/

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