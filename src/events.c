#include <pebble.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "events.h"

struct eventStruct{
  char title[MAX_EVENT_LENGTH];
  long start;
  long end;
  char color[7];
};
struct eventStruct events[NUM_EVENTS];
int init = 0;
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
  strcpy(events[numEvent].title,title);
  strcpy(events[numEvent].color,color);
  events[numEvent].start = start;
  events[numEvent].end = end;
}

/**
*Returns the long value of a char string
*@param str the string
*@param strlen length to convert
*@return the represented long's value
*/
long stol(char * str,int strlen){
  long val=0;
  long base=1;
  int i;
  for(i=strlen-1;i>=0;i--){
    val += (str[i]-'0')*base;
    base *= 10;
    //APP_LOG(APP_LOG_LEVEL_DEBUG,"Val = %lld, base = %lld, i = %i, str = %s",val,base,i,str);
  }
  return val;
}
/**
*Copies a long's value into a string
*@param val the long to copy
*@param str the buffer string to copy into
*@param strlen the length of the buffer
*@return a pointer to str if conversion succeeds, null if it fails 
*/
char *ltos(long val, char *str,int strlen){
  int base = 1;
  while(val/(base*10) != 0)base *= 10;
  int i = 0;
  while(val > 0){
    //APP_LOG(APP_LOG_LEVEL_DEBUG,"Val = %ld, base = %i, i = %i, str = %s",val,base,i,str);
    str[i] = '0' + val/base;
    val%=base;
    base/=10;
    if(!(i==0 && str[i]=='0'))i++;//no leading zeroes allowed
    if(i > strlen)return NULL;//respect the buffer size
  }
  str[i]='\0';//must be null terminated
  return str;
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
  if(!init)initEvents();
  if(numEvent >= NUM_EVENTS)return NULL;
  if(events[numEvent].start == 0)return NULL;
  
  time_t now = time(NULL);
  //APP_LOG(APP_LOG_LEVEL_DEBUG,"Time found");
  if(events[numEvent].start <= (long)now){
    int percent = (int)(100 *((float)((long)now - events[numEvent].start) 
                              /(events[numEvent].end - events[numEvent].start)));
    if(ltos((long)percent,buffer,bufSize)==NULL)return NULL;
    if(((int)strlen(buffer)+(int)strlen("% complete"))>bufSize)return NULL;
    strcat(buffer,"% complete");
  }else{
    long time = events[numEvent].start - now;
    long weeks =0,days=0,hours=0,minutes=0;
    if(time>0)weeks = time / 604800;
    time %= 604800;
    if(time>0)days = time / 86400;
    time &= 86400;
    if(time>0)hours = time / 3600;
    time &= 3600;
    if(time>0)minutes = time / 60;
    char buf[20];
    if(weeks != 0){
      if(ltos(weeks,buf,20)!=NULL){
        strcpy(buffer,buf);
        strcat(buffer," weeks");
      }
    }
    if(days != 0){
      if(ltos(days,buf,20)!=NULL){
        strcat(buffer,buf);
        strcat(buffer," days");
      }
    }
    if(hours != 0){
      if(ltos(hours,buf,20)!=NULL){
        strcat(buffer,buf);
        strcat(buffer," hours");
      }
    }
    if(minutes != 0){
      if(ltos(days,buf,20)!=NULL){
        strcat(buffer,buf);
        strcat(buffer," minutes");
      }
    }
  }
  return buffer;
}