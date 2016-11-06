#include <pebble.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "SmallMaths.h"

static Window *s_main_window;//points to a window variable to be accessed as needed

static TextLayer *lower_text_layer;
static TextLayer *points_text_layer;

static GBitmap *neck;
static GBitmap *upvote;
static GBitmap *downvote;
static GBitmap *snoo_happy_up;
static GBitmap *snoo_happy_down;
static GBitmap *snoo_sad_up;
static GBitmap *snoo_sad_down;

static BitmapLayer *neck_layer;
static BitmapLayer *vote_layer_1;
static BitmapLayer *vote_layer_2;
static BitmapLayer *vote_layer_3;
static BitmapLayer *snoo_layer;

int points = 100;
char* points_char;

static AppTimer* moveTimer;
static int allowedTime;

int order_3, order_2, order_1; // Vars for the third, second, and first closest move to the player.
int time_to_change; // Amount of votes to go through before changing direction (varies from 1 to 6)
int current_direction;

// Converts integer to c string
char* itoa(int val, int base){
	static char buf[32] = {0};
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = "0123456789abcdef"[val % base];
	return &buf[i+1];
}

// Shifts the move integers to the right and generates a new move at the end
void shift_moves()
{
  order_1 = order_2;
  order_2 = order_3;
  if (time_to_change != 0) order_3 = current_direction;
  else
  {
    srand(time(NULL));
    time_to_change = rand() % 7;
    
    order_3 = current_direction;
  }
  order_3 = rand() % 2;
}

// Waits for a designated amount of time
void waitFor (int secs) {
    int retTime = time(0) + secs;   // Get finishing time.
    while (time(0) < retTime);               // Loop until it arrives.
}

// Change the snoo's position
void move_snoo(Window *window, bool lose)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  if (snoo_layer!=NULL) 
  {
    bitmap_layer_destroy(snoo_layer);
    snoo_layer = NULL;
  }
  
  // Change position
  if (order_1 == 0) // If Snoo is moving down to avoid an upvote (why)
  {
    snoo_layer = bitmap_layer_create(GRect((bounds.size.w/2)+40,(bounds.size.h/2)+10, 20, 20));
    bitmap_layer_set_compositing_mode(snoo_layer, GCompOpSet);
    bitmap_layer_set_bitmap(snoo_layer, snoo_happy_down);
    layer_add_child(window_layer, bitmap_layer_get_layer(snoo_layer));
  }
  if (order_1 == 1) // If Snoo is moving up to avoid a downvote
  {
    snoo_layer = bitmap_layer_create(GRect((bounds.size.w/2)+40,(bounds.size.h/2)-30, 20, 20));
    bitmap_layer_set_compositing_mode(snoo_layer, GCompOpSet);
    bitmap_layer_set_bitmap(snoo_layer, snoo_happy_up);
    layer_add_child(window_layer, bitmap_layer_get_layer(snoo_layer));
  }
}

void refresh_window(Window *window)
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  if (vote_layer_3!=NULL) 
  {
    bitmap_layer_destroy(vote_layer_3);
    vote_layer_3 = NULL;
  }
  if (vote_layer_2!=NULL)
  {
    bitmap_layer_destroy(vote_layer_2);
    vote_layer_2 = NULL;
  }
  if (vote_layer_1!=NULL)
  {
    bitmap_layer_destroy(vote_layer_1);
    vote_layer_1 = NULL;
  }
  
  // Change the far left arrow
  if (order_3 == 0)
  {
    vote_layer_3 = bitmap_layer_create(GRect((bounds.size.w/2)-80,(bounds.size.h/2)-30, 20, 20));
    bitmap_layer_set_compositing_mode(vote_layer_3, GCompOpSet);
    bitmap_layer_set_bitmap(vote_layer_3, upvote);
    layer_add_child(window_layer, bitmap_layer_get_layer(vote_layer_3));
  }
  else
  {
    vote_layer_3 = bitmap_layer_create(GRect((bounds.size.w/2)-80,(bounds.size.h/2)+10, 20, 20));
    bitmap_layer_set_compositing_mode(vote_layer_3, GCompOpSet);
    bitmap_layer_set_bitmap(vote_layer_3, downvote);
    layer_add_child(window_layer, bitmap_layer_get_layer(vote_layer_3));
  }
  
  // Change the center arrow
  if (order_2 == 0)
  {
    vote_layer_2 = bitmap_layer_create(GRect((bounds.size.w/2)-40,(bounds.size.h/2)-30, 20, 20));
    bitmap_layer_set_compositing_mode(vote_layer_2, GCompOpSet);
    bitmap_layer_set_bitmap(vote_layer_2, upvote);
    layer_add_child(window_layer, bitmap_layer_get_layer(vote_layer_2));
  }
  else
  {
    vote_layer_2 = bitmap_layer_create(GRect((bounds.size.w/2)-40,(bounds.size.h/2)+10, 20, 20));
    bitmap_layer_set_compositing_mode(vote_layer_2, GCompOpSet);
    bitmap_layer_set_bitmap(vote_layer_2, downvote);
    layer_add_child(window_layer, bitmap_layer_get_layer(vote_layer_2));
  }
  
  // Change the far right arrow
  if (order_1 == 0)
  {
    vote_layer_1 = bitmap_layer_create(GRect((bounds.size.w/2),(bounds.size.h/2)-30, 20, 20));
    bitmap_layer_set_compositing_mode(vote_layer_1, GCompOpSet);
    bitmap_layer_set_bitmap(vote_layer_1, upvote);
    layer_add_child(window_layer, bitmap_layer_get_layer(vote_layer_1));
  }
  else
  {
    vote_layer_1 = bitmap_layer_create(GRect((bounds.size.w/2),(bounds.size.h/2)+10, 20, 20));
    bitmap_layer_set_compositing_mode(vote_layer_1, GCompOpSet);
    bitmap_layer_set_bitmap(vote_layer_1, downvote);
    layer_add_child(window_layer, bitmap_layer_get_layer(vote_layer_1));
  }
  
   window_stack_push(window, true);
}

void callback(void *data) {
  vibes_short_pulse();
  waitFor(1);
  text_layer_set_text(lower_text_layer,"Game Over! Press UP to begin a new game.");
  points = 0;
  current_direction = 1;
  order_1 = 1;
  order_2 = 1;
  order_3 = 1;
  srand(time(NULL));
  time_to_change = rand() % 7;
}

void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  Window *window = (Window *)context;
  if (order_1 == 1)
  {
    app_timer_cancel(moveTimer);
    allowedTime = ((int) (10000 * sm_exp(-0.04 * points) +.5));
    points++;
    points_char = itoa(points, 10);
    move_snoo(window, 0);
    shift_moves();
    text_layer_set_text(lower_text_layer, "");
    text_layer_set_text(points_text_layer,points_char);
    moveTimer = app_timer_register(allowedTime, callback, NULL);
    refresh_window(window);
  }
  else
  {
    vibes_short_pulse();
    waitFor(1);
    text_layer_set_text(lower_text_layer,"Game Over! Press UP to begin a new game.");
    points = 0;
    current_direction = 1;
    order_1 = 1;
    order_2 = 1;
    order_3 = 1;
    srand(time(NULL));
    time_to_change = rand() % 7;
  }
}

void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  Window *window = (Window *)context;
  if (order_1 == 0)
  {
    app_timer_cancel(moveTimer);
    allowedTime = ((int) (10000 * sm_exp(-0.04 * points) +.5));
    points++;
    points_char = itoa(points, 10);
    move_snoo(window, 1);
    shift_moves();
    text_layer_set_text(lower_text_layer, "");
    text_layer_set_text(points_text_layer,points_char);
    moveTimer = app_timer_register(allowedTime, callback, NULL);
    refresh_window(window);
  }
  else
  {
    vibes_short_pulse();
    waitFor(1);
    text_layer_set_text(lower_text_layer,"Game Over! Press UP to begin a new game.");
    points = 0;
    current_direction = 1;
    order_1 = 1;
    order_2 = 1;
    order_3 = 1;
    srand(time(NULL));
    time_to_change = rand() % 7;
  }
}

void config_provider(Window *window) {
  // Single click / repeat-on-hold config:
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  lower_text_layer = text_layer_create(
      GRect(0, 120, bounds.size.w, 50));
  points_text_layer = text_layer_create(
      GRect(0, 0, bounds.size.w, 20));

  // Lower Text Layer
  text_layer_set_background_color(lower_text_layer, GColorClear);
  text_layer_set_text_color(lower_text_layer, GColorBlack);
  text_layer_set_text(lower_text_layer, "Press UP to begin!");
  text_layer_set_font(lower_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(lower_text_layer, GTextAlignmentCenter);
  
  // Points text layer
  text_layer_set_background_color(points_text_layer, GColorClear);
  text_layer_set_text_color(points_text_layer, GColorBlack);
  text_layer_set_text(points_text_layer, "0");
  text_layer_set_font(points_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(points_text_layer, GTextAlignmentCenter);

  // Add them as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(lower_text_layer));
  layer_add_child(window_layer, text_layer_get_layer(points_text_layer));
  
  /*
  Create the Bitmap Images
  */
  neck = gbitmap_create_with_resource(RESOURCE_ID_NECK);
  upvote = gbitmap_create_with_resource(RESOURCE_ID_UPVOTE);
  downvote = gbitmap_create_with_resource(RESOURCE_ID_DOWNVOTE);
  snoo_happy_up = gbitmap_create_with_resource(RESOURCE_ID_SNOO_HAPPY_UP);
  snoo_happy_down = gbitmap_create_with_resource(RESOURCE_ID_SNOO_HAPPY_DOWN);
  snoo_sad_up = gbitmap_create_with_resource(RESOURCE_ID_SNOO_SAD_UP);
  snoo_sad_down = gbitmap_create_with_resource(RESOURCE_ID_SNOO_SAD_DOWN);
  /*
  Old Code
  // Set aside rectangles for the neck bitmap
  upvote_layer = bitmap_layer_create(GRect(0,(bounds.size.h/2)-30, 20, 20));
  downvote_layer = bitmap_layer_create(GRect(0,(bounds.size.h/2)+10, 20, 20));
  
  // Other stuff
  bitmap_layer_set_compositing_mode(upvote_layer, GCompOpSet);
  bitmap_layer_set_compositing_mode(downvote_layer, GCompOpSet);
  
  // Set bitmaps
  bitmap_layer_set_bitmap(upvote_layer, upvote);
  bitmap_layer_set_bitmap(downvote_layer, downvote);
  
  // Add our children to the layer
  layer_add_child(window_layer, bitmap_layer_get_layer(upvote_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(downvote_layer));
  */
  
  // Add neck to layer
  neck_layer = bitmap_layer_create(GRect(0, (bounds.size.h/2)-10, bounds.size.w, 20));
  bitmap_layer_set_compositing_mode(neck_layer, GCompOpSet);
  bitmap_layer_set_bitmap(neck_layer, neck);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(neck_layer));
  
  // Add initial snoo to layer
  snoo_layer = bitmap_layer_create(GRect((bounds.size.w/2)+40,(bounds.size.h/2)-30, 20, 20));
  bitmap_layer_set_compositing_mode(snoo_layer, GCompOpSet);
  bitmap_layer_set_bitmap(snoo_layer, snoo_happy_up);
  layer_add_child(window_layer, bitmap_layer_get_layer(snoo_layer));
  
  current_direction = 1;
  order_1 = 1;
  order_2 = 1;
  order_3 = 1;
  srand(time(NULL));
  time_to_change = rand() % 7;
  moveTimer = app_timer_register(10000,NULL,NULL);
  allowedTime = ((int) (10000 * sm_exp(-0.04 * points) +.5));
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(lower_text_layer);
  text_layer_destroy(points_text_layer);
  
  // Destroy Bitmaps
  gbitmap_destroy(neck);
  gbitmap_destroy(upvote);
  gbitmap_destroy(downvote);
  
  bitmap_layer_destroy(neck_layer);
  bitmap_layer_destroy(vote_layer_3);
  bitmap_layer_destroy(vote_layer_2);
  bitmap_layer_destroy(vote_layer_1);
  
  bitmap_layer_destroy(snoo_layer);
}

static void init() {
  
  //create main window element and assign to the pointer created above
  s_main_window = window_create();
  
  //Handlers to load or unload
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  window_set_click_config_provider(s_main_window, (ClickConfigProvider) config_provider);
  window_stack_push(s_main_window, true);
  
}

static void deinit() {
  
  //destroy window
  window_destroy(s_main_window);
}

int main(void){
  init();
  app_event_loop();
  deinit();
}
