#include <pebble.h>
#include "modules/core/settings.h"
#include "modules/core/clock.h"
#include "modules/core/app_drawer.h"

// Global ping state variables
bool g_ping_active = false;
char g_partner_name[32] = "";
static AppTimer *s_ping_timer = NULL;

static void clear_ping_indicator(void *data) {
  g_ping_active = false;
  s_ping_timer = NULL;
  layer_mark_dirty(window_get_root_layer(window_stack_get_top_window()));
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *ping_tuple = dict_find(iterator, MESSAGE_KEY_PING_RECEIVED);
  Tuple *partner_name_tuple = dict_find(iterator, MESSAGE_KEY_PARTNER_NAME);
  
  if (ping_tuple) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Ping received from phone!");
    g_ping_active = true;
    vibes_double_pulse();
    light_enable_interaction();
    
    if (s_ping_timer) {
      app_timer_cancel(s_ping_timer);
    }
    s_ping_timer = app_timer_register(4000, clear_ping_indicator, NULL);
    
    layer_mark_dirty(window_get_root_layer(window_stack_get_top_window()));
  }
  
  if (partner_name_tuple) {
    snprintf(g_partner_name, sizeof(g_partner_name), "%s", partner_name_tuple->value->cstring);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped! Reason: %d", reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed! Reason: %d", reason);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
  settings_init();
  clock_init();
  app_drawer_init();
  
  // Initialize AppMessage for ping functionality
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  const int inbox_size = 256;
  const int outbox_size = 256;
  app_message_open(inbox_size, outbox_size);
}

static void deinit() {
  if (s_ping_timer) {
    app_timer_cancel(s_ping_timer);
  }
  app_drawer_deinit();
  clock_deinit();
  settings_deinit();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
