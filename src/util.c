#include <pebble.h>
#include "util.h"


static int savedUptime = 0;
static time_t lastLaunch = 0;



/**
*Copies the pebble's remaining battery percentage into a buffer
*@param buffer a buffer of at least 6 bytes
*/
void getPebbleBattery(char * buffer){
  BatteryChargeState charge_state = battery_state_service_peek();
  snprintf(buffer, 6, "%d%%", charge_state.charge_percent);
  if (charge_state.is_charging) strcat(buffer,"+");
}

/**
*Gets the time since the watchface last launched
*@return uptime in seconds
*/
int getUptime(){
  return (int)time(NULL) - (int) lastLaunch;
}

/**
*Gets the total uptime since installation
*@return  total uptime in seconds
*/
int getTotalUptime(){
  return savedUptime + getUptime();
}

/**
*Sets the total uptime from previous watchface instances
*@param uptime in seconds
*/
void setSavedUptime(int uptime){
  savedUptime = uptime;
}

/**
*Sets the last watchface launch time
*@param launchTime time set on init
*/
void setLaunchTime(time_t launchTime){
  lastLaunch = launchTime;
}

/**
*Copies a string to a char pointer, re-allocating memory for the
*destination pointer so it is exactly large enough.
*@param dest the destination pointer
*@param src the source string
*@pre dest is either NULL or was previously passed to this function
*@post dest points to a copy of src, or NULL if memory allocation failed
*@return dest
*/
char * malloc_strcpy(char * dest, char * src){
  if(dest != NULL){
    free(dest);
    dest = NULL;
  }
  dest = malloc(strlen(src) + 1);
  if(dest != NULL) strcpy(dest,src);
  return dest;
}

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
char * malloc_set_text(TextLayer * textLayer,char * oldString, char * src){
  char * newString = malloc(strlen(src) + 1);
  strcpy(newString,src);
  text_layer_set_text(textLayer, newString);
  if(oldString != NULL){
    free(oldString);
  }
  return newString;
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


