#include <pebble.h>
#include "events.h"
#include "messaging.h"
#include "util.h"
#include "display.h"

//----------LOCAL VALUE DEFINITIONS----------
#define DEBUG 0  //set to 1 to enable main program debug logging

//----------STATIC VARIABLES----------
static time_t lastBatteryUpdate; //the last time a phone battery update was requested
static int batteryUpdateFreq = 300; //how often (in seconds) to request a phone battery update

//----------STATIC FUNCTIONS----------
/**
*Updates the currently displayed time
*/
static void update_time() {
  time_t now = time(NULL);
  set_time(now);
  //if phone is connected, possibly get updates
  if(connection_service_peek_pebble_app_connection()){
    //Get event updates if scheduled
    time_t lastEventUpdate = get_last_update_time();
    int eventUpdateFreq = get_event_update_freq();
    if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"update_time:Time: %d Last Update:%d Next Update Scheduled in %d seconds",
                   (int)now,(int)lastEventUpdate,(int)lastEventUpdate+eventUpdateFreq-(int)now);
    if(now > (lastEventUpdate + eventUpdateFreq)){
      if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"update_time:Requesting event update");
      request_event_updates();
    }
    update_event_display(); //update event display information
    //Also update phone battery info
    if(now > (lastBatteryUpdate + batteryUpdateFreq)){
      if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"update_time:Requesting battery update");
      lastBatteryUpdate = now;
      request_battery_update();
    }
  }else{//phone is disconnected, set phone battery to X
    update_phone_battery("X");
  }
  //Finally,update pebble battery info
  char pbl_battery_buf[7];
  BatteryChargeState charge_state = battery_state_service_peek();
  snprintf(pbl_battery_buf, sizeof(pbl_battery_buf), "%d%%", charge_state.charge_percent);
  if (charge_state.is_charging) strcat(pbl_battery_buf,"+");
  update_watch_battery(pbl_battery_buf);
}

//Automatically called every minute
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

//initialize program
void handle_init(void) {
  events_init();
  display_init();
  messaging_init();
   // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  // Make sure the time is displayed from the start
  update_time(); 
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"handle_init:INIT SUCCESS");
}

//unload program
void handle_deinit(void) {
  display_deinit();
  events_deinit();
  messaging_deinit();
}

//MAIN
int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
