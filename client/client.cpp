#include <Arduino.h>
#include <Adafruit_ILI9341.h>
#include <SD.h>
#include "consts_and_types.h"
#include "map_drawing.h"

// the variables to be shared across the project, they are declared here!
shared_vars shared;

Adafruit_ILI9341 tft = Adafruit_ILI9341(clientpins::tft_cs, clientpins::tft_dc);

// -- buffer code from class -- //
// max size of buffer, including null terminator
const uint16_t buf_size = 256;
// current number of chars in buffer, not counting null terminator
uint16_t buf_len = 0;
// input buffer
char* buffer = (char *) malloc(buf_size);
// -------------------------- //

void setup() {
	// initialize Arduino
	init();

	// initialize zoom pins
	pinMode(clientpins::zoom_in_pin, INPUT_PULLUP);
	pinMode(clientpins::zoom_out_pin, INPUT_PULLUP);

	// initialize joystick pins and calibrate centre reading
	pinMode(clientpins::joy_button_pin, INPUT_PULLUP);
	// x and y are reverse because of how our joystick is oriented
	shared.joy_centre = xy_pos(analogRead(clientpins::joy_y_pin), analogRead(clientpins::joy_x_pin));

	// initialize serial port
	Serial.begin(9600);
	Serial.flush(); // get rid of any leftover bits

	// initially no path is stored
	shared.num_waypoints = 0;

	// initialize display
	shared.tft = &tft;
	shared.tft->begin();
	shared.tft->setRotation(3);
	shared.tft->fillScreen(ILI9341_BLUE); // so we know the map redraws properly

	// initialize SD card
	if (!SD.begin(clientpins::sd_cs)) {
		Serial.println("Initialization has failed. Things to check:");
		Serial.println("* Is a card inserted properly?");
		Serial.println("* Is your wiring correct?");
		Serial.println("* Is the chipSelect pin the one for your shield or module?");

		while (1) {} // nothing to do here, fix the card issue and retry
	}

	// initialize the shared variables, from map_drawing.h
	// doesn't actually draw anything, just initializes values
	initialize_display_values();

	// initial draw of the map, from map_drawing.h
	draw_map();
	draw_cursor();

	// initial status message
	status_message("FROM?");
}

void process_input() {
	// read the zoom in and out buttons
	shared.zoom_in_pushed = (digitalRead(clientpins::zoom_in_pin) == LOW);
	shared.zoom_out_pushed = (digitalRead(clientpins::zoom_out_pin) == LOW);

	// read the joystick button
	shared.joy_button_pushed = (digitalRead(clientpins::joy_button_pin) == LOW);

	// joystick speed, higher is faster
	const int16_t step = 64;

	// get the joystick movement, dividing by step discretizes it
	// currently a far joystick push will move the cursor about 5 pixels
	xy_pos delta(
			// the funny x/y swap is because of our joystick orientation
			(analogRead(clientpins::joy_y_pin)-shared.joy_centre.x)/step,
			(analogRead(clientpins::joy_x_pin)-shared.joy_centre.y)/step
			);
	delta.x = -delta.x; // horizontal axis is reversed in our orientation

	// check if there was enough movement to move the cursor
	if (delta.x != 0 || delta.y != 0) {
		// if we are here, there was noticeable movement

		// the next three functions are in map_drawing.h
		erase_cursor();       // erase the current cursor
		move_cursor(delta);   // move the cursor, and the map view if the edge was nudged
		if (shared.redraw_map == 0) {
			// it looks funny if we redraw the cursor before the map scrolls
			draw_cursor();      // draw the new cursor position
		}
	}
}

bool wait4line(unsigned long dur){
	/*******************
	 * wait for time dur (in millisecond)for a new line to reach the serial
	 * return whether a line was recieved or not
	 ******************/
	// clear the buffer
	buf_len = 0;
	buffer[buf_len] = 0;

	// start the timer
	unsigned long start = millis();
	unsigned long now = start;
	do {
		// read from the serial
		char in_char;
		if (Serial.available()) {
			// read the incoming byte:
			in_char = Serial.read();

			// if end of line is received, waiting4line is done:
			if (in_char == '\n' || in_char == '\r') {
				// now we process the buffer
				return true;
			}
			else {
				// add character to buffer, provided that we don't overflow.
				// drop any excess characters.
				if ( buf_len < buf_size-1 ) {
					buffer[buf_len] = in_char;
					buf_len++;
					buffer[buf_len] = 0;
				}
			}
		}
		now = millis();
	} while (now-start < dur);
	
	// if we reached here then a line was not recieved
	return false;
}

void splitStr(char* &input, char** output, int tCount, const char* delim){
	/*******************
	 * split input into tokens in output array
	 * fill up output array with the last token
	 * if there are more spaces than tokens
	 ******************/
	int i=0;
	char* p;
	// loop through tokens
	for (p = strtok(buffer,delim); p != NULL and i < tCount; p = strtok(NULL, delim)) {
		output[i] = p;
		i++;
	}
	// fill the remaining output spaces with the last token
	while (i<tCount){
		output[i] = p;
		i++;
	}

}

bool verify(const char* input, const char* expected){
	/***************
	 * Compare input to expected
	 * update status_message if they differ
	 ***************/
	// check if we get expected input
	if(strcmp(input, expected) != 0){
		char msg[30];
		strcpy (msg,"Expected:`");
		strcat (msg, expected);
		strcat (msg, "`");
		strcat (msg, " got:`");
		strcat (msg, input);
		strcat (msg, "`");
		status_message(msg);
		return false;
	}
	return true;
}

void getRoute(lon_lat_32 start, lon_lat_32 end){
	/***************
	 * Populates the shared.waypoints with a path from
	 * start to end by communicating with the server
	 ***************/
	
	// clear out the Serial first
	char t;
	while(Serial.available()){
		t = Serial.read();
	}

	// send the request
	Serial.print("R ");
	Serial.print(start.lat);
	Serial.print(" ");
	Serial.print(start.lon);
	Serial.print(" ");
	Serial.print(end.lat);
	Serial.print(" ");
	Serial.println(end.lon);

	// wait for input from server
	status_message("Receiving path...");
	// catch a line on the serial
	if(!wait4line(10000)){
		// if wait4line doesnt catch a line in 10000 ms, then timeout
		status_message("Path Timeout!");
		return;
	}

	// split the line into 4 using " " deliminator
	char* tokens[4];
	splitStr(buffer, tokens, 4, " ");

	// verify input is expected
	if(!verify(tokens[0], "N")){return;} 

	// verify the recieved waypoint count
	int32_t wpCount = atoi(tokens[1]);
	if(wpCount > max_waypoints){
		status_message("Way too many waypoints");
		return;
	}

	// save it and aknowlodge
	shared.num_waypoints = wpCount;
	Serial.println("A");

	// get each waypoint position
	status_message("Getting waypoints...");
	for(int i=0; i<wpCount; i++){
		if(!wait4line(1000)){
			status_message("Waypoint Timeout!");
			return;
		}
		
		splitStr(buffer, tokens, 4, " ");

		// verify input is expected
		if(!verify(tokens[0], "W")){return;} 

		char* lat = tokens[1];
		char* lon = tokens[2];
		lon_lat_32 wpoint(atol(lon), atol(lat));
		shared.waypoints[i] = wpoint;
		Serial.println("A");
	}

	if(!wait4line(10000)){
		// if wait4line doesnt catch a line in 10000 ms 
		// then timeout
		status_message("E signal Timeout!");
		return;
	}

	splitStr(buffer, tokens, 4, " "); 
	// verify input is expected
	if(!verify(tokens[0], "E")){return;}
	status_message("GOT WAYPOINTS");
}

void draw_route(){
	/***************
	 * draw the route from start to end point on
	 * the tft arduino screen
	 ***************/
	for(int i = 0; i < shared.num_waypoints - 1; i++){
		// draw a line between each waypoint
		// get current waypoint
		lon_lat_32 A = shared.waypoints[i];
		// get next waypoint
		lon_lat_32 B = shared.waypoints[i+1];

		// calculate they positions on the global map
		int A_X = longitude_to_x(shared.map_number, A.lon); 
		int A_Y = latitude_to_y(shared.map_number, A.lat); 
		int B_X = longitude_to_x(shared.map_number, B.lon); 
		int B_Y = latitude_to_y(shared.map_number, B.lat);
		
		// calculate their positions on the screen
		int Ax = A_X - shared.map_coords.x;
		int Ay = A_Y - shared.map_coords.y;
		int Bx = B_X - shared.map_coords.x;
		int By = B_Y - shared.map_coords.y;
		
		// draw a blue line between them
		shared.tft -> drawLine(Ax, Ay, Bx, By, 0x001F);
	}
}

int main() {
	setup();

	// very simple finite state machine:
	// which endpoint are we waiting for?
	enum {WAIT_FOR_START, WAIT_FOR_STOP} curr_mode = WAIT_FOR_START;

	// the two points that are clicked
	lon_lat_32 start, end;

	while (true) {
		// clear entries for new state
		shared.zoom_in_pushed = 0;
		shared.zoom_out_pushed = 0;
		shared.joy_button_pushed = 0;
		shared.redraw_map = 0;

		// reads the three buttons and joystick movement
		// updates the cursor view, map display, and sets the
		// shared.redraw_map flag to 1 if we have to redraw the whole map
		// NOTE: this only updates the internal values representing
		// the cursor and map view, the redrawing occurs at the end of this loop
		process_input();

		// if a zoom button was pushed, update the map and cursor view values
		// for that button push (still need to redraw at the end of this loop)
		// function zoom_map() is from map_drawing.h
		if (shared.zoom_in_pushed) {
			zoom_map(1);
			shared.redraw_map = 1;
		}
		else if (shared.zoom_out_pushed) {
			zoom_map(-1);
			shared.redraw_map = 1;
		}

		// if the joystick button was clicked
		if (shared.joy_button_pushed) {

			if (curr_mode == WAIT_FOR_START) {
				// if we were waiting for the start point, record it
				// and indicate we are waiting for the end point
				start = get_cursor_lonlat();
				curr_mode = WAIT_FOR_STOP;
				status_message("TO?");

				// wait until the joystick button is no longer pushed
				while (digitalRead(clientpins::joy_button_pin) == LOW) {}
			}
			else {
				// if we were waiting for the end point, record it
				// and then communicate with the server to get the path
				end = get_cursor_lonlat();

				// TODO: communicate with the server to get the waypoints
				getRoute(start, end);
				
				// now we have stored the path length in
				// shared.num_waypoints and the waypoints themselves in
				// the shared.waypoints[] array, switch back to asking for the
				// start point of a new request
				shared.redraw_map = 1;
				curr_mode = WAIT_FOR_START;
				// wait until the joystick button is no longer pushed
				while (digitalRead(clientpins::joy_button_pin) == LOW) {}
			}
		}

		if (shared.redraw_map) {
			// redraw the status message
			if (curr_mode == WAIT_FOR_START) {
				status_message("FROM?");
			}
			else {
				status_message("TO?");
			}

			// redraw the map and cursor
			draw_map();
			draw_cursor();

			// TODO: draw the route if there is one
			draw_route();
		}
	}

	Serial.flush();
	return 0;
}
