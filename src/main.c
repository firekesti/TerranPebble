#include <pebble.h>
  
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_bluetooth_layer;
static GFont s_time_font;
static GFont s_date_font;
static GColor background_color = GColorBlack;
static GColor text_color = GColorWhite;
// Vibe pattern: ON in ms then OFF etc
static const uint32_t const disconnect_segments[] = { 100, 100, 400, 100, 100 };
static const uint32_t const connect_segments[] = { 200, 100, 200, 100, 50 };

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 2h hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the Time TextLayer
  text_layer_set_text(s_time_layer, buffer);
}

static void update_date() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "Day Mon Dt";

  // Write the current hours and minutes into the buffer
  strftime(buffer, sizeof("Day Mon Dt"), "%a %b %d", tick_time);

  // Display this time on the Time TextLayer
  text_layer_set_text(s_date_layer, buffer);
}

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100%";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "bzz!");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_layer, battery_text);
}

static void handle_bluetooth_init(bool connected) {
  layer_set_hidden((Layer *)s_bluetooth_layer, connected);  
}

static void handle_bluetooth(bool connected) {
  layer_set_hidden((Layer *)s_bluetooth_layer, connected);
  if (connected) {
    VibePattern pattern = {
      .durations = connect_segments,
      .num_segments = ARRAY_LENGTH(connect_segments),
    };
    vibes_enqueue_custom_pattern(pattern);
  } else {
    VibePattern pattern = {
      .durations = disconnect_segments,
      .num_segments = ARRAY_LENGTH(disconnect_segments),
    };
    vibes_enqueue_custom_pattern(pattern);
  }
}

static void main_window_load(Window *window) {
  window_set_background_color(window, background_color);
  
  // Create battery TextLayer
  // top: 10
  // paddingTop: 10
  // height: 24
  // bottom: 34
  s_battery_layer = text_layer_create(GRect(0, 10, 144, 24));
  text_layer_set_text_color(s_battery_layer, text_color);
  text_layer_set_background_color(s_battery_layer, background_color);
  text_layer_set_text(s_battery_layer, "100%");


  // Create time TextLayer
  // top: 40
  // paddingTop: 6
  // height: 48
  // bottom: 88
  s_time_layer = text_layer_create(GRect(0, 40, 144, 48));
  text_layer_set_background_color(s_time_layer, background_color);
  text_layer_set_text_color(s_time_layer, text_color);
  text_layer_set_text(s_time_layer, "00:00");
  
  // Create date TextLayer
  // top: 106
  // paddingTop: 18
  // height: 24
  // bottom: 130
  s_date_layer = text_layer_create(GRect(0, 106, 144, 24));
  text_layer_set_background_color(s_date_layer, background_color);
  text_layer_set_text_color(s_date_layer, text_color);
  text_layer_set_text(s_date_layer, "Day Mon Dt");
  
  // Create bluetooth TextLayer
  // top: 136
  // paddingTop: 12
  // height: 24
  // bottom: 160
  // paddingBottom: 6
  s_bluetooth_layer = text_layer_create(GRect(0, 136, 144, 24));
  text_layer_set_text_color(s_bluetooth_layer, text_color);
  text_layer_set_background_color(s_bluetooth_layer, background_color);
  text_layer_set_text(s_bluetooth_layer, "<B>");
  
  // Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_TERRAN_48));
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_TERRAN_24));

  // Apply to TextLayers
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_font(s_date_layer, s_date_font);
  text_layer_set_font(s_battery_layer, s_date_font);
  text_layer_set_font(s_bluetooth_layer, s_date_font);
  
  // Improve the layout to be more like a watchface
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  text_layer_set_text_alignment(s_bluetooth_layer, GTextAlignmentCenter);

  // Add them as child layers to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_battery_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_bluetooth_layer));
  
  // Make sure the right info is displayed from the start
  update_time();
  update_date();
  handle_battery(battery_state_service_peek());
  handle_bluetooth_init(bluetooth_connection_service_peek());
}

static void main_window_unload(Window *window) {
  // Destroy TextLayers
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_bluetooth_layer);

  // Unload GFont
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  update_date();
  handle_battery(battery_state_service_peek());
}
  
static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  // Register with battery service
  battery_state_service_subscribe(handle_battery);
  // Register with bluetooth service
  bluetooth_connection_service_subscribe(handle_bluetooth);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);

  // Unsubscribe!
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
