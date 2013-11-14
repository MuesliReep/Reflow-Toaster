#include "Adafruit_MAX31855.h"
#include "U8glib.h"
#include "Profile.h"

// The relay control pin
#define RELAYPIN A0

// The SPI pins for the thermocouple sensor
#define MAX_CLK 5
#define MAX_CS 6
#define MAX_DATA 7

// The LCD control Pins
#define LCD_SCK 3
#define LCD_DATA 9
#define LCD_CS 8

// the Proportional control constant
#define Kp  10
// the Integral control constant
#define Ki  0.5
// the Derivative control constant 
#define Kd  100

// Windup error prevention, 5% by default
#define WINDUPPERCENT 0.05  

Adafruit_MAX31855 thermocouple(MAX_CLK, MAX_CS, MAX_DATA);
U8GLIB_ST7920_128X64 u8g(LCD_SCK, LCD_DATA, LCD_CS, U8G_PIN_NONE); 

// volatile means it is going to be messed with inside an interrupt 
// otherwise the optimization code will ignore the interrupt

volatile long seconds_time = 0;      // this will get incremented once a second
volatile float the_temperature;      // in celsius
volatile float previous_temperature; // the last reading (1 second ago)

// The target temperature in Celsius
float target_temperature;

// The reflow profile
Profile profile;

// we need this to be a global variable because we add error each second
float Summation;        // The integral of error since time = 0

int relay_state;       // whether the relay pin is high (on) or low (off)

void setup() {  
  
	// the relay pin controls the plate
	pinMode(RELAYPIN, OUTPUT);

	// Start the serial connection
	Serial.begin(9600); 
	Serial.println("Reflowduino!");
  
	// The data header (we have a bunch of data to track)
	Serial.print("Time (s)\tTemp (C)\tTarget (C)\tError\tSlope\tSummation\tPID Controller\tRelay");
 
	// Now that we are mucking with stuff, we should track our variables
	Serial.print("\t\tKp = "); Serial.print(Kp);
	Serial.print(" Ki = "); Serial.print(Ki);
	Serial.print(" Kd = "); Serial.println(Kd);
  
	// Setup the lcd
	// assign default color value
	if ( u8g.getMode() == U8G_MODE_R3G3B2 ) 
		u8g.setColorIndex(255);			// white
	else if ( u8g.getMode() == U8G_MODE_GRAY2BIT )
		u8g.setColorIndex(3);			// max intensity
	else if ( u8g.getMode() == U8G_MODE_BW )
		u8g.setColorIndex(1);			// pixel on
    
	// Print 
	u8g.firstPage();  
	do {
		u8g.setFont(u8g_font_9x15);
		u8g.drawStr( 0, 35, "Lets reflow!");
		u8g.setFont(u8g_font_5x7);
		u8g.drawStr( 0, 64, __DATE__);
	} while( u8g.nextPage() );

	// Start up delay so the user has time to check everything.
	// Could also be done with a button
	delay(10000); //10 sec

	// Set the initial temperature
	target_temperature = 0.0;  
	// set the integral to 0
	Summation = 0;

	// Setup 1 Hz timer to refresh display using 16 Timer 1
	TCCR1A = 0;										// CTC mode (interrupt after timer reaches OCR1A)
	TCCR1B = _BV(WGM12) | _BV(CS10) | _BV(CS12);	// CTC & clock div 1024
	OCR1A = 15609;									// 16mhz / 1024 / 15609 = 1 Hz
	TIMSK1 = _BV(OCIE1A);							// turn on interrupt
}

 
void loop() { 
	// The LCD and thermocouple are updated in the interrupt

	float MV;    // Manipulated Variable (ie. whether to turn on or off the relay!)
	float Error; // how off we are
	float Slope; // the change per second of the error

	//Get the target temperature from the reflow profile
	target_temperature = profile.getTemp(seconds_time);

	Error = target_temperature - the_temperature;
	Slope = previous_temperature - the_temperature;
	// Summation is done in the interrupt

	// proportional-derivative controller only
	MV = Kp * Error + Ki * Summation + Kd * Slope;

	// Since we just have a relay, we'll decide 1.0 is 'relay on' and less than 1.0 is 'relay off'
	// this is an arbitrary number, we could pick 100 and just multiply the controller values

	if (MV >= 1.0) {
		relay_state = HIGH;
		digitalWrite(RELAYPIN, HIGH);
	} else {
		relay_state = LOW;
		digitalWrite(RELAYPIN, LOW);
	}
}


// This is the Timer 1 CTC interrupt, it goes off once a second
SIGNAL(TIMER1_COMPA_vect) { 
  
	// time moves forward!
	seconds_time++;

	// save the last reading for our slope calculation
	previous_temperature = the_temperature;

	// we will want to know the temperauter in the main loop()
	// instead of constantly reading it, we'll just use this interrupt
	// to track it and save it once a second to 'the_temperature'
	the_temperature = thermocouple.readCelsius();
  
	// Sum the error over time
	Summation += target_temperature - the_temperature;
  
	if ( (the_temperature < (target_temperature * (1.0 - WINDUPPERCENT))) ||
       (the_temperature > (target_temperature * (1.0 + WINDUPPERCENT))) ) {
			// to avoid windup, we only integrate within 5%
			Summation = 0;
	}

	// Display current time and temperatures
	u8g.firstPage();  
	do {
		// time
		u8g.setFont(u8g_font_9x15);
		u8g.drawStr( 0, 10, "Time: ");
		u8g.setPrintPos(64, 10); 
		u8g.print(seconds_time);
		
		// temp
		u8g.drawStr( 0, 30, "Temp: ");
		u8g.setPrintPos(64, 30); 
		u8g.print(the_temperature);
		
		// target
		u8g.drawStr( 0, 50, "Targ: ");
		u8g.setPrintPos(64, 50); 
		u8g.print(target_temperature);
		
	} while( u8g.nextPage() );
  
	// Print out a log so we can see whats up

	Serial.print(seconds_time);				// The current time in seconds
	Serial.print("\t");
	Serial.print(the_temperature);			// The current measured temperature
	Serial.print("\t");
	Serial.print(target_temperature);		// The current target temperature
	Serial.print("\t");
	Serial.print(target_temperature - the_temperature);		// the Error!
	Serial.print("\t");
	Serial.print(previous_temperature - the_temperature);	// the Slope of the Error
	Serial.print("\t");
	Serial.print(Summation); 				// the Integral of Error
	Serial.print("\t");
	Serial.print(Kp*(target_temperature - the_temperature) + Ki*Summation + Kd*(previous_temperature - the_temperature)); //  controller output
	Serial.print("\t");
	Serial.println(relay_state);			// The current state of the relay
  
} 
