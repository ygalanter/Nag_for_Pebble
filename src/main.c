#include <pebble.h>
#include "main.h"  

static Window *window;
static TextLayer *textlayer;

int buzz_intensity, buzz_interval, buzz_start;

// schedules next wake up and does current buzz
static void schedule_and_buzz() {
  wakeup_cancel_all();
  wakeup_service_subscribe(NULL);

  time_t next;
  
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Buzz Interval = %d, buzz_start = %d", buzz_interval, buzz_start);
 
  // if inital call is to start at specific hour time - calculate that time
  if (buzz_start != START_IMMEDIATLY) {
    
      next = time(NULL);
      struct tm *t = localtime(&next);
    
      switch (buzz_start) {
        case START_ON_15MIN: // rounding to next quarter
          if (t->tm_min < 15) {
            t->tm_min = 15;
          } else if (t->tm_min < 30) {
            t->tm_min = 30;
          } else if (t->tm_min < 45) {
            t->tm_min = 45;
          } else {
            t->tm_min = 0;
            t->tm_hour++;  
          }
          break;
        case START_ON_HALFHOUR: // rounding to next half an hour
          if (t->tm_min < 30) {
            t->tm_min = 30;
          } else {
            t->tm_min = 0;
            t->tm_hour++;  
          }
          t->tm_sec = 0;
          break;
        case START_ON_HOUR: // simple incrementing hour (currently doesnt handle day change (if hour jumps from 23 to 24))
          t->tm_hour++;
          t->tm_min = 0;
          t->tm_sec = 0;
          break;
      }
    
      // making next time
      next = mktime(t);
    
      //and after that there will be regular wakeup call
      persist_write_int(KEY_BUZZ_START, START_IMMEDIATLY);
      buzz_start = START_IMMEDIATLY;
      
  } else { // otherwise start from current time
      next = time(NULL) + buzz_interval*60;
  }
  
  wakeup_schedule(next, 0, false);  
  
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
      window_set_background_color(window, GColorWhite);
      window_set_fullscreen(window, true);
    
      textlayer = text_layer_create(GRect(10,10,124,148));
      text_layer_set_text_color(textlayer, GColorWhite);
      text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
      text_layer_set_background_color(textlayer, GColorBlack);
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