#include <pebble.h>
#include "debug.h"
#include "display_handler.h"
#include "events.h"

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
  while(dictItem != NULL && dictItem->length > 0){
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

int lastMemUsed = 0;

/**debug method that prints memory usage along with
*a string for identification purposes
*@param string printed in the debug output
*/
void memDebug(char * string){
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Used:%dB  Free:%dB  Change:%dB  Info:%s",
          (int) heap_bytes_used(),
          (int) heap_bytes_free(),
          (int) heap_bytes_used()-lastMemUsed,
          string);
  lastMemUsed = heap_bytes_used();
}

void setPreview1(){
  time_t now = time(NULL);
  update_text("84%",TEXT_PHONE_BATTERY);
  add_event(0,"Work",now - 14600,now+23530,"FF0000");
  add_event(1,"Sleep",now + 999560,now+1000000,"FFFF00");
  update_weather(30,808);
  update_text("Cloudy",TEXT_INFOTEXT);
}

void setPreview2(){
  time_t now = time(NULL);
  char newColors[][7] = {"AAFFFF","555555","000055","550055","",""};
  update_colors(newColors); 
  update_text("84%",TEXT_PHONE_BATTERY);
  update_text("25 Unread",TEXT_INFOTEXT);
  update_weather(55,508);
  add_event(0,"School",now - 14600,now+2353,"0000FF");
  add_event(1,"Movie",now + 9995,now+10000,"00FF00");
}