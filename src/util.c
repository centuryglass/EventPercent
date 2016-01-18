#include <pebble.h>
#include "util.h"

static int savedUptime = 0;
static time_t lastLaunch = 0;

int lastMemUsed = 0;

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
  if(oldString != NULL) free(oldString);
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


//debug functions
#ifdef DEBUG
//Tests the ltos function
void ltosTest(){
  char testBuf [10];
  strcpy(testBuf,"");
  long i;
  for(i = 0;i < 100;i++){
    strcpy(testBuf,"");
    ltos(i,testBuf,10);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"ltos test: Long value:%ld to string:%s",i,testBuf);
  }
}

/**
*Debug method that outputs all values of a dictionary
*@param it a pointer to a DictionaryIterator
*that currently has a dictionary open for reading.
*/
void debugDictionary(DictionaryIterator * it){
  DictionaryIterator safeCopy = *it;
  it = &safeCopy;//makes sure the original iterator isn't disturbed
  Tuple * dictItem = dict_read_first(it);
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Reading in a dictionary of size %u",(unsigned int)dict_size(it));
  int index = 0;
  while(dictItem != NULL){
    int key =(int) dictItem->key;
    long size =(long) dictItem->length;
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Item no.=%d, key=%d, size=%ld",index,key,size);
    if(dictItem->type == TUPLE_BYTE_ARRAY){
      //copy the data into a char array with enough room to add a null char
      char * bytedata = NULL;
      bytedata = malloc(size + 1);
      if(bytedata == NULL){
        APP_LOG(APP_LOG_LEVEL_DEBUG,"type=byte_array, value=?");
      }
      else{
        memcpy(bytedata,dictItem->value->data,size);
        bytedata[size] = '\0';
        APP_LOG(APP_LOG_LEVEL_DEBUG,"type=byte_array, value=%s",bytedata);
        free(bytedata);
      }
    }
    else if(dictItem->type == TUPLE_CSTRING){
      APP_LOG(APP_LOG_LEVEL_DEBUG,"type=cstring, value=%s",dictItem->value->cstring);
    }
    if(dictItem->type == TUPLE_UINT){
      APP_LOG(APP_LOG_LEVEL_DEBUG,"type=uInt, value=%u",(unsigned int)dictItem->value->uint32);
    }
    if(dictItem->type == TUPLE_INT){
      APP_LOG(APP_LOG_LEVEL_DEBUG,"type=int, value=%d",(int)dictItem->value->uint32);
    }
    index++;
    dictItem = dict_read_next(it);
  }
}

/**debug method that prints memory usage along with
*a string for identification purposes
*@param string printed in the debug output
*/
void memDebug(char * string){
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Used:%dB  Free:%dB  Change:%dB  Info:%s",
          heap_bytes_used(),
          heap_bytes_free(),
          heap_bytes_used()-lastMemUsed,
          string);
  lastMemUsed = heap_bytes_used();
}
#endif