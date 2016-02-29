#include <pebble.h>
#include "main.h"  

static Window *window;
static TextLayer *textlayer;

int buzz_intensity, buzz_interval, buzz_start;

// schedules next wake up and does current buzz
static void schedule_and_buzz() {
  
  //canceling and resubscribing to wakeup
  wakeup_cancel_all();
  wakeup_service_subscribe(NULL);
    
  //getting current time
  time_t next= time(NULL);  
    
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Buzz Interval = %d, buzz_start = %d", buzz_interval, buzz_start);
  
  // if inital call is to start at specific hour time - calculate that time
  if (buzz_start != START_IMMEDIATLY) {
    
      int period;
      switch (buzz_start){
        case START_ON_15MIN: period = 15; break;
        case START_ON_HALFHOUR: period = 30; break;
        case START_ON_HOUR: period = 60; break;
        default: period = 60; break;
      }
    
      next = (next / 60) * 60; // rounding to a minute (removing seconds)
      next = next - (next % (period * 60)) + (period * 60); // calculating exact start timing    
    
      //and after that there will be regular wakeup call
      persist_write_int(KEY_BUZZ_START, START_IMMEDIATLY);
      buzz_start = START_IMMEDIATLY;
    
  } else { // otherwise scheduling next call according to inteval
      next = next + buzz_interval*60;  
  }
  
  //buzzing
  switch(buzz_intensity){
    case BUZZ_SHORT:
      vibes_short_pulse();
      break;
    case BUZZ_LONG:
      vibes_short_pulse();
      break;
    case BUZZ_DOUBLE:
      vibes_double_pulse();
      break;  
  }
  
  // scheduling next wakeup
  wakeup_schedule(next, 0, false);  
  
}

// handle configuration change
static void in_recv_handler(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_read_first(iterator);

  while (t)  {

    switch(t->key)    {
    
      case KEY_BUZZ_INTENSITY:
           persist_write_int(KEY_BUZZ_INTENSITY, t->value->uint8);
           buzz_intensity = t->value->uint8;
           break;
      case KEY_BUZZ_INTERVAL:
           persist_write_int(KEY_BUZZ_INTERVAL, t->value->uint8);
           buzz_interval = t->value->uint8;
           break;
      case KEY_BUZZ_START:
           persist_write_int(KEY_BUZZ_START, t->value->uint8);
           buzz_start = t->value->uint8;
           break;      
    }    
    
    t = dict_read_next(iterator);
  }
  
  //removing window causing app exit;
  window_stack_pop(false);
  
}  

static void init() {
  
  // reading stored values
  buzz_intensity = persist_exists(KEY_BUZZ_INTENSITY)? persist_read_int(KEY_BUZZ_INTENSITY) : BUZZ_SHORT;
  buzz_interval = persist_exists(KEY_BUZZ_INTERVAL)? persist_read_int(KEY_BUZZ_INTERVAL) : 60;
  buzz_start = persist_exists(KEY_BUZZ_START)? persist_read_int(KEY_BUZZ_START) : START_ON_HOUR;
  
  // if it's not a wake up call, meaning we're launched from Config - display current config
  if ( launch_reason() != APP_LAUNCH_WAKEUP) { 
    
      window = window_create();
      window_set_background_color(window, GColorBlack);
//       #ifdef PBL_PLATFORM_APLITE
//         window_set_fullscreen(window, true);
//       #endif
      GRect bounds = layer_get_bounds(window_get_root_layer(window));
    
      #ifdef PBL_RECT
        textlayer = text_layer_create(GRect(bounds.origin.x + 10, bounds.origin.y + 20, bounds.size.w -20, bounds.size.h - 20));
      #else
        textlayer = text_layer_create(GRect(bounds.origin.x + 15, bounds.origin.y + 35, bounds.size.w - 30, bounds.size.h -30));
      #endif
      text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
      text_layer_set_background_color(textlayer, GColorBlack);
      text_layer_set_text_color(textlayer, GColorWhite);
      text_layer_set_text(textlayer, "Please configure the Nag on your phone and tap 'Set' to start the app. This screen will automatically disppear.");
      text_layer_set_text_alignment(textlayer, GTextAlignmentCenter);
      layer_add_child(window_get_root_layer(window), text_layer_get_layer(textlayer));
    
      window_stack_push(window, false);
    
      // subscribing to JS messages
      app_message_register_inbox_received(in_recv_handler);
      app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
    
  } else { // if it is a wake up call - buzz and reschedule
     schedule_and_buzz();
  }  
}

static void deinit() {
  if (window) { // if UI existed - destroy it and schedule first buzz
    app_message_deregister_callbacks();
    text_layer_destroy(textlayer);
    window_destroy(window);
    if (buzz_intensity != BUZZ_DISABLED) {
      schedule_and_buzz();  
    }
  }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}