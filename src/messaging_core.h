/*
*@File messaging_core.h
*Manages sending and receiving of messages
*/

#pragma once
#include <pebble.h>

#define DICT_SIZE 256//AppMessage dictionary size
typedef void (* InboxHandler)(DictionaryIterator *iterator);

/**
*Initialize messaging and open AppMessage
*/
void open_messaging();

/**
*Free up memory and disable messaging
*/
void close_messaging();

/*
*Registers a function to pass incoming messages
*to be read
*@param handler the message reading function
*/
void register_inbox_handler(InboxHandler handler);

/**
*Adds a message to the outbox queue, to
*be sent soon
*@param dictBuf a dictionary message buffer to
*be copied
*/
void add_message(uint8_t dictBuf[DICT_SIZE]);
