#include <pebble.h>
#include "events.h"
#include "messaging.h"
#include "util.h"
#include "display.h"

#define DEBUG 0

static time_t lastBatteryUpdate;
static int batteryUpdateFreq = 300;


static void update_time() {
  time_t now = time (NULL);
  set_time(now);
  //update events periodically
  time_t lastEventUpdate = get_last_update_time();
  int eventUpdateFreq = get_event_update_freq();
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"Time: %d Last Update:%d Next Update Scheduled:%d",
                   (int)now,(int)lastEventUpdate,(int)lastEventUpdate+eventUpdateFreq);
  if(now > (lastEventUpdate + eventUpdateFreq)){
    if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"Requesting event update");
    request_event_updates();
  }
  else update_event_display();
  if(now > (lastBatteryUpdate + batteryUpdateFreq)){
    if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"Requesting battery update");
    request_battery_update();
  }
}
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}
//AppMessage callback functions


void handle_init(void) {
  display_init();
  events_init();
  messaging_init();
   // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  // Make sure the time is displayed from the start
  update_time(); 
  if(DEBUG)APP_LOG(APP_LOG_LEVEL_DEBUG,"INIT SUCCESS");
}

void handle_deinit(void) {
  messaging_deinit();
  events_deinit();
  display_deinit();
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
