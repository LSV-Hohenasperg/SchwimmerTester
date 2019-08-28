/*
   ===========================================================================
                                      SCHWIMMERTESTER

   Description: schwimmertester is a Arduino-Nano-Sketch, to control a vakuum pump,
                two valves and to read a pressure sensor. The board is equipped with
                a Touch-LCD screen.
   Author(s)  : Thorsten Klinkhammer (klinkhammer@gmail.com)
                Markus Kuhnla (markus@kuhnla.de)
   Date       : August, 24th 2019



   List of functions:
          lowPass()           - Smoothes values, read from the pressure sensor
          newMeasurement()    - Reads a new value from the pressure sensor
          tolerance()         - Checks if a value is in the tolerable interval
          readTouchScreen()   - Reads the touchscreen coordinates (user input)
          updateMainScreen()  - Update dynamic elements on main screen
          drawMainScreen()    - Paints all elements for the main screen
          updateEditScreen()  - Update dynamic elements on the number pad screen.
          drawEditScreen()    - Paints all elements for the edit screen (input of numbers)
          processMainScreen() - Checks if user has touched s.th. and reacts accordingly
          processEditScreen() - Checks if user has touched s.th. and reacts accordingly
          processCycles()     - run the climb & descent cycles as often as needed
          setup()             - One time setup at program start
          loop()              - Main program loop


   Hints for nosy people:
      If you want to fiddle with the program, then make some changes
      in the section, which is marked as "PLAYGROUND SECTION" (search for this string).
      Changing values in this section should bring you some fun and should not
      destroy the program logic.

      If you are a really experienced professional, change everything you want :-)
   ===========================================================================
*/


/*+++++++++++++++++++++++++++++
            INCLUDES
  +++++++++++++++++++++++++++++
*/

//Include TFT- and Touchscreen libraries
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
#include <MCUFRIEND_kbv.h>

//Include libc headers
#include <stdio.h>
#include <math.h>


/*+++++++++++++++++++++++++++++
            DEFINES
  +++++++++++++++++++++++++++++
*/

/*
  All digital output pins are connected to optocouples. 
  This is why the logic is inverted. If the digital output is
  low, the logical meaning is "active" and vice versa
 */
#define ON  LOW
#define OFF HIGH

//Pinout used for the touch panel. DO NOT CHANGE THIS!!
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 8   // can be a digital pin
#define XP 9   // can be a digital pin


//Pinout used for sensors and valves. DO NOT CHANGE THIS!!!
#define PIN_PRESSURE    A7
#define PIN_SUCCTION    11   
#define PIN_VENTILATION 10   
#define PIN_PUMP        12   

//Macros for actuator control
#define ACTUATOR_PUMP(newState) {      \
  digitalWrite(PIN_PUMP,newState);     \
  statePump=!newState;                 \
  }

#define ACTUATOR_SUCCTION(newState) {  \
  digitalWrite(PIN_SUCCTION,newState); \
  stateSucctionValve=!newState;        \
  }

#define ACTUATOR_VENTILATION(newState) { \
  digitalWrite(PIN_VENTILATION,newState);\
  stateVentilationValve=!newState;       \
  }

#define CLIMB_FAST {        \
  ACTUATOR_PUMP(ON);        \
  ACTUATOR_SUCCTION(ON);    \
  ACTUATOR_VENTILATION(OFF);\
}

#define CLIMB_SLOW {         \
  ACTUATOR_PUMP(ON);         \
  ACTUATOR_SUCCTION(ON);     \
  ACTUATOR_VENTILATION(ON);  \
  }

#define DESCEND_FAST {      \
  ACTUATOR_PUMP(OFF);       \
  ACTUATOR_SUCCTION(ON);    \
  ACTUATOR_VENTILATION(ON); \
  }

#define DESCEND_SLOW {      \
  ACTUATOR_PUMP(OFF);       \
  ACTUATOR_SUCCTION(ON);    \
  ACTUATOR_VENTILATION(OFF);\
  }

#define HOLD_ALT {          \
  ACTUATOR_PUMP(OFF);       \
  ACTUATOR_SUCCTION(OFF);   \
  ACTUATOR_VENTILATION(OFF);\
  }


//Parameters for the active area of the TouchScreen.
//  Change on your own risk!
#define TS_MINX        75
#define TS_MAXX        930
#define TS_MINY        130
#define TS_MAXY        905
#define TS_MINPRESSURE 10    //Minimum pressure for recognizing a touch event
#define TS_MAXPRESSURE 1000  //Maximum pressure for recognizing a touch event
#define TS_DEBOUNCE_COUNTER 2//Suppress 2 readings after a successful ts reading

//Index for the buttons on main screen
#define BTN_MAIN_GNDTIME     0
#define BTN_MAIN_FLIGHTTIME  1
#define BTN_MAIN_MAX_ALT     2
#define BTN_MAIN_NO_CYCLE    3
#define BTN_MAIN_START       4
#define BTN_MAIN_STOP        5
#define BTN_MAIN_SUCCTION    6
#define BTN_MAIN_VENTILATION 7
#define BTN_MAIN_PUMP        8

//climb and descend cycles are implemented as a state machine.
//here are the valid states
#define CYCLE_CLIMB          0   //Start of machine
#define CYCLE_HOLD_ALTITUDE  1
#define CYCLE_DESCEND        2
#define CYCLE_HOLD_GROUND    3
#define CYCLE_END            4


//Output number with 5 digits right aligned to the screen.
//  Unfortunately avrgcc uses a minimized libc version where
//  length designators in format strings are not allowed to be
//  variable. Because of this, we need to create a format-string
//  by our own, which respects the current length of text fields.
//  (This works but consumes additional cpu time)
#define NUMBER2SCREEN(n,x,y,s) {                    \
    char numBuf[INPUT_LEN+1];			    \
    char fmtString[4];				    \
    sprintf(fmtString,"%%%dd",INPUT_LEN);	    \
    sprintf(numBuf,fmtString,n);		    \
    s.setCursor(x,y);				    \
    s.setTextColor(BLACK, WHITE);                   \
    s.setTextSize(2);				    \
    s.print(numBuf);				    \
  }

#define CONVERT_PRESS2METER(pressure) ((288.15/0.0065) * (1-pow((float) pressure / 1013,(1/5.255))))
#define CONVERT_METER2FEET(meters) (meters * 3.28084)


//Set all variables for using the edit screen
#define SWITCH_TO_EDITSCREEN(headline,value)   { \
    editScreenHeadline=headline;     \
    editPtr=&value;                  \
    drawScreen=drawEditScreen;       \
    updateScreen=updateEditScreen;   \
    processScreen=processEditScreen; \
    drawScreen();                    \
  }

//Set all variables for using the main screen
#define SWITCH_TO_MAINSCREEN {       \
    drawScreen=drawMainScreen;       \
    updateScreen=updateMainScreen;   \
    processScreen=processMainScreen; \
    drawScreen();                    \
  }

/*
   PLAYGROUND SECTION

   change everything you like
*/

/*
  Initial values for adjustable data fields at program startup.
  Be careful to not use values with more than INPUT_LEN digits.
*/
#define DEFAULT_TIME_GROUND  60      //In seconds
#define DEFAULT_TIME_FLIGHT  60      //In seconds
#define DEFAULT_SIM_ALTITUDE 16000   //In feet
#define DEFAULT_NO_CYCLES    10      //Count of climb-descent cycles to do


/*
  The program needs to know the pressure at GND-level. The current implementation
  assumes to be at ground at a specific pressure. In fact, we use a differential
  pressure sensor and if the sensor delivers a pressure of 0, we are on the ground
  and we need to assume the pressure at ground for all calculations. This is why 
  all calculated heights are relative to GND and not to MSL!
*/
#define DEFAULT_GROUND_PRESSURE 1013 //Considered pressure for ground level (0 ft)

/*
  The pressure sensor has jumping readings and needs to be damped somehow.
  This is done using a low pass filter which bypasses only low frequencies.
  If you want to dampen the value more, adjust the value closer to 1. 
  Alowed values are 0.1 to 0.9
*/
#define PRESSURE_FILTERFACTOR   0.7


/*
  The controller tolerates pressures in a range with a specific width.
  For example: If you want to have a pressure of 250 mBar, the controller
  is happy if the current pressure is between (250 - TOLERANCE) and
  (250 + TOLERANCE). If not, it will take actions to adjust the pressure.
  Set the value for the tolerance window below. If the value is small,
  the controller will do a lot of corrections. Be careful with small
  values! 

  Attention: This is mBar and NOT feet!!
 */
#define ALTITUDE_PRESSURE_TOLERANCE 20  


/*
  Timings: 
  The main program runs several tasks periodically.
  you may adjust the timing intervals for each task 
  seperately. At the moment, there are three tasks
  and for each of them you may adjust the timing interval in milliceconds.

    1. Update all values on the screen  --> SCREEN_UPDATE_INTERVAL
    2. Check and process user inputs    --> SCREEN_PROCESS_INTERVAL
    3. Start a new pressure measurement --> MEASUREMENT_INTERVAL
    4. Write protocol data to serial out--> PROTOCOL_INTERVAL

  All numbers are interpreted in milliseconds.
 */
#define SCREEN_PROCESS_INTERVAL   5 
#define SCREEN_UPDATE_INTERVAL    5 
#define MEASUREMENT_INTERVAL     20
#define PROTOCOL_INTERVAL       200

#define INPUT_LEN 5                  //Length of data input field

#define HEADLINE_TIME_GND    "Set: Time GND [sec] "
#define HEADLINE_TIME_FLIGHT "Set: Time FLIGHT [sec] "
#define HEADLINE_MAX_ALT     "Set: Max Altitude [ft] "
#define HEADLINE_NO_CYCLES   "Set: No. of CYC [cyc] "


// Color definitions
#define BLACK       0x0000      /*   0,   0,   0 */
#define NAVY        0x000F      /*   0,   0, 128 */
#define DARKGREEN   0x03E0      /*   0, 128,   0 */
#define DARKCYAN    0x03EF      /*   0, 128, 128 */
#define MAROON      0x7800      /* 128,   0,   0 */
#define PURPLE      0x780F      /* 128,   0, 128 */
#define OLIVE       0x7BE0      /* 128, 128,   0 */
#define LIGHTGREY   0xC618      /* 192, 192, 192 */
#define DARKGREY    0x7BEF      /* 128, 128, 128 */
#define BLUE        0x001F      /*   0,   0, 255 */
#define GREEN       0x07E0      /*   0, 255,   0 */
#define CYAN        0x07FF      /*   0, 255, 255 */
#define RED         0xF800      /* 255,   0,   0 */
#define MAGENTA     0xF81F      /* 255,   0, 255 */
#define YELLOW      0xFFE0      /* 255, 255,   0 */
#define WHITE       0xFFFF      /* 255, 255, 255 */
#define ORANGE      0xFD20      /* 255, 165,   0 */
#define GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define PINK        0xF81F

//Look&Feel of the buttons on the main screen.
#define BUTTON_W          80   //Height of a button
#define BUTTON_H          30   //Width of a button

//Look&Feel of the buttons on the number pad screen
#define BUTTON_SV_X 40
#define BUTTON_SV_Y 90
#define BUTTON_SV_W 65
#define BUTTON_SV_H 65
#define BUTTON_SPACING_X 10
#define BUTTON_SPACING_Y 8
#define BUTTON_TEXTSIZE 2

//Text box at the number pad screen, where the entered numbers go
#define TEXT_X 40
#define TEXT_Y 44
#define TEXT_W 235
#define TEXT_H 40
#define TEXT_TSIZE 3
#define TEXT_TCOLOR BLACK

//Logo at the main screen
#define LOGO_X 0
#define LOGO_Y 0
#define LOGO_W 320
#define LOGO_H 35
#define LOGO_TSIZE 2

char mainButtonLabels[9][7] = {"20", "20", "3000", "50",
                               "Start", "Stop",
                               "Vlv1", "Vlv2", "Pump"
                              };


char editButtonLabels[15][6] = {"Back", "Clear", "OK",
                                "1", "2", "3",
                                "4", "5", "6",
                                "7", "8", "9",
                                "+", "0", "-"
                               };

uint16_t mainButtonColors[9] = {DARKGREY, DARKGREY, DARKGREY, DARKGREY,
                                DARKGREEN, RED,
                                DARKGREY, DARKGREY, DARKGREY
                               };


uint16_t editButtonColors[15] = {RED, DARKGREY, GREEN,
                                 DARKGREY, DARKGREY, DARKGREY,
                                 DARKGREY, DARKGREY, DARKGREY,
                                 DARKGREY, DARKGREY, DARKGREY,
                                 PURPLE, DARKGREY, PURPLE,
                                };

/*
   End of PLAYGROUND SECTION

   If you are not an experienced programmer, change
   nothing beyond this point. You've be warned!!
*/

//Forward declarations
void drawMainScreen(void);
void updateMainScreen(void);
void processMainScreen(void);
void drawEditScreen(void);
void updateEditScreen(void);
void processEditScreen(void);


/*+++++++++++++++++++++++++++++
            GLOBALS
  +++++++++++++++++++++++++++++
*/

MCUFRIEND_kbv tft;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
Adafruit_GFX_Button BTN_Main[9];
Adafruit_GFX_Button BTN_Edit[15];



char     *editScreenHeadline;      //Pointer to the headline for the editScreen. One of HEADLINE_xxx
uint32_t editScreenInput   = 0;

//Three handy pointers to the screen management functions to be in use.
void    (*updateScreen)()  = updateMainScreen;
void    (*drawScreen)()    = drawMainScreen;
void    (*processScreen)() = processMainScreen;

uint8_t  stateCycle = 0;          //Shows wheather we are doing cycles
uint8_t  stateSucctionValve = 0;  //Shows the current state of the sucction valve
uint8_t  stateVentilationValve = 0; //Shows the current state of the ventilation valve
uint8_t  statePump = 0;           //Shows the current state of the pump relais

volatile int32_t touchXPos;       //X-coordinate of touch screen
volatile int32_t touchYPos;       //Y-coordinate of touch screen

//Calculated variables
int32_t flightHeight     = 0;
int32_t altitudeHeightM  = 0;
int32_t altitudeHeightFT = 0;

//Variables for editable fields
uint32_t timeGround  = DEFAULT_TIME_GROUND;
uint32_t timeFlight  = DEFAULT_TIME_FLIGHT;
uint32_t maxAltitude = DEFAULT_SIM_ALTITUDE;
uint32_t noCycles    = DEFAULT_NO_CYCLES;
uint32_t *editPtr;             //Pointer to one editable variable
uint32_t currentCycle = 0;     //Counter for cycles


//Variables for the cycling state machine
uint8_t  cyclingState = CYCLE_CLIMB;
uint32_t cycleMillies = 0;

uint16_t currentPressure  = DEFAULT_GROUND_PRESSURE;//Current pressure value
int      pressureDeduction=0;                       //First deduction of the pressure curve; positive in climb and negative in descend
                                                    //in mBar/s
volatile uint32_t millies = 0;                      //Used to count the timer interrupts
uint8_t touchDebounce     = TS_DEBOUNCE_COUNTER;    //Suppress n following touch events after a successful ts reading
bool eventHappened        =true;                    //Controls wheather a value update on the screen is required
enum {altTolerable,altTooLow,altTooHigh};




/*=========================================================================================

                      F U N C T I O N S

  ==========================================================================================*/

/*
  ===============================================================================
  Function  : lowPass
  Synopsis  : uint16_t lowPass(uint16_t factor, uint16_t measure)
  Discussion: Filters measurements using a low pass filter. The smoothing
            factor is adjustable between 0 and 1. I.e: if you set the factor
            to 0.1, the old median will be used with 10% and the new reading
            has a weight of 90% (very low smoothing).
  Return    : New filtered measurement value (float)
*/
uint16_t lowPass(uint16_t factor, uint16_t measure)
{
  return (currentPressure * factor + measure) / (factor + 1.0);
}

/*
  ===============================================================================
  Function  : newMeasurement
  Synopsis  : void newValue(void)
  Discussion: This function is intended to be called by an interrupt. The
            interrupt should be setup on ValueChange at the measurement pin.
  Return    : void
*/

void newMeasurement(void)
{
  uint16_t oldPressure=currentPressure;
  currentPressure = lowPass(PRESSURE_FILTERFACTOR, min(DEFAULT_GROUND_PRESSURE,
                            DEFAULT_GROUND_PRESSURE - (analogRead(PIN_PRESSURE) / 204.8 - 0.21) * 250));
  pressureDeduction=(pressureDeduction*0.1+(oldPressure-currentPressure)*(1000/ MEASUREMENT_INTERVAL))/1.1;
  altitudeHeightM = CONVERT_PRESS2METER(currentPressure);
  altitudeHeightFT= CONVERT_METER2FEET(altitudeHeightM);
}

/*
  ===============================================================================
  Function  : tolerance
  Synopsis  : int tolerance(unit32_t measurement,unit32_t goal)
  Discussion: Determine weather the current measurement is in the acceptable
            window
  Return    altTooLow    - Masurement is too high (altitude too low)
            altTolerable - Measurement is in tolerance window
            altTooHigh   - Measurement is too low (altitude too high)
*/
int tolerance(unit32_t measurement, unit32_t goal)
{
  unit32_t delta;

  delta = measurement - goal; //delta positive means height not reached (too low)

  if  (abs(delta) <= ALTITUDE_PRESSURE_TOLERANCE)
    return altTolerable;//Pressure tolerable

  if (measurement < goal - ALTITUDE_PRESSURE_TOLERANCE)
    return altTooHigh; //Pressure too low

  return altTooLow;   //Pressure too high
}


/*
  ===============================================================================
  Function  : readTouchScreen
  Synopsis  : void readTouchscreen(void)
  Discussion: Examine the coordinates where the touch screen has been pressed.
            Store the results in the globals touchXpos and touchYpos. Set them to 0
            if no touchScreen event can be detected.
            The Touch screen needs some kind of debouncing. I implement it in a read counter.
            After a successful read, n succeeding reads are suppressed
  Return    : void
*/
void readTouchScreen(void)
{
  touchYPos = 0;
  touchXPos = 0;

  if (touchDebounce) {
    touchDebounce--;
    return;
  }
  
  TSPoint p = ts.getPoint();

  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  if (p.z > TS_MINPRESSURE && p.z < TS_MAXPRESSURE) {
    // scale from 0->1023 to tft.width
    touchYPos = (tft.height() - map(p.x, TS_MINX, TS_MAXX, tft.height(), 0));
    touchXPos = (tft.width() - map(p.y, TS_MAXY, TS_MINY, tft.width(), 0));
    touchDebounce=TS_DEBOUNCE_COUNTER;
  }
}

/*
  ===============================================================================
  Function  : updateMainScreen
  Synopsis  : void updateMainScreen(void)
  Discussion: Update all dynamic elemnts on the main screen without redrawing the
            whole screen.
  Return    : void
*/

void updateMainScreen(void)
{
  //Write all numerical values to the screen
  NUMBER2SCREEN(currentPressure,  188, 50, tft);
  NUMBER2SCREEN(altitudeHeightM,  188, 70, tft);
  NUMBER2SCREEN(altitudeHeightFT, 188, 90, tft);
  NUMBER2SCREEN(currentCycle,     188, 110, tft);

  if (!eventHappened)
    return;
       
  BTN_Main[BTN_MAIN_START].drawButton(stateCycle);
  BTN_Main[BTN_MAIN_STOP].drawButton(!stateCycle);
  BTN_Main[BTN_MAIN_SUCCTION].drawButton(stateSucctionValve);
  BTN_Main[BTN_MAIN_VENTILATION].drawButton(stateVentilationValve);
  BTN_Main[BTN_MAIN_PUMP].drawButton(statePump);
  eventHappened=false;

}

/*
  ===============================================================================
  Function  : drawMainScreen
  Synopsis  : void drawMainScreen(void)
  Discussion: Draw all elements to the main screen. Take care of some status variables
            and draw elements disabled if required.
  Return    : void
*/
void drawMainScreen(void)
{

  tft.fillScreen(WHITE);
  tft.fillRect(LOGO_X, LOGO_Y, LOGO_W, LOGO_H, LIGHTGREY);
  tft.setCursor(LOGO_X + 35, LOGO_Y + 10);
  tft.setTextColor(BLACK);
  tft.setTextSize(LOGO_TSIZE);
  tft.print("Float Gauge Test Unit");
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.setCursor(9, 50);
  tft.print("Actual Pres.:        mbar");
  tft.setCursor(9, 70);
  tft.print("Comp. Alt.:          m");
  tft.setCursor(9, 90);
  tft.print("Comp. Alt.:          ft");
  tft.setCursor(9, 110);
  tft.print("Actual Cycle:        cyc");
  tft.setCursor(9, 150);
  tft.print("Time GND [sec]");
  tft.setCursor(9, 200);
  tft.print("Time FLIGHT [sec]");
  tft.setCursor(9, 250);
  tft.print("Max Altitude [ft]");
  tft.setCursor(9, 300);
  tft.print("No. of CYC [cyc]");

  tft.fillRect(0, 445, 320, 35, NAVY);
  tft.setCursor(LOGO_X + 30, 445 + 10);
  tft.setTextColor(WHITE);
  tft.setTextSize(LOGO_TSIZE);
  tft.print("by LSV-Hohenasperg e.V.");

  //Update the array for the labels, according to current values
  sprintf(mainButtonLabels[BTN_MAIN_GNDTIME]   , "%6d", timeGround);
  sprintf(mainButtonLabels[BTN_MAIN_FLIGHTTIME], "%6d", timeFlight);
  sprintf(mainButtonLabels[BTN_MAIN_MAX_ALT]   , "%6d", maxAltitude);
  sprintf(mainButtonLabels[BTN_MAIN_NO_CYCLE]  , "%6d", noCycles);

  // create buttons (clickable things)

  //Create & draw buttons with dynamic labels
  BTN_Main[BTN_MAIN_GNDTIME].initButton(&tft, 275, 155, BUTTON_W, BUTTON_H, BLACK,
                                        mainButtonColors[BTN_MAIN_GNDTIME], WHITE,
                                        mainButtonLabels[BTN_MAIN_GNDTIME], BUTTON_TEXTSIZE);
  BTN_Main[BTN_MAIN_GNDTIME].drawButton();

  BTN_Main[BTN_MAIN_FLIGHTTIME].initButton(&tft, 275, 205, BUTTON_W, BUTTON_H, BLACK,
      mainButtonColors[BTN_MAIN_FLIGHTTIME], WHITE,
      mainButtonLabels[BTN_MAIN_FLIGHTTIME], BUTTON_TEXTSIZE);
  BTN_Main[BTN_MAIN_FLIGHTTIME].drawButton();

  BTN_Main[BTN_MAIN_MAX_ALT].initButton(&tft, 275, 255, BUTTON_W, BUTTON_H, BLACK,
                                        mainButtonColors[BTN_MAIN_MAX_ALT], WHITE,
                                        mainButtonLabels[BTN_MAIN_MAX_ALT], BUTTON_TEXTSIZE);
  BTN_Main[BTN_MAIN_MAX_ALT].drawButton();

  BTN_Main[BTN_MAIN_NO_CYCLE].initButton(&tft, 275, 305, BUTTON_W, BUTTON_H, BLACK,
                                         mainButtonColors[BTN_MAIN_NO_CYCLE], WHITE,
                                         mainButtonLabels[BTN_MAIN_NO_CYCLE], BUTTON_TEXTSIZE);
  BTN_Main[BTN_MAIN_NO_CYCLE].drawButton();


  //Static buttons from here on
  BTN_Main[BTN_MAIN_START].initButton(&tft, 85, 355, 130, BUTTON_H, BLACK,
                                      mainButtonColors[BTN_MAIN_START], WHITE,
                                      mainButtonLabels[BTN_MAIN_START], BUTTON_TEXTSIZE);

  BTN_Main[BTN_MAIN_STOP].initButton(&tft, 235, 355, 130, BUTTON_H, BLACK,
                                     mainButtonColors[BTN_MAIN_STOP], WHITE,
                                     mainButtonLabels[BTN_MAIN_STOP], BUTTON_TEXTSIZE);

  BTN_Main[BTN_MAIN_SUCCTION].initButton(&tft, 60, 420, 80, BUTTON_H, BLACK,
                                         mainButtonColors[BTN_MAIN_SUCCTION], WHITE,
                                         mainButtonLabels[BTN_MAIN_SUCCTION], BUTTON_TEXTSIZE);

  BTN_Main[BTN_MAIN_VENTILATION].initButton(&tft, 160, 420, 80, BUTTON_H, BLACK,
      mainButtonColors[BTN_MAIN_VENTILATION], WHITE,
      mainButtonLabels[BTN_MAIN_VENTILATION], BUTTON_TEXTSIZE);

  BTN_Main[BTN_MAIN_PUMP].initButton(&tft, 260, 420, 80, BUTTON_H, BLACK,
                                     mainButtonColors[BTN_MAIN_PUMP], WHITE,
                                     mainButtonLabels[BTN_MAIN_PUMP], BUTTON_TEXTSIZE);



}

/*
  ===============================================================================
  Function  : updateEditScreen
  Synopsis  : void updateEditScreen(void)
  Discussion: Update dynamic values on the number pad screen (editScreen)
  Return    : void
*/
void updateEditScreen(void)
{
  NUMBER2SCREEN(editScreenInput, TEXT_X + 130, TEXT_Y + 10, tft);
}

/*
  ===============================================================================
  Function  : drawEditScreen
  Synopsis  : void drawEditScreen(void)
  Discussion: Draw a number pad with buttons and a headline.
  Return    : void
*/
void drawEditScreen(void)
{
  tft.fillScreen(WHITE);
  tft.fillRect(LOGO_X, LOGO_Y, LOGO_W, LOGO_H, LIGHTGREY);
  tft.setCursor(LOGO_X + 9, LOGO_Y + 10);
  tft.setTextColor(BLACK);
  tft.setTextSize(LOGO_TSIZE);

  tft.print(editScreenHeadline);

  // create buttons (terrible hack to create a 3x3 matrix)
  for (uint8_t row = 0; row < 5; row++) {
    for (uint8_t col = 0; col < 3; col++) {
      BTN_Edit[col + row * 3].initButton(&tft, BUTTON_SV_X + 35 + col * (BUTTON_SV_W + BUTTON_SPACING_X + 10),
                                         BUTTON_SV_Y + 40 + row * (BUTTON_SV_H + BUTTON_SPACING_Y), // x, y, w, h, outline, fill, text
                                         BUTTON_SV_W, BUTTON_SV_H, BLACK, editButtonColors[col + row * 3], WHITE,
                                         editButtonLabels[col + row * 3], BUTTON_TEXTSIZE);
      BTN_Edit[col + row * 3].drawButton();
    }
  }
  // create 'text field'
  tft.drawRect(TEXT_X, TEXT_Y, TEXT_W, TEXT_H, BLACK);

}


/*
  ===============================================================================
  Function  : processMainScreen
  Synopsis  : void processMainScreen(void)
  Discussion: Process touch screen events on the main screen and set state variables
            accordingly.
  Return    : void
*/
void processMainScreen(void)
{
  uint8_t button = 0;

  readTouchScreen(); //Read touch screen coordinates
  // go thru all the buttons, checking if they were pressed
  for (button = 0; button < 9 ; button++)
    if (BTN_Main[button].contains(touchXPos, touchYPos)) {
      eventHappened=true;
      break;
    }

  switch (button) {
    case BTN_MAIN_GNDTIME:
      if (!stateCycle)
        SWITCH_TO_EDITSCREEN(HEADLINE_TIME_GND, timeGround);
      break;

    case BTN_MAIN_FLIGHTTIME:
      if (!stateCycle)
        SWITCH_TO_EDITSCREEN(HEADLINE_TIME_FLIGHT, timeFlight);
      break;

    case BTN_MAIN_MAX_ALT:
      if (!stateCycle)
        SWITCH_TO_EDITSCREEN(HEADLINE_MAX_ALT, maxAltitude);
      break;

    case BTN_MAIN_NO_CYCLE:
      if (!stateCycle)
        SWITCH_TO_EDITSCREEN(HEADLINE_NO_CYCLES, noCycles);
      break;

    case BTN_MAIN_START:
      Serial.println("Millies;timeGround;timeFlight;maxAltitude;noCycles;currentCycle;stateCycle;stateSucctionValve;stateVentilationValve;statePump;currentPressure;pressureDeduction");
      stateCycle = 1;
      break;

    case BTN_MAIN_STOP:
      stateCycle = 0;
      break;

    case BTN_MAIN_SUCCTION:
      if (!stateCycle) {
        if (stateSucctionValve)
	  ACTUATOR_SUCCTION(OFF);
	else
	  ACTUATOR_SUCCTION(ON);
      }
      break;

    case BTN_MAIN_VENTILATION:
      if (!stateCycle) {
        if(stateVentilationValve)
	  ACTUATOR_VENTILATION(OFF);
	else
	  ACTUATOR_VENTILATION(ON);
      }
      break;

    case BTN_MAIN_PUMP:
      if (!stateCycle) {
	if (statePump)
	  ACTUATOR_PUMP(OFF);
	else 
	  ACTUATOR_PUMP(ON);
      }
      break;
  }
}

/*
  ===============================================================================
  Function  : processEditScreen
  Synopsis  : void processMainScreen(void)
  Discussion: Process touch screen events on the main screen and set state variables
            accordingly.
  Return    : void
*/
void processEditScreen(void)
{
  uint8_t button = 0;

  readTouchScreen(); //Read touch screen coordinates

  // go thru all the buttons, checking if they were pressed
  for (button = 0; button < 15; button++) {
    if (BTN_Edit[button].contains(touchXPos, touchYPos)) {
      eventHappened=true;
      break;
    }
  }

  switch (button) {
    case 0: //Back
      SWITCH_TO_MAINSCREEN;
      break;

    case 1: //Clear
      editScreenInput = 0;
      break;

    case 2: //OK
      *editPtr = editScreenInput;
      SWITCH_TO_MAINSCREEN;
      break;

    case 12://+
      editScreenInput++;
      break;

    case 13: //0
      editScreenInput = editScreenInput * 10;
      break;

    case 14: //-
      editScreenInput--;
      break;

    case 15: //No button pressed
      break;

    default: //Other digit
      editScreenInput = editScreenInput * 10 + (button - 2); //Sorry for this hack
      break;
  }
}

/*
  ===============================================================================
  Function  : processCycles
  Synopsis  : void processCycles(void)
  Discussion:
	    PROCESING OF CYCLES

	    Implemented as a state machine. State variable is cyclingState.
	    One cycle is:
	    1. Go to the specified altitude
	    2. Wait timeFlight seconds
	    3. Go to the ground altitude
	    4. Wait timeGround seconds
  Return    : void
*/

void processCycles(void)
{

  unit32_t pressureDest=DEFAULT_GROUND_PRESSURE - (maxAltitude / 27.0);
  int      weAre;
    
  switch (cyclingState) {
    case CYCLE_CLIMB:
      weAre=tolerance(currentPressure, pressureDest);

      //Climb if we are below the destination altitude
      if (weAre == altTooLow)
	CLIMB_FAST;

      //advance state if desired altitude is reached
      else if (weAre == altTolerable)
        cyclingState++;

      cycleMillies = millies;
      break;

    case CYCLE_HOLD_ALTITUDE:
      weAre=tolerance(currentPressure, pressureDest);

      //If weAre tooHigh, open one valve for a smooth descend
      if (weAre == altTooHigh)
	DESCEND_SLOW;
      
      //climb if necessary
      if (weAre == altTooLow)
	CLIMB_FAST;

      //Hold altitude if we are OK
      if (weAre == altTolerable)
	HOLD_ALT;

      //Check if flight time is reached
      if (millies >= cycleMillies + 1000 * timeFlight)
        cyclingState++;

      break;

    case CYCLE_DESCEND:
      weAre=tolerance(currentPressure, DEFAULT_GROUND_PRESSURE);

      //Descend if we are above the destination altitude
      if (weAre == altTooHigh)
	DESCEND_FAST;

      //advance state if ground is reached
      if (weAre == altTolerable)
        cyclingState++;

      cycleMillies = millies;
      break;

    case CYCLE_HOLD_GROUND:

      //Check if ground time is reached
      if (millies >= cycleMillies + 1000 * timeGround)
        cyclingState++;
      break;

    case CYCLE_END:
      currentCycle++;
      if (currentCycle >= noCycles) {
	currentCycle=0;
        stateCycle = 0; //End cycling
	HOLD_ALT;
      }

      cyclingState = CYCLE_CLIMB;
      break;

    default:
      //Error handling
      cyclingState = CYCLE_CLIMB;
      stateCycle = 0;
      break;

  }
}


/*=========================================================================================

                               S E T U P

  ==========================================================================================*/


void setup(void) {
  Serial.begin(9600);

  tft.reset();
  tft.begin(tft.readID());
  tft.setRotation(0);
  tft.fillScreen(BLACK);
  pinMode(13, OUTPUT);
  pinMode(PIN_PRESSURE, INPUT);
  pinMode(PIN_SUCCTION, OUTPUT);
  pinMode(PIN_VENTILATION, OUTPUT);
  pinMode(PIN_PUMP, OUTPUT);
  ACTUATOR_SUCCTION(OFF);
  ACTUATOR_VENTILATION(OFF);
  ACTUATOR_PUMP(OFF);

  //Now setup timer0 to generate an interrupt every 1 ms
  TCCR0B |= (1 << CS01); //Set the prescaler to 8
  TCCR0A = (1 << WGM01); //Set timer mode to Clear Timer on Compare Match (CTC)
  OCR0A = 0x7C;         //The Formula for OCR is: OCR=16MHz/Prescaler*desiredSeconds -1
  TIMSK0 |= (1 << OCIE0A); //Set the interrupt request
  sei();                //Enable interrupt

  //Always start with the main screen
  SWITCH_TO_MAINSCREEN;
}

/*=========================================================================================

                               I N T E R R U P T S

  ==========================================================================================*/


ISR(TIMER0_COMPA_vect) {   //This is the interrupt service routine for timer0
  //Advance the counter for milliseconds
  millies++;
}

/*=========================================================================================

                               M A I N L O O P

  ==========================================================================================*/
void loop()
{

  //Pressure measurement every MEASUREMENT_INTERVAL ms
  if (!(millies % MEASUREMENT_INTERVAL))
    newMeasurement(); //It's time for a new measurement

  if (!(millies % SCREEN_UPDATE_INTERVAL))
    {
      updateScreen();  //It's time to update screen values
    }
    
  if (!(millies % SCREEN_PROCESS_INTERVAL))
    processScreen(); //Time for user input processing

  if (!(millies % PROTOCOL_INTERVAL) && stateCycle) {
    char serialBuffer[128];
    sprintf(serialBuffer,"%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d",
	    Millies,
	    timeGround,
	    timeFlight,
	    maxAltitude,
	    noCycles,
	    currentCycle,
	    stateCycle,
	    stateSucctionValve,
	    stateVentilationValve,
	    statePump,
	    currentPressure,
	    pressureDeduction);
    Serial.println(serialBuffer);

  }
    

  if (stateCycle)
    processCycles();

}
