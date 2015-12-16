#pragma once

#define DICT_SIZE 512
#define INBOX_SIZE 1024
#define OUTBOX_SIZE 1024


//adds a new message to the queue
void add_message(uint8_t dictBuf[DICT_SIZE]);

//Sends the first message in the queue
void send_message();

//Removes the first message in the queue
void delete_message();

//Removes all messages in the queue
void delete_all_messages();

//return 1 if the queue is empty, 0 if messages remain
int message_queue_empty();