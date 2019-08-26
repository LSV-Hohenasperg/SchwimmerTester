/*
 * ===========================================================================
 *                                    SCHWIMMERTESTER
 * 
 * Description: schwimmertester is a Arduino-Nano-Sketch, to control a vakuum pump,
 *              two valves and to read a pressure sensor. The board is equipped with
 *              a Touch-LCD screen. 
 * Author(s)  : Thorsten Klinkhammer (klinkhammer@gmail.com)
 *              Markus Kuhnla (markus@kuhnla.de)
 * Date       : August, 24th 2019
 * 
 * 
 * 
 * List of functions:
 *        lowPass()           - Smoothes values, read from the pressure sensor
 *        newMeasurement()    - Reads a new value from the pressure sensor
 *        tolerance()         - Checks if a value is in the tolerable interval
 *        climb()             - Sets all digital outputs for a climb (reducing pressure)
 *        descend()           - Sets all digital outputs for a descend (raising pressure)
 *        level()             - Maintains a given pressure
 *        readTouchScreen()   - Reads the touchscreen coordinates (user input)
 *        drawMainScreen()    - Paints all elements for the main screen
 *        drawEditScreen()    - Paints all elements for the edit screen (input of numbers)
 *        processMainScreen() - Checks if user has touched s.th. and reacts accordingly
 *        processEditScreen() - Checks if user has touched s.th. and reacts accordingly
 *        setup()             - One time setup at program start
 *        loop()              - Main program loop
 *        
 *        
 * Hints for nosy people:
 *    If you want to fiddle with the program, then make some changes
 *    in the section, which is marked as "PLAYGROUND SECTION" (search for this string).
 *    Changing values in this section should bring you some fun and should not
 *    destroy the program logic.
 *    
 *    If you are a really experienced professional, then change everything you want :-)
 * ===========================================================================
 */


/*+++++++++++++++++++++++++++++
 *          INCLUDES
 *+++++++++++++++++++++++++++++
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
 *          DEFINES 
 *+++++++++++++++++++++++++++++
 */
#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

//Pinout used for the touch panel
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 8   // can be a digital pin
#define XP 9   // can be a digital pin


//Pinout used for sensors and valves
#define PIN_PRESSURE    A7
#define PIN_SUCCTION    10   //13=D10
#define PIN_VENTILATION 11   //14=D11
#define PIN_PUMP        12   //15=D12

//Parameters for the TouchScreen
#define TS_MINX 75
#define TS_MAXX 930
#define TS_MINY 130
#define TS_MAXY 905
#define MINPRESSURE 10
#define MAXPRESSURE 1000


#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

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

#define BUTTON_X 40
#define BUTTON_Y 90
#define BUTTON_W 80
#define BUTTON_H 30
#define BUTTON_SV_X 40
#define BUTTON_SV_Y 90
#define BUTTON_SV_W 65
#define BUTTON_SV_H 65
#define BUTTON_SPACING_X 10
#define BUTTON_SPACING_Y 8
#define BUTTON_TEXTSIZE 2

// text box where numbers go
#define TEXT_X 40
#define TEXT_Y 44
#define TEXT_W 235
#define TEXT_H 40
#define TEXT_TSIZE 3
#define TEXT_TCOLOR BLACK

// logo and Menu Page 1
#define LOGO_X 0
#define LOGO_Y 0
#define LOGO_W 320
#define LOGO_H 35
#define LOGO_TSIZE 2

//Buttons for Page1
#define BTN_MAIN_GNDTIME     0
#define BTN_MAIN_FLIGHTTIME  1
#define BTN_MAIN_MAX_ALT     2
#define BTN_MAIN_NO_CYCLE    3
#define BTN_MAIN_START       4
#define BTN_MAIN_STOP        5
#define BTN_MAIN_SUCCTION    6
#define BTN_MAIN_VENTILATION 7
#define BTN_MAIN_PUMP        8



//Output number with 4 digits right aligned to the screen.
//  Unfortunately avrgcc uses a minimized libc version where
//  length designators in format strings are not allowed to be
//  variable. Because of this, we need to write a format string
//  with a fixed length. i.e. %05d (Very ugly)
#define NUMBER2SCREEN(n,x,y,s) { char numBuf[INPUT_LEN+1];                                       \
                                 sprintf(numBuf,"%05d",n);                                       \
                                 for (int i=0; i<INPUT_LEN && numBuf[i]=='0';i++) numBuf[i]=' '; \
                                 s.setCursor(x,y);                                               \
                                 s.setTextColor(BLACK, WHITE); s.setTextSize(2);                 \
                                 s.print(numBuf);                                                \
                               }

#define CONVERT_PRESS2METER(pressure) ((288.15/0.0065) * (1-pow((float) pressure / 1013,(1/5.255))))
#define CONVERT_METER2FEET(meters) (meters * 3.28084)

/*
 * PLAYGROUND SECTION
 * 
 * change everything you like
 */

//Initial values at program startup
#define DEFAULT_TIME_GROUND 20;     //In seconds
#define DEFAULT_TIME_FLIGHT 20;     //In seconds
#define DEFAULT_SIM_ALTITUDE 10000; //In feet
#define DEFAULT_NO_CYCLES 10;       //

#define FILTERFACTOR            0.6 //Damp input values by 60% 
#define TOLERANCE               10  //Accept measurements in a window of +- 10 mBar (270 ft)
#define SCREEN_PROCESS_INTERVAL  5  //Interval to process inputs in milliseconds
#define SCREEN_PAINT_INTERVAL   50  //Interval to update the screen in milliseconds

#define INPUT_LEN 5                  //Length of data input field

#define HEADLINE_TIME_GND    "Set: Time GND [sec] "
#define HEADLINE_TIME_FLIGHT "Set: Time FLIGHT [sec] "
#define HEADLINE_MAX_ALT     "Set: Max Altitude [ft] "
#define HEADLINE_NO_CYCLES   "Set: No. of CYC [cyc] "


Adafruit_GFX_Button BTN_Main[9];
char mainButtonLabels[9][7] = {"20","20","3000","50",
                            "Start","Stop",
                            "Vlv1", "Vlv2", "Pump"};


Adafruit_GFX_Button BTN_Edit[15];
char editButtonLabels[15][6] = {"Back","Clear","OK",
                                "1","2","3",
                                "4","5","6",
                                "7","8","9",
                                "+","0","-"};

uint16_t mainButtonColors[9] = {DARKGREY, DARKGREY,DARKGREY,DARKGREY,
                            DARKGREEN, RED,
                            DARKGREY, DARKGREY, DARKGREY};

                            
uint16_t editButtonColors[15] = {RED, DARKGREY,GREEN,
                                 DARKGREY,DARKGREY,DARKGREY,
                                 DARKGREY,DARKGREY,DARKGREY,
                                 DARKGREY,DARKGREY,DARKGREY,
                                 PURPLE,DARKGREY,PURPLE,};

/*
 * End of PLAYGROUND SECTION 
 * 
 * If you are not an experienced programmer, change 
 * nothing beyond this point. You've be warned!!
 */


/*+++++++++++++++++++++++++++++
 *          GLOBALS 
 *+++++++++++++++++++++++++++++
 */

MCUFRIEND_kbv tft;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
char     *editScreenHeadline;      //Pointer to the headline for the editScreen. One of HEADLINE_xxx
uint32_t editScreenInput  = 0;
char     activeScreenPage = 1;
uint8_t  stateCycle=0;           //Shows wheather we are doing cycles
uint8_t  stateSucctionValve=0;    //Shows the current state of the sucction valve
uint8_t  stateVentilationValve=0; //Shows the current state of the ventilation valve
uint8_t  statePump=0;             //Shows the current state of the pump relais

volatile int32_t touchXPos;      //X-coordinate of touch screen
volatile int32_t touchYPos;      //Y-coordinate of touch screen

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
uint32_t currentCycle= 0;      //Counter for cycles


float currentPressure = 1013; //Current pressure value



                            
/*=========================================================================================
 * 
 *                    F U N C T I O N S
 * 
 ==========================================================================================*/

/*
===============================================================================
Function  : lowPass
Synopsis  : float lowPass(float factor, float measure)
Discussion: Filters measurements using a low pass filter. The smoothing
            factor is adjustable between 0 and 1. I.e: if you set the factor
            to 0.1, the old median will be used with 10% and the new reading
            has a weight of 90% (very low smoothing). 
Return    : New filtered measurement value (float)
*/
float lowPass(float factor, float measure)
{
    return (currentPressure*factor + measure) / (factor +1.0);
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
  currentPressure = lowPass(FILTERFACTOR,max(1013,
                                         1013-(analogRead(PIN_PRESSURE)/204.8-0.21)*250));
  altitudeHeightM = CONVERT_PRESS2METER(currentPressure);
  altitudeHeightFT = CONVERT_METER2FEET(altitudeHeightM);
}

/*
===============================================================================
Function  : tolerance
Synopsis  : int tolerance(float measurement,float goal)
Discussion: Determine weather the current measurement is in the acceptable
            window
Return    : positive - Masurement is too high
            0        - Measurement is in tolerance window
            negative - Measurement is too low
*/
int tolerance(float measurement, float goal)
{
  float delta;
  
  delta = measurement - goal;
  
  if  (abs(delta) <= TOLERANCE)
      return 0; //Measurement tolerable

  if (measurement < goal-TOLERANCE)
      return -1; //Measurement too low
  
  return 1; //Measurement too high
}

/*
===============================================================================
Function  : climb
Synopsis  : int climb(float goal)
Discussion: Setup of all valves and relais to make the chamber climb
Return    : 0 - climbing not needed
            1 - climbing required
*/
int climb(float goal)
{
  if (tolerance(currentPressure,goal) != -1) {
      digitalWrite(PIN_PUMP,LOW);        //Switch on pump
      digitalWrite(PIN_SUCCTION,LOW);    //Switch on sucction valve
      digitalWrite(PIN_VENTILATION,LOW); //Switch on ventilation valve
      return 1;
  }
  return 0;
}

/*
===============================================================================
Function  : descend
Synopsis  : int descend(float goal)
Discussion: Setup of all valves and relais to make the chamber descend
Return    : 0 - Descending not needed
            1 - Descend required
*/
int descend(float goal)
{
  if (tolerance(currentPressure,goal) != 1) {
      digitalWrite(PIN_PUMP,HIGH);        //Switch off pump
      digitalWrite(PIN_SUCCTION,HIGH);    //Switch off sucction valve
      digitalWrite(PIN_VENTILATION,LOW);  //Switch on ventilation valve
      return 1;
  }
  return 0;
}

/*
===============================================================================
Function  : level
Synopsis  : void level(float pressure)
Discussion: Maintain the current pressure level
Return    : void
*/
void level(float pressure)
{
  if ( !climb(pressure) && !descend(pressure)) {
      digitalWrite(PIN_PUMP,HIGH);        //Switch off pump
      digitalWrite(PIN_SUCCTION,HIGH);    //Switch off sucction valve
      digitalWrite(PIN_VENTILATION,HIGH); //Switch off ventilation valve
  }
}


/*
===============================================================================
Function  : readTouchScreen
Synopsis  : void readTouchscreen(void)
Discussion: Examine the coordinates where the touch screen has been pressed.
            Store the results in the globals touchXpos and touchYpos. Set them to 0
            if no touchScreen event can be detected.
Return    : void
*/
void readTouchScreen(void) 
{
  touchYPos = 0;
  touchXPos = 0;

  TSPoint p = ts.getPoint();

  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    // scale from 0->1023 to tft.width 
   touchYPos = (tft.height() - map(p.x, TS_MINX, TS_MAXX, tft.height(), 0));
   touchXPos = (tft.width()-map(p.y, TS_MAXY, TS_MINY, tft.width(), 0));
  } 
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
  tft.setCursor(LOGO_X + 35, LOGO_Y+10);
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
  //Write all numerical values to the screen
  NUMBER2SCREEN(currentPressure, 188,50, tft);
  NUMBER2SCREEN(altitudeHeightM, 188,70, tft);
  NUMBER2SCREEN(altitudeHeightFT,188,90, tft);
  NUMBER2SCREEN(currentCycle,    188,110,tft);
   
  tft.fillRect(0, 445, 320, 35, NAVY);
  tft.setCursor(LOGO_X + 30, 445+10);
  tft.setTextColor(WHITE);
  tft.setTextSize(LOGO_TSIZE);
  tft.print("by LSV-Hohenasperg e.V.");
  
  // create buttons (clickable things)
  BTN_Main[BTN_MAIN_GNDTIME].initButton(&tft, 275,155, BUTTON_W, BUTTON_H, BLACK, 
                      mainButtonColors[BTN_MAIN_GNDTIME], WHITE,
                      mainButtonLabels[BTN_MAIN_GNDTIME], BUTTON_TEXTSIZE); 
  BTN_Main[BTN_MAIN_GNDTIME].drawButton();
  NUMBER2SCREEN(timeGround, 277,155, tft);

  BTN_Main[BTN_MAIN_FLIGHTTIME].initButton(&tft, 275,205, BUTTON_W, BUTTON_H, BLACK, 
                      mainButtonColors[BTN_MAIN_FLIGHTTIME], WHITE,
                      mainButtonLabels[BTN_MAIN_FLIGHTTIME], BUTTON_TEXTSIZE); 
  BTN_Main[BTN_MAIN_FLIGHTTIME].drawButton();
  NUMBER2SCREEN(timeFlight, 277,205, tft);
  
  BTN_Main[BTN_MAIN_MAX_ALT].initButton(&tft, 275,255, BUTTON_W, BUTTON_H, BLACK, 
                      mainButtonColors[BTN_MAIN_MAX_ALT], WHITE,
                      mainButtonLabels[BTN_MAIN_MAX_ALT], BUTTON_TEXTSIZE); 
  BTN_Main[BTN_MAIN_MAX_ALT].drawButton();
  NUMBER2SCREEN(maxAltitude, 277,255, tft);
  
  BTN_Main[BTN_MAIN_NO_CYCLE].initButton(&tft, 275,305, BUTTON_W, BUTTON_H, BLACK, 
                      mainButtonColors[BTN_MAIN_NO_CYCLE], WHITE,
                      mainButtonLabels[BTN_MAIN_NO_CYCLE], BUTTON_TEXTSIZE); 
  BTN_Main[BTN_MAIN_NO_CYCLE].drawButton();
  NUMBER2SCREEN(noCycles, 277,305, tft);
 
  BTN_Main[BTN_MAIN_START].initButton(&tft, 85, 355, 130, BUTTON_H, BLACK, 
                      mainButtonColors[BTN_MAIN_START], WHITE,
                      mainButtonLabels[BTN_MAIN_START], BUTTON_TEXTSIZE); 
  BTN_Main[BTN_MAIN_START].drawButton(stateCycle);
  
  BTN_Main[BTN_MAIN_STOP].initButton(&tft, 235,355, 130, BUTTON_H, BLACK, 
                      mainButtonColors[BTN_MAIN_STOP], WHITE,
                      mainButtonLabels[BTN_MAIN_STOP], BUTTON_TEXTSIZE); 
  BTN_Main[BTN_MAIN_STOP].drawButton(!stateCycle);
  
  BTN_Main[BTN_MAIN_SUCCTION].initButton(&tft, 60,420, 80, BUTTON_H, BLACK, 
                      mainButtonColors[BTN_MAIN_SUCCTION], WHITE,
                      mainButtonLabels[BTN_MAIN_SUCCTION], BUTTON_TEXTSIZE); 
  BTN_Main[BTN_MAIN_SUCCTION].drawButton(stateSucctionValve);

  BTN_Main[BTN_MAIN_VENTILATION].initButton(&tft, 160, 420, 80, BUTTON_H, BLACK, 
                      mainButtonColors[BTN_MAIN_VENTILATION], WHITE,
                      mainButtonLabels[BTN_MAIN_VENTILATION], BUTTON_TEXTSIZE); 
  BTN_Main[BTN_MAIN_VENTILATION].drawButton(stateVentilationValve);

  BTN_Main[BTN_MAIN_PUMP].initButton(&tft, 260,420, 80, BUTTON_H, BLACK, 
                      mainButtonColors[BTN_MAIN_PUMP], WHITE,
                      mainButtonLabels[BTN_MAIN_PUMP], BUTTON_TEXTSIZE); 
  BTN_Main[BTN_MAIN_PUMP].drawButton(statePump);


  
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
  tft.setCursor(LOGO_X + 9, LOGO_Y+10);
  tft.setTextColor(BLACK);
  tft.setTextSize(LOGO_TSIZE);

  tft.print(editScreenHeadline);

  // create buttons
  for (uint8_t row=0; row<5; row++) {
    for (uint8_t col=0; col<3; col++) {
      BTN_Edit[col + row*3].initButton(&tft, BUTTON_SV_X+35+col*(BUTTON_SV_W+BUTTON_SPACING_X+10),
                 BUTTON_SV_Y+40+row*(BUTTON_SV_H+BUTTON_SPACING_Y),    // x, y, w, h, outline, fill, text
                  BUTTON_SV_W, BUTTON_SV_H, BLACK, editButtonColors[col+row*3], WHITE,
                  editButtonLabels[col + row*3], BUTTON_TEXTSIZE);
      BTN_Edit[col + row*3].drawButton();
    }
  }
  // create 'text field'
  tft.drawRect(TEXT_X, TEXT_Y, TEXT_W, TEXT_H, BLACK);

  NUMBER2SCREEN(editScreenInput,TEXT_X+130,TEXT_Y+10,tft);
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
  uint8_t button=0;

  readTouchScreen(); //Read touch screen coordinates
  // go thru all the buttons, checking if they were pressed
  for (button=0; button<9 ; button++)
    if (BTN_Main[button].contains(touchXPos,touchYPos)) {
      BTN_Main[button].press(true);
      break;
    }

uint8_t stateCycle=0;
  switch (button){
    case BTN_MAIN_GNDTIME:
              if (!stateCycle) {
                editScreenHeadline=HEADLINE_TIME_GND;
                editPtr=&timeGround;
                activeScreenPage = 2;
              }
              break;
              
    case BTN_MAIN_FLIGHTTIME:
              if (!stateCycle) {
                editScreenHeadline=HEADLINE_TIME_FLIGHT;
                editPtr=&timeFlight;
                activeScreenPage = 2;
              }
              break;
              
    case BTN_MAIN_MAX_ALT:
              if (!stateCycle) {
                editScreenHeadline=HEADLINE_MAX_ALT;
                editPtr=&maxAltitude;
                activeScreenPage = 2;
              }
              break;
              
    case BTN_MAIN_NO_CYCLE:
              if (!stateCycle) {
                editScreenHeadline=HEADLINE_NO_CYCLES;
                editPtr=&noCycles;
                activeScreenPage = 2;
              }
              break;

    case BTN_MAIN_START:
              stateCycle=1;
              break;
              
    case BTN_MAIN_STOP:
              stateCycle=0;
              break;
              
    case BTN_MAIN_SUCCTION:
              if (!stateCycle) {
                stateSucctionValve=1-stateSucctionValve;
                digitalWrite(PIN_SUCCTION,!stateSucctionValve);
              }
              break;
              
    case BTN_MAIN_VENTILATION:
              if (!stateCycle) {
                stateVentilationValve=1-stateVentilationValve;
                digitalWrite(PIN_VENTILATION,!stateVentilationValve);
              }
              break;
              
    case BTN_MAIN_PUMP:
              if (!stateCycle) {
                statePump=1-statePump;
                digitalWrite(PIN_PUMP,!statePump);
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
  uint8_t b=0;
  
  readTouchScreen(); //Read touch screen coordinates

  // go thru all the buttons, checking if they were pressed
  for (uint8_t b=0; b<15; b++) {
    if (BTN_Edit[b].contains(touchXPos,touchYPos)) {
      BTN_Edit[b].press(true);  // tell the button it is pressed
      break;
    }
  }

  for (b=0; b<15; b++) {
    if (BTN_Edit[b].justPressed())
      switch(b){
        case 0: //Back
          activeScreenPage = 1; //Back to the main screen
          break;

        case 1: //Clear
          editScreenInput=0;
          break;
          
        case 2: //OK
          *editPtr=editScreenInput;
          activeScreenPage = 1; //Back to main screen
          break;
        
        case 12://+
          editScreenInput++;
          break;
        
        case 13: //0
          editScreenInput=0;
          break;
        
        case 14: //-
          editScreenInput--;
          break;
        
        default: //Other digit
          editScreenInput=editScreenInput*10+(b-2); //Sorry for this hack
          break;
      }
  }
  
}
/*=========================================================================================
 * 
 *                             S E T U P
 * 
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
  digitalWrite(PIN_SUCCTION,HIGH);
  digitalWrite(PIN_VENTILATION,HIGH);
  digitalWrite(PIN_PUMP,HIGH);

  //Now setup timer0 to generate an interrupt every 1 ms
  TCCR0B|=(1<<CS01);    //Set the prescaler to 8
  TCCR0A=(1<<WGM01);    //Set timer mode to Clear Timer on Compare Match (CTC)   
  OCR0A=0x7C;           //The Formula for OCR is: OCR=16MHz/Prescaler*desiredSeconds -1
  TIMSK0|=(1<<OCIE0A);  //Set the interrupt request
  sei();                //Enable interrupt

}

/*=========================================================================================
 * 
 *                             I N T E R R U P T S
 * 
 ==========================================================================================*/

volatile unsigned int millies=0; //Used to count the timer interrupts
#define MEASUREMENT_INTERVAL 100 //Pressure measurement every 100 ms

ISR(TIMER0_COMPA_vect){    //This is the interrupt service routine for timer0
  //Measure the pressure every N milliseconds
  millies++;
}

/*=========================================================================================
 * 
 *                             M A I N L O O P
 * 
 ==========================================================================================*/

void loop()
{

  //Pressure measurement every MEASUREMENT_INTERVAL ms
  if (millies%MEASUREMENT_INTERVAL == 0)
    newMeasurement(); //It's time for a new measurement

  if (!millies%SCREEN_PAINT_INTERVAL)
    switch(activeScreenPage) {
      case 1:
          drawMainScreen();
          break;
      case 2:
          drawEditScreen();
          break;
      default:
          //Huuuuh this is crazy!
          break;
    }

  if (!millies%SCREEN_PROCESS_INTERVAL)
    switch(activeScreenPage) {
      case 1:
          processMainScreen();
          break;
      case 2:
          processEditScreen();
          break;
      default:
          //Huuuuh this is crazy!
          break;
    }

 

}
