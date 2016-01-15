#include <pebble.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "events.h"
#include "messaging.h"
#include "util.h"
#include "keys.h"

//----------LOCAL VALUE DEFINITIONS----------
//#define DEBUG_EVENTS  //uncomment to enable event debug logging

//----------EVENT DATA STRUCTURE----------
struct eventStruct{
  char title[MAX_EVENT_LENGTH];//event title
  long start;//event start time (seconds)
  long end;//event end time (seconds)
  char color[7];//event display color
};

//----------STATIC VARIABLES----------
static struct eventStruct events[NUM_EVENTS];//event data array
static int init = 0;//Equals 1 iff events_init has been run

//----------PUBLIC FUNCTIONS----------
//initializes event functionality 
void events_init(){
  if(!init){
    int numKeys, i;
    size_t lastKeySize;
    struct eventStruct * index;
    bool readSuccess = true;
    
    //Load stored event data 
    //Find number of event keys to read
    numKeys =  sizeof(events)/PERSIST_DATA_MAX_LENGTH;
    lastKeySize =  sizeof(events) % PERSIST_DATA_MAX_LENGTH;
    if(lastKeySize > 0)numKeys++;
    else lastKeySize = PERSIST_DATA_MAX_LENGTH;
    #ifdef DEBUG_EVENTS
      APP_LOG(APP_LOG_LEVEL_DEBUG,"Loading event data as %d keys",numKeys+1);
    #endif
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
          #ifdef DEBUG_EVENTS
            APP_LOG(APP_LOG_LEVEL_DEBUG,"Key %d:Expected to read %d bytes, read %d",i,(int)keysize,(int)bytesRead);
          #endif
          readSuccess = false;
        }
        #ifdef DEBUG_EVENTS
          else APP_LOG(APP_LOG_LEVEL_DEBUG,"Key %d read successfully",i);
        #endif
        index += bytesRead;
      }
      else{//Event key not found in storage, stop loading
        readSuccess = false;
        break;
      }
    }
    if(!readSuccess){
      #ifdef DEBUG_EVENTS
        APP_LOG(APP_LOG_LEVEL_DEBUG,"Failed to find expected key %d, initializing blank event structure",i);
      #endif
      for(i = 0; i < NUM_EVENTS; i++){
        strcpy(events[i].title,"");
        strcpy(events[i].color,"000000");
        events[i].start = 0;
        events[i].end = 0;
      }
    }
    #ifdef DEBUG_EVENTS
      for(i = 0; i <NUM_EVENTS;i++){
        APP_LOG(APP_LOG_LEVEL_DEBUG,"Restored event %d, titled %s",i,events[i].title);
      }
    #endif
  init = 1;  
  }
}

//Shuts down event functionality
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
    #ifdef DEBUG_EVENTS
      APP_LOG(APP_LOG_LEVEL_DEBUG,"persist_save_events:Saving event data as %d keys",numKeys+1);
    #endif
    index = events;
    for(i = 0;i < numKeys;i++){
      size_t keysize;
      if(i == numKeys-1)keysize = lastKeySize;
      else keysize = PERSIST_DATA_MAX_LENGTH;
      size_t bytesWritten =persist_write_data(KEY_EVENT_DATA_BEGIN+i, index, keysize);
      
      if(bytesWritten != keysize){
        APP_LOG(APP_LOG_LEVEL_ERROR,
                       "persist_save_events:Key %d:Expected to write %d bytes, wrote %d",i,(int)keysize,(int)bytesWritten);
      }
      #ifdef DEBUG_EVENTS 
        else APP_LOG(APP_LOG_LEVEL_DEBUG,"persist_save_events:Key %d written successfully",i);
      #endif
      index += bytesWritten;
    }
    init = 0;
  }
}

//Stores an event
void add_event(int numEvent,char *title,long start,long end,char* color){
  if(!init)events_init();
  if(numEvent >= NUM_EVENTS){
    APP_LOG(APP_LOG_LEVEL_ERROR,"Event %s is out of bounds at index %d",title,numEvent);
    return;
  }
  #ifdef DEBUG_EVENTS 
    APP_LOG(APP_LOG_LEVEL_DEBUG,"add_event:creating an event with title %s",title);
  #endif
  strcpy(events[numEvent].title,title);
  strcpy(events[numEvent].color,color);
  events[numEvent].start = start;
  events[numEvent].end = end;
}

//Gets one of the stored event titles
char *get_event_title(int numEvent,char *buffer,int bufSize){
  if(!init)events_init();
  if(numEvent >= NUM_EVENTS)return NULL;//Check if event is within bounds
  if(strcmp(events[numEvent].title,"")==0)return NULL;//Check if event exists
  if(bufSize<=(int)strlen(events[numEvent].title))return NULL;
  strcpy(buffer,events[numEvent].title);
  return buffer;
}

//Gets the percent completed of an event
int get_percent_complete(int numEvent){
  if(!init)events_init();
  if(numEvent >= NUM_EVENTS)return -1;//Check if event is within bounds
  if(strcmp(events[numEvent].title,"")==0)return -1;//Check if event exists
  if(events[numEvent].start == 0)return -1;//Check if event has a start time
  time_t now = time(NULL);
  if(events[numEvent].start <= (int)now){//Get percent completed if event has started
    int eventDuration = events[numEvent].end - events[numEvent].start;
    int timeElapsed = (int)now - events[numEvent].start;
    int percent = 100 * timeElapsed / eventDuration;
    if(percent > 100){//Event is past complete
      percent = 100;
    }
    return percent;
  }else return -1;
}

//Gets one of the stored events' time info as a formatted string
char *get_event_time_string(int numEvent,char *buffer,int bufSize){
  if(!init)events_init();
  if(numEvent >= NUM_EVENTS)return NULL;//Check if event is within bounds
  if(strcmp(events[numEvent].title,"")==0)return NULL;//Check if event exists
  if(events[numEvent].start == 0)return NULL;//Check if event has a start time
  if(strcmp(events[numEvent].title,"") == 0)return NULL;//Check if event has a title
  int percent = get_percent_complete(numEvent);
  
  if(percent != -1){//return percent complete if event has started
    if(ltos((long)percent,buffer,bufSize)==NULL)return NULL;
    if(((int)strlen(buffer)+(int)strlen("% complete"))>bufSize)return NULL;
    strcat(buffer,"% complete");
  }else{//Get time until event if event hasn't started
    time_t now = time(NULL);
    if(events[numEvent].end < now){//Event is over, request new data and return null
      request_update(event_request);
      return NULL;
    }
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
    #ifdef DEBUG_EVENTS 
      struct tm *tick_time = localtime((time_t *)&(events[numEvent].start));
      static char s_buffer[16];
      strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M %d %e" : "%I:%M %d %e", tick_time);
      APP_LOG(APP_LOG_LEVEL_DEBUG,"get_event_time_string:Event %s starts at %s, %s from now",events[numEvent].title,s_buffer,buffer);
      tick_time = localtime((time_t *)&(events[numEvent].end));
      strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M %d %e" : "%I:%M %d %e", tick_time);
      APP_LOG(APP_LOG_LEVEL_DEBUG,"get_event_time_string:Event %s ends at %s",events[numEvent].title,s_buffer);
    #endif
  }
  return buffer;
}

//Gets an event's display color string
void get_event_color(int numEvent,char * buffer){
  if(!init)events_init();
  if(numEvent >= NUM_EVENTS)return;//Check if event is within bounds
  if(strcmp(events[numEvent].title,"")==0)return;//Check if event exists
  #ifdef DEBUG_EVENTS 
    APP_LOG(APP_LOG_LEVEL_DEBUG,"get_event_color:Copying event number %d color:%s ",numEvent,events[numEvent].color);
  #endif
  strcpy(buffer,events[numEvent].color);
}








