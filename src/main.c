#include <pebble.h>
#include "events.h"
#include "message_handler.h"
#include "util.h"
#include "display_handler.h"
#include "storage_keys.h"
#include "debug.h"

//----------LOCAL VALUE DEFINITIONS----------
//#define DEBUG_MAIN  //uncomment to enable main program debug logging

//----------STATIC FUNCTIONS----------
/**
*Updates the currently displayed time
*/
static void update_time() {
  setPreview1();
  #ifdef DEBUG_MAIN
  APP_LOG(APP_LOG_LEVEL_DEBUG,"update_time: starting time update");
  #endif
  time_t now = time(NULL);
  #ifdef DEBUG_MAIN
  APP_LOG(APP_LOG_LEVEL_DEBUG,"update_time: setting display time");
  #endif
  set_time(now);//update time display
  for(int i = 0; i < NUM_EVENTS; i++){//update display events
    char eventTitle[MAX_EVENT_LENGTH];
    char eventTime[MAX_EVENT_LENGTH];
    char eventColor[7];
    get_event_title(i, eventTitle, sizeof(eventTitle));
    get_event_time_string(i, eventTime, sizeof(eventTime));
    get_event_color(i, eventColor);
    update_event_display(i, eventTitle, eventTime, get_percent_complete(i), eventColor);
  }
  //if phone is connected, possibly get updates
  if(connection_service_peek_pebble_app_connection()){
    for(int i = 0; i < NUM_UPDATE_TYPES; i++){
      //load update time & update frequency
      time_t lastUpdate = get_update_time(i);
      int updateFreq = get_update_frequency(i);
      #ifdef DEBUG_MAIN 
        char updateType[10];
        switch((UpdateType) i){
          case UPDATE_TYPE_EVENT:
            strcpy(updateType,"Events");
            break;
          case UPDATE_TYPE_BATTERY:
            strcpy(updateType,"Battery");
            break;
          case UPDATE_TYPE_WEATHER:
            strcpy(updateType,"Weather");
            break;
          case UPDATE_TYPE_INFOTEXT:
            strcpy(updateType,"InfoText");
            break;
          default:
            strcpy(updateType,"Invalid Type!");
        }
        APP_LOG(APP_LOG_LEVEL_DEBUG,"update_time:%s= Last Update:%d Next: %d minutes, updateFreq:%d min",
              updateType,(int)lastUpdate,((int)lastUpdate+updateFreq-(int)now)/60,updateFreq/60);
      #endif
      if(now > (lastUpdate + updateFreq)){
        #ifdef DEBUG_MAIN
          APP_LOG(APP_LOG_LEVEL_DEBUG,"update_time: sending request");
        #endif
        request_update(i);
      }
    }    
  }
  else update_text("X",TEXT_PHONE_BATTERY);//phone is disconnected, set phone battery to X
  
  //Finally,update pebble battery info
  char pbl_battery_buf[6];
  getPebbleBattery(pbl_battery_buf);
  update_text(pbl_battery_buf,TEXT_PEBBLE_BATTERY);
  
}

//Automatically called every minute
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

//initialize program
void handle_init(void) {
  #ifdef DEBUG_MAIN 
  APP_LOG(APP_LOG_LEVEL_DEBUG,"handle_init:INIT BEGIN");
  #endif
  //save launch time for stats
  setLaunchTime(time(NULL));
  if(persist_exists(PERSIST_KEY_UPTIME))
    setSavedUptime(persist_read_int(PERSIST_KEY_UPTIME));
  //initialize modules
  display_init();
  events_init();
  message_handler_init();
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  // Make sure the time is displayed from the start
  update_time();
  #ifdef DEBUG_MAIN 
  APP_LOG(APP_LOG_LEVEL_DEBUG,"handle_init:INIT SUCCESS");
  #endif 
  
  
  
  
}

//unload program
void handle_deinit(void) {
  events_deinit();
  messaging_deinit();
  display_deinit();
  //Update runtime stats
  persist_write_int(PERSIST_KEY_UPTIME, getTotalUptime());
}

//MAIN
int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
