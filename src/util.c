#include <pebble.h>
#include "util.h"

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
  while(base >= 1){
      str[i] = '0' + val/base;
      val%=base;
      base/=10;
      if(!(i==0 && str[i]=='0' && val!=0))i++;//no leading zeroes allowed
      if(i > strlen)return NULL;//respect the buffer size
  }
  str[i]='\0';//must be null terminated
  return str;
}

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