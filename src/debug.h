#pragma once

/**
*Debug method that outputs all values of a dictionary
*@param it a pointer to a DictionaryIterator
*that currently has a dictionary open for reading.
*/
void debugDictionary(DictionaryIterator * it);

/**debug method that prints memory usage along with
*a string for identification purposes
*@param string printed in the debug output
*/
void memDebug(char * string);

void setPreview1();

void setPreview2();