#include <pebble.h>
#include "events.h"
#include "messaging.h"
#include "util.h"
#include "display.h"
#include "keys.h"

//----------LOCAL VALUE DEFINITIONS----------
#define DEBUG_MAIN  //uncomment enable main program debug logging

//----------STATIC FUNCTIONS----------
/**
*Updates the currently displayed time
*/
static void update_time() {
  time_t now = time(NULL);
  set_time(now);//update time display
  update_event_display(); //update event display information
  
  //if phone is connected, possibly get updates
  if(connection_service_peek_pebble_app_connection()){
    time_t lastEventRequest,lastBatteryRequest,lastInfoRequest,lastWeatherRequest;
    int eventUpdateFreq, battUpdateFreq, infoUpdateFreq,weatherUpdateFreq;
    //Load last update times
    lastEventRequest = get_request_time(event_request);
    lastBatteryRequest = get_request_time(battery_request);
    lastInfoRequest = get_request_time(infotext_request);
    lastWeatherRequest = get_request_time(weather_request);
    
    eventUpdateFreq = get_update_frequency(event_request);
    battUpdateFreq = get_update_frequency(battery_request);
    infoUpdateFreq = get_update_frequency(infotext_request);
    weatherUpdateFreq = get_update_frequency(weather_request);
    #ifdef DEBUG_MAIN
      APP_LOG(APP_LOG_LEVEL_DEBUG,"update_time:Events= Last Update:%d Next Update Scheduled in %d seconds",
            (int)lastEventRequest,(int)lastEventRequest+eventUpdateFreq-(int)now);
      APP_LOG(APP_LOG_LEVEL_DEBUG,"update_time:Battery= Last Update:%d Next Update Scheduled in %d seconds",
            (int)lastBatteryRequest,(int)lastBatteryRequest+battUpdateFreq-(int)now);
      APP_LOG(APP_LOG_LEVEL_DEBUG,"update_time:InfoText= Last Update:%d Next Update Scheduled in %d seconds",
            (int)lastInfoRequest,(int)lastInfoRequest+infoUpdateFreq-(int)now);
      APP_LOG(APP_LOG_LEVEL_DEBUG,"update_time:Weather= Last Update:%d Next Update Scheduled in %d seconds",
            (int)lastWeatherRequest,(int)lastWeatherRequest+weatherUpdateFreq-(int)now);
    #endif
    if(now > (lastEventRequest + eventUpdateFreq)){//event update
      #ifdef DEBUG_MAIN
        APP_LOG(APP_LOG_LEVEL_DEBUG,"update_time: sending request:Event request");
      #endif
      request_update(event_request);
    }
    if(now > (lastBatteryRequest + battUpdateFreq)){//battery update
      #ifdef DEBUG_MAIN
        APP_LOG(APP_LOG_LEVEL_DEBUG,"update_time: sending request:Battery request");
      #endif
      request_update(battery_request);
    }
    if(now > (lastInfoRequest + infoUpdateFreq)){//infotext update
      #ifdef DEBUG_MAIN
        APP_LOG(APP_LOG_LEVEL_DEBUG,"update_time: sending request:InfoText request");
      #endif
      request_update(infotext_request);
    }
    if(now > (lastWeatherRequest + weatherUpdateFreq)){//weather update
      #ifdef DEBUG_MAIN
        APP_LOG(APP_LOG_LEVEL_DEBUG,"update_time: sending request:Weather request");
      #endif
      request_update(weather_request);     
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
  #ifdef DEBUG_MAIN 
    APP_LOG(APP_LOG_LEVEL_DEBUG,"handle_init:INIT BEGIN");
  #endif
  
  //save launch time for stats
  setLaunchTime(time(NULL));
  if(persist_exists(PERSIST_KEY_UPTIME))
    setSavedUptime(persist_read_int(PERSIST_KEY_UPTIME));
  //initialize modules
  events_init();
  display_init();
  messaging_init();
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
  display_deinit();
  events_deinit();
  messaging_deinit();
  //Update runtime stats
  persist_write_int(PERSIST_KEY_UPTIME, getTotalUptime());
}

//MAIN
int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
