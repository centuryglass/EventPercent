#include <pebble.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "events.h"
#include "message.h"
#include "util.h"

struct eventStruct{
  char title[MAX_EVENT_LENGTH];
  long start;
  long end;
  char color[7];
};
struct eventStruct events[NUM_EVENTS];

int init = 0;//Equals 1 iff initEvents has been run

//Initializes the eventStruct
//All functions in this class should make sure this one
//has been run before doing anything else
void initEvents(){
  int i;
  for(i = 0; i < NUM_EVENTS; i++){
    strcpy(events[i].title,"");
    strcpy(events[i].color,"000000");
    events[i].start = 0;
    events[i].end = 0;
  }
  init = 1;
}

/**
*Stores an event
*@param numEvent the event slot to set
*@param title the event title
*@param start the event start time
*@param end the event end time
*@param color the event color string
*/
void setEvent(int numEvent,char *title,long start,long end,char* color){
  if(!init)initEvents();
  APP_LOG(APP_LOG_LEVEL_DEBUG,"creating an event with title %s",title);
  strcpy(events[numEvent].title,title);
  
  strcpy(events[numEvent].color,color);
  events[numEvent].start = start;
  events[numEvent].end = end;
}


/**
*Gets one of the stored event titles
*@param numEvent the event number
*@param buffer the buffer where the event string will be stored
*@param bufSize number of bytes allocated to the buffer
*@return buffer if operation succeeds, NULL otherwise
*/
char *getEventTitle(int numEvent,char *buffer,int bufSize){
  if(!init)initEvents();
  if(numEvent >= NUM_EVENTS)return NULL;
  if(bufSize<=(int)strlen(events[numEvent].title))return NULL;
  strcpy(buffer,events[numEvent].title);
  return buffer;
}

/**
*Gets one of the stored events' time info as a formatted string
*Either Days/Hours/Minutes until event, or percent complete
*@param numEvent the event number
*@param buffer the buffer where the time string will be stored
*@param bufSize number of bytes allocated to the buffer
*@return buffer if operation succeeds, NULL otherwise
*/
char *getEventTimeString(int numEvent,char *buffer,int bufSize){
  if(!init)initEvents();//Must be initialized
  if(numEvent >= NUM_EVENTS)return NULL;//Ensure event exists
  if(events[numEvent].start == 0)return NULL;//Ensure event has a start time
  if(strcmp(events[numEvent].title,"") == 0)return NULL;//Ensure event has a title
  time_t now = time(NULL);
  if(events[numEvent].end < now){//Event is over, request new data and return null
    requestEventUpdates();
    return NULL;
  }
  //APP_LOG(APP_LOG_LEVEL_DEBUG,"Time found");
  if(events[numEvent].start <= (long)now){//Get percent complete if event has started
    float eventDuration = events[numEvent].end - events[numEvent].start;
    float timeElapsed = (float)((long)now - events[numEvent].start);
    int percent = (int)(100 * timeElapsed / eventDuration);
    if(ltos((long)percent,buffer,bufSize)==NULL)return NULL;
    if(((int)strlen(buffer)+(int)strlen("% complete"))>bufSize)return NULL;
    strcat(buffer,"% complete");
  }else{//Get time until event if event hasn't started
    long time = events[numEvent].start - now;
    long weeks =0,days=0,hours=0,minutes=0;
    if(time>0)weeks = time / 604800;
    time %= 604800;
    if(time>0)days = time / 86400;
    time %= 86400;
    if(time>0)hours = time / 3600;
    time %= 3600;
    if(time>0)minutes = time / 60;
    char buf[20];
    strcpy(buffer,"");
    if(weeks != 0){
      if(ltos(weeks,buf,20)!=NULL){
        strcat(buffer,buf);
        if(weeks >1) strcat(buffer," Weeks,");
        else strcat(buffer," Week,");
      }
    }
    if(days != 0){
      if(ltos(days,buf,20)!=NULL){
        strcat(buffer,buf);
        if(days > 1)strcat(buffer," Days,");
        else strcat(buffer," Day,");
      }
    }
    if(hours != 0){
      if(ltos(hours,buf,20)!=NULL){
        strcat(buffer,buf);
        if(hours>1)strcat(buffer," Hours,");
        else strcat(buffer," Hour,");
      }
    }
    if(ltos(minutes,buf,20)!=NULL){
        strcat(buffer,buf);
        strcat(buffer," Min.");
    }
    /*DEBUG routine
    struct tm *tick_time = localtime((time_t *)&(events[numEvent].start));
    static char s_buffer[16];
    strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M %d %e" : "%I:%M %d %e", tick_time);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Event %s starts at %s, %s from now",events[numEvent].title,s_buffer,buffer);
    tick_time = localtime((time_t *)&(events[numEvent].end));
    strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M %d %e" : "%I:%M %d %e", tick_time);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Event %s ends at %s",events[numEvent].title,s_buffer);
    */ 
  }
  return buffer;
}

//Requests updated event info from the companion app
void requestEventUpdates(){
  uint8_t buf[DICT_SIZE];
  DictionaryIterator iter;
  //request all events:
  dict_write_begin(&iter,buf,DICT_SIZE);
  dict_write_cstring(&iter, KEY_REQUEST, "Event request");
  dict_write_int32(&iter, KEY_BATTERY_UPDATE, 1);
  dict_write_int32(&iter,KEY_EVENT_NUM,NUM_EVENTS);
  dict_write_end(&iter);
  add_message(buf);
  //APP_LOG(APP_LOG_LEVEL_DEBUG,"Attempting to send message");
  send_message();
}