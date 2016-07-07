#include <pebble.h>
  
static Window *s_main_window;

#define KEY_INVERT 0
#define KEY_BACKGROUND 1
#define KEY_DATE 2
#define KEY_GRAPHPERIOD 3

static TextLayer *s_timeperiod_layer;
static TextLayer *s_day_layer;
static TextLayer *s_year_layer;
static GFont s_digi_14_font;
static GFont s_square_small_font;
static GFont s_small_font;

static Layer *time_layer;
static Layer *date_layer;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static InverterLayer *s_inverter_layer;

// GRAPH
static BitmapLayer *s_graph_underline;

#define GRAPH_BARS 16
static BitmapLayer *s_graph_layers[GRAPH_BARS];
static int s_graph_heights[GRAPH_BARS];

// TIME
#define TOTAL_TIME_DIGITS 5
static BitmapLayer *time_digits_layers[TOTAL_TIME_DIGITS];

const int TIME_IMAGE_RESOURCE_IDS[] = {
    RESOURCE_ID_IMAGE_TIME_0_BLACK,
    RESOURCE_ID_IMAGE_TIME_1_BLACK,
    RESOURCE_ID_IMAGE_TIME_2_BLACK,
    RESOURCE_ID_IMAGE_TIME_3_BLACK,
    RESOURCE_ID_IMAGE_TIME_4_BLACK,
    RESOURCE_ID_IMAGE_TIME_5_BLACK,
    RESOURCE_ID_IMAGE_TIME_6_BLACK,
    RESOURCE_ID_IMAGE_TIME_7_BLACK,
    RESOURCE_ID_IMAGE_TIME_8_BLACK,
    RESOURCE_ID_IMAGE_TIME_9_BLACK,
    RESOURCE_ID_IMAGE_TIME_COLON_BLACK
};

#define COLON 10

static GBitmap *timeDigits[11];

// DATE
#define TOTAL_DATE_DIGITS 5
static BitmapLayer *date_digits_layers[TOTAL_DATE_DIGITS];

const int DATE_IMAGE_RESOURCE_IDS[] = {
    RESOURCE_ID_IMAGE_DATE_0_BLACK,
    RESOURCE_ID_IMAGE_DATE_1_BLACK,
    RESOURCE_ID_IMAGE_DATE_2_BLACK,
    RESOURCE_ID_IMAGE_DATE_3_BLACK,
    RESOURCE_ID_IMAGE_DATE_4_BLACK,
    RESOURCE_ID_IMAGE_DATE_5_BLACK,
    RESOURCE_ID_IMAGE_DATE_6_BLACK,
    RESOURCE_ID_IMAGE_DATE_7_BLACK,
    RESOURCE_ID_IMAGE_DATE_8_BLACK,
    RESOURCE_ID_IMAGE_DATE_9_BLACK,
    RESOURCE_ID_IMAGE_DATE_DASH_BLACK
};

#define DASH 10

static GBitmap *dateDigits[11];

// BATTERY
static bool mCharging = false;
static uint8_t batteryPercent;

static BitmapLayer *batTop;
static BitmapLayer *batBottom;

#define BATTERY_BARS 10
static BitmapLayer *s_bat_layers[BATTERY_BARS];

static BitmapLayer *s_charge_layer;
static GBitmap *s_charge_bitmap;
//////////

// No BLUETOOTH
static BitmapLayer *s_bt_layer;
static GBitmap *s_bt_bitmap;

// GRAPH UPDATES
static void update_graph() {
	// Move Everything Up 1
	int p = 41;
	int h;
	int i;
	for (i = 0; i < GRAPH_BARS-1; i++) {
		h = s_graph_heights[i+1];
		s_graph_heights[i] = h;
		GRect frame1 = (GRect) {
			.origin = GPoint(p, 49-h),
			.size = GSize(2,h)
	    };
		layer_set_frame(bitmap_layer_get_layer(s_graph_layers[i]), frame1);
		p+=4;
    }
	
	// Random new bar
	h = rand() % 40;
	s_graph_heights[GRAPH_BARS-1] = h;
	GRect frame = (GRect) {
			.origin = GPoint(p, 49-h),
			.size = GSize(2,h)
	    };
	layer_set_frame(bitmap_layer_get_layer(s_graph_layers[GRAPH_BARS-1]), frame);
}
//------------------------------------------------------------------------------//

unsigned short get_display_hour(unsigned short hour) {
    if (clock_is_24h_style()) {
		return hour;
    }
    unsigned short display_hour = hour % 12;
    // Converts "0" to "12"
    return display_hour ? display_hour : 12;
}

static void set_container_image(BitmapLayer *bmp_layer, const GBitmap *bmp_image, GPoint origin) {
	GRect frame = (GRect) {
		.origin = origin,
		.size = bmp_image->bounds.size
	};
	bitmap_layer_set_bitmap(bmp_layer, bmp_image);
	layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);
} 

// UPDATE DATE
static void update_date(struct tm *tick_time){
	bool dateFormat = persist_read_bool(KEY_DATE);
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "update_date %d", dateFormat);
	
	static GPoint dayDigitPos[2] = {{37,118},{53,118}};
	static GPoint monDigitPos[2] = {{77,118},{93,118}};
	
	static char buffer_day[] = "00";
	static char buffer_month[] = "00";	
	static char buffer_daytxt[] = "xxxxxxxxxx";
	static char buffer_year[] = "0000";	
	
	strftime(buffer_day, sizeof("00"), "%d", tick_time);
	strftime(buffer_month, sizeof("00"), "%m", tick_time);		
	strftime(buffer_daytxt, sizeof("xxxxxxxxxx"), "%A", tick_time);
	strftime(buffer_year, sizeof("0000"), "%Y", tick_time);
	
	text_layer_set_text(s_day_layer, buffer_daytxt);
	text_layer_set_text(s_year_layer, buffer_year);	
	
	if (dateFormat) {
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "update_date 1");
		set_container_image(date_digits_layers[0], dateDigits[atoi(buffer_day) / 10], dayDigitPos[0]);
		set_container_image(date_digits_layers[1], dateDigits[atoi(buffer_day) % 10], dayDigitPos[1]);

		set_container_image(date_digits_layers[3], dateDigits[atoi(buffer_month) / 10], monDigitPos[0]);
		set_container_image(date_digits_layers[4], dateDigits[atoi(buffer_month) % 10], monDigitPos[1]);
	}
	else {
		//APP_LOG(APP_LOG_LEVEL_DEBUG, "update_date 0");
		set_container_image(date_digits_layers[0], dateDigits[atoi(buffer_day) / 10], monDigitPos[0]);
		set_container_image(date_digits_layers[1], dateDigits[atoi(buffer_day) % 10], monDigitPos[1]);

		set_container_image(date_digits_layers[3], dateDigits[atoi(buffer_month) / 10], dayDigitPos[0]);
		set_container_image(date_digits_layers[4], dateDigits[atoi(buffer_month) % 10], dayDigitPos[1]);
	}
}

// UPDATE HOURS
static void update_hours(struct tm *tick_time) {
	static GPoint hourDigitPos[2] = {{21,54}, {45,54}};	
	static char buffer_timeperiod[] = "24";
	
	unsigned short display_hour = get_display_hour(tick_time->tm_hour);	
	
	if (!clock_is_24h_style()) {
		strftime(buffer_timeperiod, sizeof("xx"), "%p", tick_time);
	}
	text_layer_set_text(s_timeperiod_layer, buffer_timeperiod);
	
    set_container_image(time_digits_layers[0], timeDigits[display_hour / 10], hourDigitPos[0]);
    set_container_image(time_digits_layers[1], timeDigits[display_hour % 10], hourDigitPos[1]);
	
	layer_set_frame(time_layer, GRect(0, 0, 144, 168));	
}

// MINUTE UPDATES
static void update_minutes(struct tm *tick_time) {
	static GPoint minDigitPos[2] = {{79,54}, {103,54}};
	
    set_container_image(time_digits_layers[3], timeDigits[tick_time->tm_min / 10], minDigitPos[0]);
    set_container_image(time_digits_layers[4], timeDigits[tick_time->tm_min % 10], minDigitPos[1]);
}


//--------------------------------------------------------------//
//																//
// 							TIME UPDATES						//
//																//
//--------------------------------------------------------------//
static void update_time(struct tm *tick_time, TimeUnits units_changed) {
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "update_time() - Start");	
	int period = persist_read_int(KEY_GRAPHPERIOD);
	static char buffer_sec[] = "00";
	strftime(buffer_sec, sizeof("00"), "%S", tick_time);
	
	if (units_changed & DAY_UNIT) {
		update_date(tick_time);
	}
	if (units_changed & HOUR_UNIT) {
		update_hours(tick_time);
	}
	if (units_changed & MINUTE_UNIT) {
		update_minutes(tick_time);
	}
	if (units_changed & SECOND_UNIT) {
		if (period > 0 && atoi(buffer_sec) % period == 0) update_graph();
	}
}
//--------------------------------------------------------------//

// BATTERY UPDATES
static void update_battery(BatteryChargeState charge_state) {
	mCharging = charge_state.is_charging;
    batteryPercent = charge_state.charge_percent;
	APP_LOG(APP_LOG_LEVEL_DEBUG, "update_battery() - %d" ,batteryPercent);
	
	 if (charge_state.is_charging) {
		layer_set_hidden(bitmap_layer_get_layer(s_charge_layer), false);
    }
	else {
		layer_set_hidden(bitmap_layer_get_layer(s_charge_layer), true);
	}
	
	// Show/Hide Bars
	int i;
	for (i = 1; i < BATTERY_BARS; i++) {
		if (i+1 <= (batteryPercent / 10)) {
			layer_set_hidden(bitmap_layer_get_layer(s_bat_layers[i]), false);
		}
		else {
			layer_set_hidden(bitmap_layer_get_layer(s_bat_layers[i]), true);
		}
	}
}

// BT UPDATES
void bluetooth_connection_callback(bool connected) {
	if (!connected && !mCharging) {
		// vibe!
		vibes_long_pulse();
    }
	layer_set_hidden(bitmap_layer_get_layer(s_bt_layer), connected);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "update_bt() - BT Updated");
}

//--------------------------------------------------------------//
//																//
// 						MESSAGE HANDLER							//
//																//
//--------------------------------------------------------------//
static void in_recv_handler(DictionaryIterator *iterator, void *context)
{
	//Get Tuple
	Tuple *t = dict_read_first(iterator);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "in_recv_handler() - HI");
	while(t != NULL)
	{
		switch(t->key)
		{
			case KEY_INVERT:
				if(strcmp(t->value->cstring, "on") == 0)
				{
					layer_set_hidden(inverter_layer_get_layer(s_inverter_layer), false);
					persist_write_bool(KEY_INVERT, true);
					APP_LOG(APP_LOG_LEVEL_DEBUG, "in_recv_handler() - Inverted");
				}
				else if(strcmp(t->value->cstring, "off") == 0)
				{
					layer_set_hidden(inverter_layer_get_layer(s_inverter_layer), true);
					persist_write_bool(KEY_INVERT, false);
					APP_LOG(APP_LOG_LEVEL_DEBUG, "in_recv_handler() - Not Inverted");
				}
				break;
			
			case KEY_BACKGROUND:
				if(strcmp(t->value->cstring, "on") == 0)
				{
					gbitmap_destroy(s_background_bitmap);
					s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SQUAREFACE_BACKGROUND_BLACK);
					bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
					persist_write_bool(KEY_BACKGROUND, true);
					APP_LOG(APP_LOG_LEVEL_DEBUG, "in_recv_handler() - Show Back");
				}
				else if(strcmp(t->value->cstring, "off") == 0)
				{
					gbitmap_destroy(s_background_bitmap);
					s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SQUAREFACE_BACKGROUND_WHITE);
					bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
					persist_write_bool(KEY_BACKGROUND, false);
					APP_LOG(APP_LOG_LEVEL_DEBUG, "in_recv_handler() - HideBack");
				}
				break;
			
			case KEY_DATE: ;
				time_t now = time(NULL);
				if(strcmp(t->value->cstring, "DD-MM") == 0)
				{				
					persist_write_bool(KEY_DATE, true);
					APP_LOG(APP_LOG_LEVEL_DEBUG, "in_recv_handler() - DD-MM");
				}
				else if(strcmp(t->value->cstring, "MM-DD") == 0)
				{
					persist_write_bool(KEY_DATE, false);
					APP_LOG(APP_LOG_LEVEL_DEBUG, "in_recv_handler() - MM-DD");
				}
				update_date(localtime(&now));
				break;
			
			case KEY_GRAPHPERIOD:
				if(strcmp(t->value->cstring, "0") == 0) persist_write_int(KEY_GRAPHPERIOD, 0);
				else if(strcmp(t->value->cstring, "1") == 0) persist_write_int(KEY_GRAPHPERIOD, 1);
				else if(strcmp(t->value->cstring, "5") == 0) persist_write_int(KEY_GRAPHPERIOD, 5);
				else if(strcmp(t->value->cstring, "10") == 0) persist_write_int(KEY_GRAPHPERIOD, 10);
				else if(strcmp(t->value->cstring, "30") == 0) persist_write_int(KEY_GRAPHPERIOD, 30);
				else if(strcmp(t->value->cstring, "60") == 0) persist_write_int(KEY_GRAPHPERIOD, 60);
				int period = persist_read_int(KEY_GRAPHPERIOD);
				APP_LOG(APP_LOG_LEVEL_DEBUG, "in_recv_handler() - graph secs %d", period);
				break;
		}
    	t = dict_read_next(iterator);
	}
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

//--------------------------------------------------------------//
//																//
// 						MAIN WINDOW LOAD						//
//																//
//--------------------------------------------------------------//
static void main_window_load (Window *window) {
	int i; // for loops
	
	// Background Image
	bool showBackground = persist_read_bool(KEY_BACKGROUND);
	
	s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SQUAREFACE_BACKGROUND_BLACK);
	s_background_layer = bitmap_layer_create(GRect(0,0,144,168));
	bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));	
	
	if (!showBackground) { // IDIOTS METHOD OF HIDING BACKGROUND
		gbitmap_destroy(s_background_bitmap);
		s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SQUAREFACE_BACKGROUND_WHITE);
		bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
	}
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load() - Background Complete");		

	
	// Text Fonts
	s_digi_14_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DSDIGI_NORMAL_14));
	s_square_small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SQUARE_NORMAL_12));
	s_small_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load() - Fonts Complete");
	
	// Text Time Period
	s_timeperiod_layer = text_layer_create(GRect(104, 82, 20, 14));
	text_layer_set_background_color(s_timeperiod_layer, GColorClear);
	text_layer_set_text_color(s_timeperiod_layer, GColorWhite);
	text_layer_set_font(s_timeperiod_layer, s_digi_14_font);
	text_layer_set_text_alignment(s_timeperiod_layer, GTextAlignmentRight);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_timeperiod_layer));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load() - TimePeriod Complete");
	
	// Text Day
	s_day_layer = text_layer_create(GRect(36, 102, 72, 14));
	text_layer_set_background_color(s_day_layer, GColorClear);
	text_layer_set_text_color(s_day_layer, GColorWhite);
	text_layer_set_font(s_day_layer, s_digi_14_font);
	text_layer_set_text_alignment(s_day_layer, GTextAlignmentLeft);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_day_layer));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load() - Day Complete");
		
	// Text Year
	s_year_layer = text_layer_create(GRect(76, 135, 32, 14));
	text_layer_set_background_color(s_year_layer, GColorClear);
	text_layer_set_text_color(s_year_layer, GColorWhite);
	text_layer_set_font(s_year_layer, s_digi_14_font);
	text_layer_set_text_alignment(s_year_layer, GTextAlignmentRight);
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_year_layer));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load() - Year Complete");
	
	// GET DIGITS
	for (i = 0; i < 11; i++) {
		timeDigits[i] = gbitmap_create_with_resource(TIME_IMAGE_RESOURCE_IDS[i]);
		if (timeDigits[i] == NULL) {
			APP_LOG(APP_LOG_LEVEL_DEBUG, "init() - gbitmap_create Failed for bigDigits[%d]", i);
		}
	}
	
	for (i = 0; i < 11; i++) {
		dateDigits[i] = gbitmap_create_with_resource(DATE_IMAGE_RESOURCE_IDS[i]);
		if (dateDigits[i] == NULL) {
			APP_LOG(APP_LOG_LEVEL_DEBUG, "init() - gbitmap_create Failed for bigDigits[%d]", i);
		}
	}	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load() - GetDigits Complete");
	
	// TIME LAYER // 
    time_layer = layer_create(layer_get_frame(window_get_root_layer(window)));
    layer_add_child(window_get_root_layer(window), time_layer);	
	GRect dummy_frame = { {0, 0}, {0, 0} };
	
	for (i = 0; i < TOTAL_TIME_DIGITS; i++) {
		time_digits_layers[i] = bitmap_layer_create(dummy_frame);
		layer_add_child(time_layer, bitmap_layer_get_layer(time_digits_layers[i]));
    }
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load() - DisplayTime Complete");	
	
	// TIME COLON //
	set_container_image(time_digits_layers[2], timeDigits[COLON], GPoint(69, 65));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load() - Colon Complete");
	
	// DATE LAYER //
	date_layer = layer_create(layer_get_frame(window_get_root_layer(window)));
    layer_add_child(window_get_root_layer(window), date_layer);
	
	for (i = 0; i < TOTAL_DATE_DIGITS; i++) {
		date_digits_layers[i] = bitmap_layer_create(dummy_frame);
		layer_add_child(date_layer, bitmap_layer_get_layer(date_digits_layers[i]));
    }
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load() - DisplayDate Complete");
	
	// DATE DASH //
	set_container_image(date_digits_layers[2], dateDigits[DASH], GPoint(69, 126));
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load() - Dash Complete");
	
	// TOP GRAPH
	s_graph_underline = bitmap_layer_create(GRect(41, 50, 62, 1));
	bitmap_layer_set_background_color(s_graph_underline, GColorWhite);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_graph_underline));
	
	srand(time(NULL));
	
	int p = 41;
	int h = rand() % 39;
	for (i = 0; i < GRAPH_BARS; i++) {
		s_graph_layers[i] = bitmap_layer_create(GRect(p, 49-h, 2, h));
		s_graph_heights[i] = h;
		bitmap_layer_set_background_color(s_graph_layers[i], GColorWhite);
		layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_graph_layers[i]));
		p+=4;
		h = (rand()+(i)) % 39;
    }
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load() - Graph Init Complete");
	
	// BATTERY //
	batTop = bitmap_layer_create(GRect(21, 87, 85, 1));
	bitmap_layer_set_background_color(batTop, GColorWhite);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(batTop));
	
	batBottom = bitmap_layer_create(GRect(21, 92, 85, 1));
	bitmap_layer_set_background_color(batBottom, GColorWhite);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(batBottom));
	
	s_bat_layers[0] = bitmap_layer_create(GRect(21, 89, 4, 2));
	bitmap_layer_set_background_color(s_bat_layers[0], GColorWhite);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bat_layers[0]));
	
	int j = 26;
	for (i = 1; i < BATTERY_BARS; i++) {
		s_bat_layers[i] = bitmap_layer_create(GRect(j, 89, 8, 2));
		bitmap_layer_set_background_color(s_bat_layers[i], GColorWhite);
		layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bat_layers[i]));
		j+=9;
	}
	
	s_charge_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGE_BLACK);
	s_charge_layer = bitmap_layer_create(GRect(14,86,5,8));
	bitmap_layer_set_bitmap(s_charge_layer, s_charge_bitmap);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_charge_layer));
	layer_set_hidden(bitmap_layer_get_layer(s_charge_layer), true);
	
	update_battery(battery_state_service_peek());
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load() - Battery Init Complete");
	
	// BT DISCONNECT
	s_bt_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_DISCONNECTED_BLACK);
	s_bt_layer = bitmap_layer_create(GRect(64,140,16,16));
	bitmap_layer_set_bitmap(s_bt_layer, s_bt_bitmap);
	layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_layer));
	layer_set_hidden(bitmap_layer_get_layer(s_bt_layer), true);	
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load() - No BT Init Complete");
	
	// INVERTER
	bool inverted = persist_read_bool(KEY_INVERT);
	
	s_inverter_layer = inverter_layer_create(GRect(0, 0, 144, 168));
	layer_add_child(window_get_root_layer(window), (Layer *)s_inverter_layer);
	layer_set_hidden(inverter_layer_get_layer(s_inverter_layer), !inverted);
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_load() - Inverter Complete");
}

//--------------------------------------------------------------//
//																//
// xxxxx				MAIN WINDOW UN-LOAD				  xxxxx //
//																//
//--------------------------------------------------------------//
static void main_window_unload (Window *window) {
	int i;
	
	// Background Image
	layer_remove_from_parent(bitmap_layer_get_layer(s_background_layer));
	bitmap_layer_destroy(s_background_layer);
	gbitmap_destroy(s_background_bitmap);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_unload() - Background Destroyed");	
	
	// Text
  	text_layer_destroy(s_timeperiod_layer);
  	text_layer_destroy(s_day_layer);
  	text_layer_destroy(s_year_layer);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_unload() - Text Destroyed");	
	
	fonts_unload_custom_font(s_digi_14_font);
	fonts_unload_custom_font(s_square_small_font);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_unload() - Fonts Destroyed");
	
	// Layers	
	for (i = 0; i < TOTAL_TIME_DIGITS; i++) {
		bitmap_layer_destroy(time_digits_layers[i]);
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_unload() - Time Destroyed");
	
	for (i = 0; i < TOTAL_DATE_DIGITS; i++) {
		bitmap_layer_destroy(date_digits_layers[i]);
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_unload() - Date Destroyed");
	
	for (i = 0; i < GRAPH_BARS; i++) {
		bitmap_layer_destroy(s_graph_layers[i]);
	}
	bitmap_layer_destroy(s_graph_underline);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_unload() - Graph Destroyed");	
	
	for (i = 0; i < 11; i++) {
		gbitmap_destroy(timeDigits[i]);
		gbitmap_destroy(dateDigits[i]);
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_unload() - Digits Destroyed");	
	
	layer_destroy(time_layer);
	layer_destroy(date_layer);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_unload() - TimeDate Layers Destroyed");	
	
	bitmap_layer_destroy(batTop);
	bitmap_layer_destroy(batBottom);
	
	layer_remove_from_parent(bitmap_layer_get_layer(s_charge_layer));
	bitmap_layer_destroy(s_charge_layer);
	gbitmap_destroy(s_charge_bitmap);
	
	for (i = 0; i < BATTERY_BARS; i++) {
		bitmap_layer_destroy(s_bat_layers[i]);
	}
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_unload() - Battery Layers Destroyed");
	
	layer_remove_from_parent(bitmap_layer_get_layer(s_bt_layer));
	bitmap_layer_destroy(s_bt_layer);
	gbitmap_destroy(s_bt_bitmap);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_unload() - BT Layers Destroyed");	
  	
	inverter_layer_destroy(s_inverter_layer);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "main_window_unload() - Inverter Layer Destroyed");	
}
 
//--------------------------------------------------------------//
//							 INIT								//
//--------------------------------------------------------------//
static void init() {
	s_main_window = window_create();

	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});

	window_stack_push(s_main_window, true);
	
	app_message_register_inbox_received((AppMessageInboxReceived) in_recv_handler);
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);
	app_message_register_outbox_sent(outbox_sent_callback);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
	// UPDATE
	time_t now = time(NULL);
	update_time(localtime(&now), SECOND_UNIT|MINUTE_UNIT|HOUR_UNIT|DAY_UNIT);
	
	tick_timer_service_subscribe(SECOND_UNIT, update_time);	
	battery_state_service_subscribe(update_battery);
    bluetooth_connection_service_subscribe(bluetooth_connection_callback);
}

//--------------------------------------------------------------//
//							DE-INIT								//
//--------------------------------------------------------------//
static void deinit() {
	window_destroy(s_main_window);
	tick_timer_service_unsubscribe();
	battery_state_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
}

int main (void) {
	init();
	app_event_loop();
	deinit();
}