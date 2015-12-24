#pragma once
#define MAX_EVENT_LENGTH 64
#define NUM_EVENTS 2

/**
*Stores an event
*@param numEvent the event slot to set
*@param title the event title
*@param start the event start time
*@param end the event end time
*@param color the event color string
*/
void setEvent(int numEvent,char *title,long start,long end,char* color);

/**
*Gets one of the stored event titles
*@param numEvent the event number
*@param buffer the buffer where the event string will be stored
*@param bufSize number of bytes allocated to the buffer
*@return buffer if operation succeeds, NULL otherwise
*/
char *getEventTitle(int numEvent,char *buffer,int bufSize);

/**
*Gets one of the stored events' time info as a formatted string
*Either Days/Hours/Minutes until event, or percent complete
*@param numEvent the event number
*@param buffer the buffer where the time string will be stored
*@param bufSize number of bytes allocated to the buffer
*@return buffer if operation succeeds, NULL otherwise
*/
char *getEventTimeString(int numEvent,char *buffer,int bufSize);

//Requests updated event info from the companion app
void requestEventUpdates();