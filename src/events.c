#include <pebble.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "events.h"
#include "messaging.h"
#include "util.h"
#include "keys.h"

//LOCAL VALUE DEFINITIONS
#define DEBUG 0  //set to 1 to enable event debug logging
#define DEFAULT_UPDATEFREQ 3600  //default event update frequency in seconds

//EVENT DATA STRUCTURE
struct eventStruct{
  char title[MAX_EVENT_LENGTH];
  long start;
  long end;
  char color[7];
};

//STATIC VARIABLES
static struct eventStruct events[NUM_EVENTS];//event data array
static time_t lastEventUpdate;//Last time event data was updated
static int updateFreq;//Seconds to wait between event updates
static int init = 0;//Equals 1 iff events_init has been run

//PUBLIC FUNCTIONS
/**
*initializes event functionality 
*/
void events_init(){
  if(!init){
    int numKeys, i;
    size_t lastKeySize;
    struct eventStruct * index;
    bool readSuccess = true;
  
    //Load last update time
    if(persist_exists(KEY_LASTUPDATE)){
      lastEventUpdate = persist_read_int(KEY_LASTUPDATE);
    }else lastEventUpdate = 0;
    
     //Load update frequency
    if(persist_exists(KEY_UPDATEFREQ)){
      updateFreq = persist_read_int(KEY_UPDATEFREQ);
    }else updateFreq = DEFAULT_UPDATEFREQ;
    
    //load stored event data 
    //Find number of event keys to read
    numKeys =  sizeof(events)/PERSIST_DATA_MAX_LENGTH;
    lastKeySize =  sizeof(events) % PERSIST_DATA_MAX_LENGTH;
    if(lastKeySize > 0)numKeys++;
    else lastKeySize = PERSIST_DATA_MAX_LENGTH;
    if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"Loading event data as %d keys",numKeys+1);
    //Load events from storage
    index = events;
    for(i = 0;i < numKeys;i++){
      int key = KEY_EVENT_DATA_BEGIN+i;
      if(persist_exists(key)){
        size_t keysize, bytesRead;
        if(i == numKeys-1)keysize = lastKeySize;
        else keysize = PERSIST_DATA_MAX_LENGTH;
        bytesRead=persist_read_data(key, index, keysize);
        if(bytesRead != keysize){
          if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"Key %d:Expected to read %d bytes, read %d",i,(int)keysize,(int)bytesRead);
          readSuccess = false;
        }else if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"Key %d read successfully",i);
        index += bytesRead;
      }
      else{//Event key not found in storage, stop loading
        readSuccess = false;
        break;
      }
    }
    if(!readSuccess){
      if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"Failed to find expected key %d, initializing blank event structure",i);
      for(i = 0; i < NUM_EVENTS; i++){
        strcpy(events[i].title,"");
        strcpy(events[i].color,"000000");
        events[i].start = 0;
        events[i].end = 0;
      }
    }
    if(DEBUG){
      for(i = 0; i <NUM_EVENTS;i++){
        APP_LOG(APP_LOG_LEVEL_DEBUG,"Restored event %d, titled %s",i,events[i].title);
      }
    }
  init = 1;  
  }
}
/**
*Shuts down event functionality
*/
void events_deinit(){
  if(init){
    int numKeys, i;
    size_t lastKeySize;
    struct eventStruct * index;
    
    //Save event data struct
    numKeys =  sizeof(events)/PERSIST_DATA_MAX_LENGTH;
    lastKeySize =  sizeof(events) % PERSIST_DATA_MAX_LENGTH;
    if(lastKeySize > 0)numKeys++;
    else lastKeySize = PERSIST_DATA_MAX_LENGTH;
    if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"persist_save_events:Saving event data as %d keys",numKeys+1);
    index = events;
    for(i = 0;i < numKeys;i++){
      size_t keysize;
      if(i == numKeys-1)keysize = lastKeySize;
      else keysize = PERSIST_DATA_MAX_LENGTH;
      size_t bytesWritten =persist_write_data(KEY_EVENT_DATA_BEGIN+i, index, keysize);
      if(bytesWritten != keysize){
        if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,
                       "persist_save_events:Key %d:Expected to write %d bytes, wrote %d",i,(int)keysize,(int)bytesWritten);
      }else if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"persist_save_events:Key %d written successfully",i);
      index += bytesWritten;
    }
    //Save last update time
    persist_write_int(KEY_LASTUPDATE,(int)lastEventUpdate);
    //Save update frequency
    persist_write_int(KEY_UPDATEFREQ,updateFreq);
    init = 0;
  }
}
/**
*Gets the event update frequency
*@return the update frequency in seconds
*/
int get_event_update_freq(){
  if(!init)events_init();
  return updateFreq;
}
/**
*Sets the event update frequency
*@param freq the update frequency in seconds
*/
void set_event_update_freq(int freq){
  if(!init)events_init();
  updateFreq = freq;
}
/**
*Gets the last time event data was updated
*@return the last update time
*/
time_t get_last_update_time(){
  if(!init)events_init();
  return lastEventUpdate;
}
/**
*Stores an event
*@param numEvent the event slot to set
*@param title the event title
*@param start the event start time
*@param end the event end time
*@param color the event color string
*/
void add_event(int numEvent,char *title,long start,long end,char* color){
  if(!init)events_init();
  APP_LOG(APP_LOG_LEVEL_DEBUG,"add_event:creating an event with title %s",title);
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
char *get_event_title(int numEvent,char *buffer,int bufSize){
  if(!init)events_init();
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
char *get_event_time_string(int numEvent,char *buffer,int bufSize){
  if(!init)events_init();
  if(numEvent >= NUM_EVENTS)return NULL;//Ensure event exists
  if(events[numEvent].start == 0)return NULL;//Ensure event has a start time
  if(strcmp(events[numEvent].title,"") == 0)return NULL;//Ensure event has a title
  time_t now = time(NULL);
  if(events[numEvent].end < now){//Event is over, request new data and return null
    request_event_updates();
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
    if(DEBUG){
      struct tm *tick_time = localtime((time_t *)&(events[numEvent].start));
      static char s_buffer[16];
      strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M %d %e" : "%I:%M %d %e", tick_time);
      APP_LOG(APP_LOG_LEVEL_DEBUG,"Event %s starts at %s, %s from now",events[numEvent].title,s_buffer,buffer);
      tick_time = localtime((time_t *)&(events[numEvent].end));
      strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M %d %e" : "%I:%M %d %e", tick_time);
      APP_LOG(APP_LOG_LEVEL_DEBUG,"Event %s ends at %s",events[numEvent].title,s_buffer);
    }
  }
  return buffer;
}








