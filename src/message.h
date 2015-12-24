/**
*@File message.h
*Handles all functionality related to
*communication between the pebble and phone
*/

#pragma once
#include <pebble.h>
#define DICT_SIZE 512
#define INBOX_SIZE 1024
#define OUTBOX_SIZE 1024

//APPMESSAGE KEY DEFINITIONS
#define KEY_REQUEST 0
#define KEY_BATTERY_UPDATE 1
#define KEY_EVENT_TITLE 2
#define KEY_EVENT_START 3
#define KEY_EVENT_END 4
#define KEY_EVENT_COLOR 5
#define KEY_EVENT_NUM 6
#define UPDATE_FREQ 7

/**
*adds a new message to the queue
*param dictBuf a dictionary buffer containing the new message
*/
void add_message(uint8_t dictBuf[DICT_SIZE]);

//Sends the first message in the queue
void send_message();

//Removes the first message in the queue
void delete_message();

//Removes all messages in the queue
void delete_all_messages();

//return 1 if the queue is empty, 0 if messages remain
int message_queue_empty();

//Prints debug info for a dictionary
void debugDictionary(DictionaryIterator * it);