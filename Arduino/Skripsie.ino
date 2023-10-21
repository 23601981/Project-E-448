#include <LiquidCrystal_I2C.h>
#include "RTC.h"
#include "pwm.h"

#define PI 3.1415926535897932384626433832795

boolean button_flag = false;   // Push button interrupt flag
int row = 0;;           //Keypad row
int column = 0;         //Keypad column
char key;               // Keypad variable
char key_mem[5];        //Keypad array

boolean already_print = false;  
boolean system_stopped = true;
boolean increment_day = false;
boolean print_on_lcd = false;
boolean value_update = false;
boolean lcd_update = false;
boolean once = false;
boolean module_reposition = false;
int menu_state = 0;
int digit_position = 0; 

float temp_latitude;
float temp_longitude;
float temp_ltm;
int temp_hour;
int temp_minute;
int temp_day;
int temp_month;
int temp_start_hour;
int temp_start_minute;
int temp_finish_hour;
int temp_finish_minute;
int temp_tracking_type; 

float latitude;
float longitude;
float ltm;
float hour;
float minute;
float day;
float month;
float start_hour;
float start_minute;
float finish_hour;
float finish_minute;
float tracking_type; // NS: tracking_type = 1;  EW: tracking_type = 2

 
int n;       //Day number of the year
int tracker_positions; //Amount of daily positions of the tracker
float tilt_angles[96]; //Tilt angles of collector
float H[96];  //Hour angle of the sun
float H_Finish; //Daily Start hour angle
float H_Start;  //Daily Finish hour angle
int current_position = -1; //Current position of the tracker
float tilt_limit = 70;

float PV_voltage;
float PV_current;
float PV_power;
float battery_voltage;
float measured_tilt;
float half_deadband = 3;

char Get_Key();
void set_Position(int pos, float h, float m);
void actuator_return();

PwmOut pwm1(D3);
PwmOut pwm2(D5);

LiquidCrystal_I2C lcd(0x27, 16, 2); // Initialize LCD Object 

void setup(){
Serial.begin(115200);

pinMode(D8, INPUT_PULLUP); // Keypad Column 1
pinMode(D7, INPUT_PULLUP); // Keypad Column 2
pinMode(D6, INPUT_PULLUP); // Keypad Column 3
pinMode(D12, OUTPUT);      // Keypad row 1
pinMode(D11, OUTPUT);      // Keypad row 2
pinMode(D10, OUTPUT);      // Keypad row 3
pinMode(D9, OUTPUT);       // Keypad row 4
pinMode(D2, INPUT_PULLUP); // Push button

pinMode(D4, OUTPUT);       //MD13S DIR pin

attachInterrupt(digitalPinToInterrupt(D2), blink, FALLING);

digitalWrite(D12, LOW);
digitalWrite(D11, LOW);
digitalWrite(D10, LOW);
digitalWrite(D9, LOW);
digitalWrite(D4, LOW);

lcd.init();  //LCD setup
lcd.clear();
lcd.backlight();
lcd.blink();
lcd.print("SYSTEM STOPPED");

RTC.begin();
RTCTime startTime(24, Month::SEPTEMBER, 2023, 23, 9, 00, DayOfWeek::SUNDAY, SaveLight::SAVING_TIME_ACTIVE);
RTC.setTime(startTime);

if (!RTC.setPeriodicCallback(rtc_interrupt, Period::N4_TIMES_EVERY_SEC)) {
    Serial.println("ERROR: periodic callback not set");
}

pwm1.begin(50,0.0);
pwm2.begin(1000, 0.0);
pwm1.pulse_perc(0);
pwm2.pulse_perc(0);
  

}
  
void loop(){
  while(button_flag == true){ // Go and stay in keypad enter mode
  if(digitalRead(D8) == LOW || digitalRead(D7) == LOW || digitalRead(D6) == LOW){  //If a new key is pressed
     Get_Key();  //Get the key that is pressed
     delay(250); //Wait for quarter of a second
     
     if(digit_position == 0){ //If no digits have already been entered
        digit_position = 1;   //One digit has been entered
        key_mem[0] = key;
     } 
     else  if(digit_position == 1){ //If 1 digit has already been entered
        if(menu_state == 1 || menu_state == 0 || menu_state == 4 || menu_state == 6 || menu_state == 7 || menu_state == 8 || menu_state == 9){ //If in longitude or latitude "enter value" mode
        digit_position = 2;       //Two digits have been entered
        key_mem[1] = key;
        }
        else if(menu_state == 10 && key == '*'){ 
          digit_position = 0;  //Zero digits have been entered
          if(key_mem[0] == '1'){ 
            menu_state = 0;
          }
          if(key_mem[0] == '2'){ 
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("SYSTEM STOPPED");
            system_stopped = true;
            button_flag = false;
            actuator_return();
            menu_state = 12;
         }
        }
        else if(menu_state == 10){ 
          digit_position = 2;     //Two digits have been entered
          key_mem[1] = key;
        }
        else if(menu_state == 2 && key == '*'){ //If in latitude "enter direction" mode and enter button is pressed
          digit_position = 0;  //Zero digits have been entered
          menu_state = 1;
          if(key_mem[0] == '1'){ 
            Serial.print("Latitude:");
            Serial.println(temp_latitude);
          }
         if(key_mem[0] == '2'){ 
            temp_latitude = -temp_latitude;
            Serial.print("Latitude:");
            Serial.println(temp_latitude);
         }
        }
        else if(menu_state == 2){ //If in latitude "enter direction mode" and another button is pressed
          digit_position = 2;     //Two digits have been entered
          key_mem[1] = key;
        }
        else if(menu_state == 3 && key == '*'){ //If in longitude "enter direction" mode and enter button is pressed
          digit_position = 0;  //Zero digits have been entered
          menu_state = 4;
          if(key_mem[0] == '1'){ 
            Serial.print("Longitude:");
            Serial.println(temp_longitude);
          }
         if(key_mem[0] == '2'){ 
            temp_longitude = -temp_longitude;
            Serial.print("Longitude:");
            Serial.println(temp_longitude);
         }
        }
        else if(menu_state == 3){ //If in longitude "enter direction mode" and another button is pressed
          digit_position = 2;     //Two digits have been entered
          key_mem[1] = key;
        }
        else if(menu_state == 5 && key == '*'){ //If in LTM "enter direction" mode and enter button is pressed
          digit_position = 0;  //Zero digits have been entered
          menu_state = 6;
          if(key_mem[0] == '1'){ 
            Serial.print("LTM:");
            Serial.println(temp_ltm);
          }
         if(key_mem[0] == '2'){ 
            temp_ltm = -temp_ltm;
            Serial.print("LTM:");
            Serial.println(temp_ltm);
         }
        }
        else if(menu_state == 5){ //If in LTM "enter direction mode" and another button is pressed
          digit_position = 2;     //Two digits have been entered
          key_mem[1] = key;
        }
        else if(menu_state == 11 && key == '*'){ 
          digit_position = 0;  //Zero digits have been entered
          menu_state = 12;
          button_flag = false;
          value_update = true;
          lcd.clear();
          if(key_mem[0] == '1'){ 
           temp_tracking_type = 1; //North-South tracking
           Serial.println("North-South tracking");
          }
         if(key_mem[0] == '2'){ 
           temp_tracking_type = 2;  //East-West tracking
           Serial.println("East-West tracking");
         }
        }
        else if(menu_state == 11){
          digit_position = 2;     //Two digits have been entered
          key_mem[1] = key;
        }
     } 
     else if(digit_position == 2){ //If 2 digits have already been entered
        digit_position = 3;         //Three digits have been entered  
        key_mem[2] = key;
     } 
     else  if(digit_position == 3){ //If 3 digits have already been entered
        digit_position = 4;         //Four digits have been entered
        key_mem[3] = key;
     }
     else if(digit_position == 4){ //If 4 digits have already been entered
      if(key == '*' && menu_state == 0){ //If enter button is pressed and in latitude "enter value" mode
        menu_state = 2;
        digit_position = 0;        //Zero digits have been entered
        temp_latitude = (key_mem[0] - 48) * 10 + (key_mem[1] -48) + (key_mem[2] - 48) * 0.1 + (key_mem[3] - 48) * 0.01;
        if(temp_latitude > 90){ 
          menu_state = 0;
        }
      }
       else if(menu_state == 0){ //If another button is pressed and in latitude "enter value" mode
         digit_position = 5;     //
       }
       else if(menu_state == 1 || menu_state == 4){ 
          digit_position = 5;  //Five digits have been entered
          key_mem[4] = key;
       }
       else if(key == '*' && menu_state == 6){ //If enter button is pressed and in time "enter" mode
        menu_state = 7;
        digit_position = 0;        //Zero digits have been entered
        temp_hour = (key_mem[0] - 48) * 10 + (key_mem[1] - 48);
        temp_minute = (key_mem[2] - 48) * 10 + (key_mem[3] - 48);
        if(temp_hour > 23 || temp_minute > 59){ 
          menu_state = 6;
        }
        Serial.print("Hour:");
        Serial.println(temp_hour);
        Serial.print("Minute:");
        Serial.println(temp_minute);
      }
       else if(menu_state == 6){ //If another button is pressed and in time "enter" mode
         digit_position = 5;     //
       }
       else if(key == '*' && menu_state == 7){ //If enter button is pressed and in date "enter" mode
        menu_state = 8;
        digit_position = 0;        //Zero digits have been entered
        temp_day = (key_mem[0] - 48) * 10 + (key_mem[1] - 48);
        temp_month = (key_mem[2] - 48) * 10 + (key_mem[3] - 48);
        
        if((temp_month == 1 || temp_month == 3 || temp_month == 5 || temp_month == 7 || temp_month == 8 || temp_month == 10 || temp_month == 12) && ( temp_day > 31 )){ 
          menu_state = 7;
        }
        else if((temp_month  == 4 || temp_month == 6  || temp_month == 9 || temp_month == 11) && temp_day > 30){ 
          menu_state = 7;
        }
        else if(temp_month == 2 && temp_day > 28 ){ 
          menu_state = 7;
        }
        else if(temp_month > 12 || temp_month == 0 || temp_day == 0){ 
          menu_state = 7;
        }
        Serial.print("Day:");
        Serial.println(temp_day);
        Serial.print("Month:");
        Serial.println(temp_month);
      }
       else if(menu_state == 7){ //If another button is pressed and in date "enter" mode
         digit_position = 5;     //
       }
       else if(key == '*' && menu_state == 8){ //If enter button is pressed and in Start Time "enter" mode
        menu_state = 9;
        digit_position = 0;        //Zero digits have been entered
        temp_start_hour = (key_mem[0] - 48) * 10 + (key_mem[1] - 48);
        temp_start_minute = (key_mem[2] - 48) * 10 + (key_mem[3] - 48);
        
        if(temp_start_hour > 23 || temp_start_minute > 59){ 
          menu_state = 8;
        }
        
        Serial.print("Start Hour:");
        Serial.println(temp_start_hour);
        Serial.print("Start Minute:");
        Serial.println(temp_start_minute);
      }
      else if(menu_state == 8){ //If another button is pressed and in Start Time "enter" mode
        digit_position = 5;     //
      }
     else if(key == '*' && menu_state == 9){ //If enter button is pressed and in Finish Time "enter" mode
        menu_state = 11;
        digit_position = 0;        //Zero digits have been entered
        temp_finish_hour = (key_mem[0] - 48) * 10 + (key_mem[1] - 48);
        temp_finish_minute = (key_mem[2] - 48) * 10 + (key_mem[3] - 48);
        
        if(temp_finish_hour > 23 || temp_finish_minute > 59 || temp_finish_hour <= temp_start_hour || ((temp_finish_hour - temp_start_hour == 1) && temp_finish_minute < temp_start_minute)){ 
          menu_state = 9;
        }
        Serial.print("Finish Hour:");
        Serial.println(temp_finish_hour);
        Serial.print("Finish Minute:");
        Serial.println(temp_finish_minute);
      }
       else if(menu_state == 9){ //If another button is pressed and in Finish Time "enter" mode
         digit_position = 5;     //
       }
     }
      else if(digit_position == 5){ //If 5 digits have already been entered
        if(key == '*' && menu_state == 1){ 
          menu_state = 3;
          digit_position = 0;
          temp_longitude = (key_mem[0] - 48) * 100 + (key_mem[1] -48) * 10 + (key_mem[2] - 48) + (key_mem[3] - 48) * 0.1 + (key_mem[4] - 48) * 0.01;
          if(temp_longitude > 180){ 
            menu_state = 1;
          }
        }
        else if(menu_state == 1){ 
          digit_position = 6; 
        }
        if(key == '*' && menu_state == 4){ 
          menu_state = 5;
          digit_position = 0;
          temp_ltm = (key_mem[0] - 48) * 100 + (key_mem[1] -48) * 10 + (key_mem[2] - 48) + (key_mem[3] - 48) * 0.1 + (key_mem[4] - 48) * 0.01;
          if(temp_ltm > 180){ 
            menu_state = 4;
          }
        }
        else if(menu_state == 4){ 
          digit_position = 6; 
        }
      }
   already_print = false;
  }
  //menu_state = 10: Start or stop "enter" mode
  //menu_state = 0 : Latitude "enter value" mode
  //menu_state = 1 : Longitude "enter value" mode
  //menu_state = 2 : Latitude "enter direction" mode
  //menu_state = 3 : Longitude "enter direction" mode
  //menu_state = 4 : LTM "enter value" mode 
  //menu_state = 5 : LTM "enter direction" mode
  //menu_state = 6 : Time "enter" mode
  //menu_state = 7 : Date "enter" mode
  //menu_state = 8 : Start time "enter" mode
  //menu_state = 9 : Finish time "enter" mode
  //menu_state = 11 : NS or EW tracking
  //This if block controls what the LCD will display
  
  if(menu_state == 10){ //True when in start or stop "enter" mode
    if(digit_position == 0 && already_print == false){
      lcd.clear(); 
      lcd.print("Start: 1 Stop: 2");
      lcd.setCursor(0, 1);
    }
    if(digit_position == 1 && already_print == false){
      if(key_mem[0] == '1' || key_mem[0] == '2'){
        lcd.print(key_mem[0]);
      }
      else{ 
        digit_position = 0;
      }
    }
    if(digit_position == 2 && already_print == false){ 
      if(key == '#'){ 
        lcd.clear(); 
        lcd.print("Start: 1 Stop: 2");
        lcd.setCursor(0, 1);
        digit_position = 0;
      }
      else{ 
        digit_position = 1;
      }
    }
     already_print = true;
  }
  else if(menu_state == 0){ //True when in "latitude value" enter mode
    if(digit_position == 0 && already_print == false){ 
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Latitude:");
      already_print = true;
    }
    else if(digit_position == 1 && already_print == false){ 
       if(key_mem[0] == '#' || key_mem[0] == '*'){ 
        digit_position = 0;
       }
       else{
       lcd.print(key_mem[0]); 
       }
       already_print = true;
    }
    else if(digit_position == 2 && already_print == false){ 
       if(key_mem[1] == '*'){ 
        digit_position = 1;
       }
       else if(key_mem[1] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Latitude:"); 
        digit_position = 0;
        }
       else {
        lcd.print(key_mem[1]); 
        lcd.print(".");
       }   
       already_print = true;
    }
    else if(digit_position == 3 && already_print == false){ 
       if(key_mem[2] == '*'){ 
        digit_position = 2;
       }
       else if(key_mem[2] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Latitude:"); 
        lcd.print(key_mem[0]);
        digit_position = 1;
       }
       else {
        lcd.print(key_mem[2]); 
       }
       already_print = true;
    }
    else if(digit_position == 4 && already_print == false){ 
       if(key_mem[3] == '*'){ 
        digit_position = 3;
       }
       else if(key_mem[3] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Latitude:"); 
        lcd.print(key_mem[0]);
        lcd.print(key_mem[1]);
        lcd.print(".");
        digit_position = 2;
       }
       else{
       lcd.print(key_mem[3]); 
       }
       already_print = true;
    }
    else if(digit_position == 5 && already_print == false){ 
       if(key == '#'){ 
       lcd.clear(); 
       lcd.setCursor(0,0);
       lcd.print("Latitude:"); 
       lcd.print(key_mem[0]);
       lcd.print(key_mem[1]);
       lcd.print(".");
       lcd.print(key_mem[2]);
       digit_position = 3;
       }
       else{ 
        digit_position = 4;
       }
      already_print = true;
    }
  }
  else if(menu_state == 2){ //True when in "latitude direction" enter mode
    if(digit_position == 0 && already_print == false){
      lcd.clear(); 
      lcd.print("N: 1 S: 2");
      lcd.setCursor(0, 1);
    }
    if(digit_position == 1 && already_print == false){
      if(key_mem[0] == '1' || key_mem[0] == '2'){
        lcd.print(key_mem[0]);
      }
      else{ 
        digit_position = 0;
      }
    }
    if(digit_position == 2 && already_print == false){ 
      if(key == '#'){ 
        lcd.clear(); 
        lcd.print("N: 1 S: 2");
        lcd.setCursor(0, 1);
        digit_position = 0;
      }
      else{ 
        digit_position = 1;
      }
    }
     already_print = true;
  }
  else if(menu_state == 1){ //True when in "longitude value" enter mode
    if(digit_position == 0 && already_print == false){ 
      lcd.clear();
      lcd.print("Longitude:");
      already_print = true;
    }
    else if(digit_position == 1 && already_print == false){ 
       if(key_mem[0] == '#' || key_mem[0] == '*'){ 
        digit_position = 0;
       }
       else{
       lcd.print(key_mem[0]); 
       }
       already_print = true;
    }
    else if(digit_position == 2 && already_print == false){ 
       if(key_mem[1] == '*'){ 
        digit_position = 1;
       }
       else if(key_mem[1] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Longitude:"); 
        digit_position = 0;
        }
       else {
        lcd.print(key_mem[1]); 
       }   
       already_print = true;
    }
    else if(digit_position == 3 && already_print == false){ 
       if(key_mem[2] == '*'){ 
        digit_position = 2;
       }
       else if(key_mem[2] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Longitude:"); 
        lcd.print(key_mem[0]);
        digit_position = 1;
       }
       else {
        lcd.print(key_mem[2]); 
        lcd.print(".");
       }
       already_print = true;
    }
    else if(digit_position == 4 && already_print == false){ 
       if(key_mem[3] == '*'){ 
        digit_position = 3;
       }
       else if(key_mem[3] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Longitude:"); 
        lcd.print(key_mem[0]);
        lcd.print(key_mem[1]);
        digit_position = 2;
       }
       else{
       lcd.print(key_mem[3]); 
       }
       already_print = true;
    }
    else if(digit_position == 5 && already_print == false){ 
       if(key_mem[4] == '*'){ 
        digit_position = 4;
       }
       else if(key_mem[4] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Longitude:"); 
        lcd.print(key_mem[0]);
        lcd.print(key_mem[1]);
        lcd.print(key_mem[2]);
        lcd.print(".");
        digit_position = 3;
       }
       else{
       lcd.print(key_mem[4]); 
       }
       already_print = true;
    }
    else if(digit_position == 6 && already_print == false){ 
       if(key == '#'){
       lcd.clear(); 
       lcd.setCursor(0,0);
       lcd.print("Longitude:"); 
       lcd.print(key_mem[0]);
       lcd.print(key_mem[1]);
       lcd.print(key_mem[2]);
       lcd.print(".");
       lcd.print(key_mem[3]);
       digit_position = 4;
       }
       else{ 
        digit_position = 5;
       }
      already_print = true;
    }
  }
  else if(menu_state == 3 || menu_state == 5){ 
    if(digit_position == 0 && already_print == false){
      lcd.clear(); 
      lcd.print("E: 1 W: 2");
      lcd.setCursor(0, 1);
    }
    if(digit_position == 1 && already_print == false){
      if(key_mem[0] == '1' || key_mem[0] == '2'){
        lcd.print(key_mem[0]);
      }
      else{ 
        digit_position = 0;
      }
    }
    if(digit_position == 2 && already_print == false){ 
      if(key == '#'){ 
        lcd.clear(); 
        lcd.print("E: 1 W: 2");
        lcd.setCursor(0, 1);
        digit_position = 0;
      }
      else{ 
        digit_position = 1;
      }
    }
     already_print = true;
  }
  else if(menu_state == 4){ 
    if(digit_position == 0 && already_print == false){ 
      lcd.clear();
      lcd.print("LTM:");
      already_print = true;
    }
    else if(digit_position == 1 && already_print == false){ 
       if(key_mem[0] == '#' || key_mem[0] == '*'){ 
        digit_position = 0;
       }
       else{
       lcd.print(key_mem[0]); 
       }
       already_print = true;
    }
    else if(digit_position == 2 && already_print == false){ 
       if(key_mem[1] == '*'){ 
        digit_position = 1;
       }
       else if(key_mem[1] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("LTM:"); 
        digit_position = 0;
        }
       else {
        lcd.print(key_mem[1]); 
       }   
       already_print = true;
    }
    else if(digit_position == 3 && already_print == false){ 
       if(key_mem[2] == '*'){ 
        digit_position = 2;
       }
       else if(key_mem[2] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("LTM:"); 
        lcd.print(key_mem[0]);
        digit_position = 1;
       }
       else {
        lcd.print(key_mem[2]); 
        lcd.print(".");
       }
       already_print = true;
    }
    else if(digit_position == 4 && already_print == false){ 
       if(key_mem[3] == '*'){ 
        digit_position = 3;
       }
       else if(key_mem[3] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("LTM:"); 
        lcd.print(key_mem[0]);
        lcd.print(key_mem[1]);
        digit_position = 2;
       }
       else{
       lcd.print(key_mem[3]); 
       }
       already_print = true;
    }
    else if(digit_position == 5 && already_print == false){ 
       if(key_mem[4] == '*'){ 
        digit_position = 4;
       }
       else if(key_mem[4] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("LTM:"); 
        lcd.print(key_mem[0]);
        lcd.print(key_mem[1]);
        lcd.print(key_mem[2]);
        lcd.print(".");
        digit_position = 3;
       }
       else{
       lcd.print(key_mem[4]); 
       }
       already_print = true;
    }
    else if(digit_position == 6 && already_print == false){ 
       if(key == '#'){
       lcd.clear(); 
       lcd.setCursor(0,0);
       lcd.print("LTM:"); 
       lcd.print(key_mem[0]);
       lcd.print(key_mem[1]);
       lcd.print(key_mem[2]);
       lcd.print(".");
       lcd.print(key_mem[3]);
       digit_position = 4;
       }
       else{ 
        digit_position = 5;
       }
      already_print = true;
    }
   }
   else if(menu_state == 6){ 
    if(digit_position == 0 && already_print == false){ 
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Time:");
      already_print = true;
    }
    else if(digit_position == 1 && already_print == false){ 
       if(key_mem[0] == '#' || key_mem[0] == '*'){ 
        digit_position = 0;
       }
       else{
       lcd.print(key_mem[0]); 
       }
       already_print = true;
    }
    else if(digit_position == 2 && already_print == false){ 
       if(key_mem[1] == '*'){ 
        digit_position = 1;
       }
       else if(key_mem[1] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Time:"); 
        digit_position = 0;
        }
       else {
        lcd.print(key_mem[1]); 
        lcd.print(":");
       }   
       already_print = true;
    }
    else if(digit_position == 3 && already_print == false){ 
       if(key_mem[2] == '*'){ 
        digit_position = 2;
       }
       else if(key_mem[2] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Time:"); 
        lcd.print(key_mem[0]);
        digit_position = 1;
       }
       else {
        lcd.print(key_mem[2]); 
       }
       already_print = true;
    }
    else if(digit_position == 4 && already_print == false){ 
       if(key_mem[3] == '*'){ 
        digit_position = 3;
       }
       else if(key_mem[3] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Time:"); 
        lcd.print(key_mem[0]);
        lcd.print(key_mem[1]);
        lcd.print(":");
        digit_position = 2;
       }
       else{
       lcd.print(key_mem[3]); 
       }
       already_print = true;
    }
    else if(digit_position == 5 && already_print == false){ 
       if(key == '#'){ 
       lcd.clear(); 
       lcd.setCursor(0,0);
       lcd.print("Time:"); 
       lcd.print(key_mem[0]);
       lcd.print(key_mem[1]);
       lcd.print(":");
       lcd.print(key_mem[2]);
       digit_position = 3;
       }
       else{ 
        digit_position = 4;
       }
      already_print = true;
    }
  }
  else if(menu_state == 7){ 
    if(digit_position == 0 && already_print == false){ 
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Date:");
      already_print = true;
    }
    else if(digit_position == 1 && already_print == false){ 
       if(key_mem[0] == '#' || key_mem[0] == '*'){ 
        digit_position = 0;
       }
       else{
       lcd.print(key_mem[0]); 
       }
       already_print = true;
    }
    else if(digit_position == 2 && already_print == false){ 
       if(key_mem[1] == '*'){ 
        digit_position = 1;
       }
       else if(key_mem[1] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Date:"); 
        digit_position = 0;
        }
       else {
        lcd.print(key_mem[1]); 
        lcd.print("/");
       }   
       already_print = true;
    }
    else if(digit_position == 3 && already_print == false){ 
       if(key_mem[2] == '*'){ 
        digit_position = 2;
       }
       else if(key_mem[2] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Date:"); 
        lcd.print(key_mem[0]);
        digit_position = 1;
       }
       else {
        lcd.print(key_mem[2]); 
       }
       already_print = true;
    }
    else if(digit_position == 4 && already_print == false){ 
       if(key_mem[3] == '*'){ 
        digit_position = 3;
       }
       else if(key_mem[3] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Date:"); 
        lcd.print(key_mem[0]);
        lcd.print(key_mem[1]);
        lcd.print("/");
        digit_position = 2;
       }
       else{
       lcd.print(key_mem[3]); 
       }
       already_print = true;
    }
    else if(digit_position == 5 && already_print == false){ 
       if(key == '#'){ 
       lcd.clear(); 
       lcd.setCursor(0,0);
       lcd.print("Date:"); 
       lcd.print(key_mem[0]);
       lcd.print(key_mem[1]);
       lcd.print("/");
       lcd.print(key_mem[2]);
       digit_position = 3;
       }
       else{ 
        digit_position = 4;
       }
      already_print = true;
    }
   }
   else if(menu_state == 8){ 
    if(digit_position == 0 && already_print == false){ 
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Start Time:");
      lcd.setCursor(0, 1);
      already_print = true;
    }
    else if(digit_position == 1 && already_print == false){ 
       if(key_mem[0] == '#' || key_mem[0] == '*'){ 
        digit_position = 0;
       }
       else{
       lcd.print(key_mem[0]); 
       }
       already_print = true;
    }
    else if(digit_position == 2 && already_print == false){ 
       if(key_mem[1] == '*'){ 
        digit_position = 1;
       }
       else if(key_mem[1] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Start Time:"); 
        lcd.setCursor(0, 1);
        digit_position = 0;
        }
       else {
        lcd.print(key_mem[1]); 
        lcd.print(":");
       }   
       already_print = true;
    }
    else if(digit_position == 3 && already_print == false){ 
       if(key_mem[2] == '*'){ 
        digit_position = 2;
       }
       else if(key_mem[2] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Start Time:"); 
        lcd.setCursor(0, 1);
        lcd.print(key_mem[0]);
        digit_position = 1;
       }
       else {
        lcd.print(key_mem[2]); 
       }
       already_print = true;
    }
    else if(digit_position == 4 && already_print == false){ 
       if(key_mem[3] == '*'){ 
        digit_position = 3;
       }
       else if(key_mem[3] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Start Time:"); 
        lcd.setCursor(0, 1);
        lcd.print(key_mem[0]);
        lcd.print(key_mem[1]);
        lcd.print(":");
        digit_position = 2;
       }
       else{
       lcd.print(key_mem[3]); 
       }
       already_print = true;
    }
    else if(digit_position == 5 && already_print == false){ 
       if(key == '#'){ 
       lcd.clear(); 
       lcd.setCursor(0,0);
       lcd.print("Start Time:"); 
       lcd.setCursor(0, 1);
       lcd.print(key_mem[0]);
       lcd.print(key_mem[1]);
       lcd.print(":");
       lcd.print(key_mem[2]);
       digit_position = 3;
       }
       else{ 
        digit_position = 4;
       }
      already_print = true;
    }
   }
   else if(menu_state == 9){ 
    if(digit_position == 0 && already_print == false){ 
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Finish Time:");
      lcd.setCursor(0, 1);
      already_print = true;
    }
    else if(digit_position == 1 && already_print == false){ 
       if(key_mem[0] == '#' || key_mem[0] == '*'){ 
        digit_position = 0;
       }
       else{
       lcd.print(key_mem[0]); 
       }
       already_print = true;
    }
    else if(digit_position == 2 && already_print == false){ 
       if(key_mem[1] == '*'){ 
        digit_position = 1;
       }
       else if(key_mem[1] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Finish Time:"); 
        lcd.setCursor(0, 1);
        digit_position = 0;
        }
       else {
        lcd.print(key_mem[1]); 
        lcd.print(":");
       }   
       already_print = true;
    }
    else if(digit_position == 3 && already_print == false){ 
       if(key_mem[2] == '*'){ 
        digit_position = 2;
       }
       else if(key_mem[2] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Finish Time:"); 
        lcd.setCursor(0, 1);
        lcd.print(key_mem[0]);
        digit_position = 1;
       }
       else {
        lcd.print(key_mem[2]); 
       }
       already_print = true;
    }
    else if(digit_position == 4 && already_print == false){ 
       if(key_mem[3] == '*'){ 
        digit_position = 3;
       }
       else if(key_mem[3] == '#'){ 
        lcd.clear(); 
        lcd.setCursor(0,0);
        lcd.print("Finish Time:"); 
        lcd.setCursor(0, 1);
        lcd.print(key_mem[0]);
        lcd.print(key_mem[1]);
        lcd.print(":");
        digit_position = 2;
       }
       else{
       lcd.print(key_mem[3]); 
       }
       already_print = true;
    }
    else if(digit_position == 5 && already_print == false){ 
       if(key == '#'){ 
       lcd.clear(); 
       lcd.setCursor(0,0);
       lcd.print("Finish Time:"); 
       lcd.setCursor(0, 1);
       lcd.print(key_mem[0]);
       lcd.print(key_mem[1]);
       lcd.print(":");
       lcd.print(key_mem[2]);
       digit_position = 3;
       }
       else{ 
        digit_position = 4;
       }
      already_print = true;
    }
   }
   if(menu_state == 11){ 
    if(digit_position == 0 && already_print == false){
      lcd.clear(); 
      lcd.print("NS: 1 EW: 2");
      lcd.setCursor(0, 1);
    }
    if(digit_position == 1 && already_print == false){
      if(key_mem[0] == '1' || key_mem[0] == '2'){
        lcd.print(key_mem[0]);
      }
      else{ 
        digit_position = 0;
      }
    }
    if(digit_position == 2 && already_print == false){ 
      if(key == '#'){ 
        lcd.clear(); 
        lcd.print("NS: 1 EW: 2");
        lcd.setCursor(0, 1);
        digit_position = 0;
      }
      else{ 
        digit_position = 1;
      }
    }
     already_print = true;
   }
   
   if(system_stopped == false){ 
     RTCTime currentTime;
     RTC.getTime(currentTime);
     float current_hour = (float)currentTime.getHour();
     float current_minute = (float)currentTime.getMinutes();
   
     float H_current = ((12 - current_hour) + (0 - current_minute)/60.0) * 15;  //Calculate the current hour angle
  
     if(H_current <= H_Start && H_current > (H_Finish - 3.75)){
       if(H_current <= H[current_position + 1] && (current_position + 1 < tracker_positions)){ 
         set_Position(current_position + 1, current_hour, current_minute);
         current_position++;
       }
       if(once == true){ 
        once = false;
       }
     }
     else if(H_current <= (H_Finish - 3.75)){ 
       if(once == false){
         increment_day = true;
       }
     }
   
   
   if(increment_day == true && once == false){ 
     Serial.println("Done_special");
     if(n + 1 == 366){ 
        n = 1;
     }
     else{
        n = n + 1;
     }
     float delta = 23.45 * sin((PI/180.0)*((360.0/365.0) * (n - 81))) ;
     float beta[96];  //Altitude angles per day
     float phi_s[96]; //Sun azimuth angles per day
     float z;
     float y;
     float cos_theta;
     float H_Solar_Time; //Hour angle in Solar Time
     float E_min = 9.87*sin((PI/180.0)*2*(360.0/364.0)*(n - 81)) - 7.53*cos((PI/180.0)*(360.0/364.0)*(n - 81)) - 1.5*sin((PI/180.0)*(360.0/364.0)*(n - 81));
   
     for(int x = 0; x < tracker_positions; x++){ 
        H_Solar_Time = ((H[x]/15) + (4/60)*(ltm - longitude) - E_min/60) * 15;  //Convert from Civil Time to Solar Time and calculate the hour angle again
        beta[x] = (180.0/PI)*asin(cos((PI/180.0)*latitude)*cos((PI/180.0)*delta)*cos((PI/180.0)*H_Solar_Time) + sin((PI/180.0)*latitude)*sin((PI/180.0)*delta)); //Calculate altitude angles
        phi_s[x] = (180.0/PI)*asin((cos((PI/180.0)*delta)*sin((PI/180.0)*H_Solar_Time))/cos((PI/180.0)*beta[x])) ;  //Calculate azimuth angles of the sun;
        z = cos((PI/180.0)*H_Solar_Time);                                                 
        y = tan((PI/180.0)*delta)/tan((PI/180)*latitude);
        if(y > z){                                       // Test if (tan_delta/tan_L) > cos_H
           if(phi_s[x] > 0){ 
             phi_s[x] = 180 - phi_s[x];
           }  
           else{ 
             phi_s[x] = -180 - phi_s[x];
           }
       }
     }
     if(tracking_type == 1){
       for(int x = 0; x < tracker_positions; x++){  
          cos_theta = sqrt(1 - (cos((PI/180.0)*beta[x])*cos((PI/180.0)*phi_s[x]))*(cos((PI/180.0)*beta[x])*cos((PI/180.0)*phi_s[x])));
          if(!isnan(cos_theta) && (sin((PI/180.0)*beta[x]) <= cos_theta) && cos_theta != 0 && sin((PI/180.0)*beta[x]) >= 0){
             tilt_angles[x] = (180.0/PI)*acos(sin((PI/180.0)*beta[x])/cos_theta);
             if(phi_s[x] > 0){ 
              tilt_angles[x] = -1 * tilt_angles[x];
             }
          }
          else{ 
             tilt_angles[x] = 100;
          }
       }
     } 
     if(tracking_type == 2){
       for(int x = 0; x < tracker_positions; x++){  
         cos_theta = sqrt(1 - (cos((PI/180.0)*beta[x])*sin((PI/180.0)*phi_s[x]))*(cos((PI/180.0)*beta[x])*sin((PI/180.0)*phi_s[x])));
         if(!isnan(cos_theta) && (sin((PI/180.0)*beta[x]) <= cos_theta) && cos_theta != 0 && sin((PI/180.0)*beta[x]) >= 0){
             tilt_angles[x] = (180.0/PI)*acos(sin((PI/180.0)*beta[x])/cos_theta);
             if(phi_s[x] > 90 ||  phi_s[x] < -90){ 
              tilt_angles[x] = -1 * tilt_angles[x];
             }
         }
         else{ 
            tilt_angles[x] = 100;
         }
      }
   }
     current_position = -1;
     increment_day = false;
     once = true;
   }
  }
 }
  if(value_update == true){ 
   system_stopped = false;
   latitude = temp_latitude;
   longitude = temp_longitude;
   ltm = temp_ltm;
   hour = (float)temp_hour;
   minute = (float)temp_minute;
   day = (float)temp_day;
   month = (float)temp_month;
   start_hour = (float)temp_start_hour;
   start_minute = (float)temp_start_minute;
   finish_hour = (float)temp_finish_hour;
   finish_minute = (float)temp_finish_minute;
   tracking_type = (float)temp_tracking_type;

   if(month == 1){ 
    n = day;
    RTCTime startTime(day, Month::JANUARY, 2023, hour, minute, 00, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
    RTC.setTime(startTime);
   }
   if(month == 2){ 
    n = 31 + day; 
    RTCTime startTime(day, Month::FEBRUARY, 2023, hour, minute, 00, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
    RTC.setTime(startTime);
   }
   if(month == 3){ 
    n = 59 + day;
    RTCTime startTime(day, Month::MARCH, 2023, hour, minute, 00, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
    RTC.setTime(startTime);
   }
   if(month == 4){ 
    n = 90 + day;
    RTCTime startTime(day, Month::APRIL, 2023, hour, minute, 00, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
    RTC.setTime(startTime);
   }
   if(month == 5){ 
    n = 120 + day;
    RTCTime startTime(day, Month::MAY, 2023, hour, minute, 00, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
    RTC.setTime(startTime);
   }
   if(month == 6){ 
    n = 151 + day;
    RTCTime startTime(day, Month::JUNE, 2023, hour, minute, 00, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
    RTC.setTime(startTime);
   }
   if(month == 7){ 
    n = 181 + day;
    RTCTime startTime(day, Month::JULY, 2023, hour, minute, 00, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
    RTC.setTime(startTime);
   }
   if(month == 8){ 
    n = 212 + day;
    RTCTime startTime(day, Month::AUGUST, 2023, hour, minute, 00, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
    RTC.setTime(startTime);
   }
   if(month == 9){ 
    n = 243 + day;
    RTCTime startTime(day, Month::SEPTEMBER, 2023, hour, minute, 00, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
    RTC.setTime(startTime);
   }
   if(month == 10){ 
    n = 273 + day;
    RTCTime startTime(day, Month::OCTOBER, 2023, hour, minute, 00, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
    RTC.setTime(startTime);
   }
   if(month == 11){ 
    n = 304 + day;
    RTCTime startTime(day, Month::NOVEMBER, 2023, hour, minute, 00, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
    RTC.setTime(startTime);
   }
   if(month == 12){ 
    n = 334 + day;
    RTCTime startTime(day, Month::DECEMBER, 2023, hour, minute, 00, DayOfWeek::MONDAY, SaveLight::SAVING_TIME_ACTIVE);
    RTC.setTime(startTime);
   }
   
   float delta = 23.45 * sin((PI/180.0)*((360.0/365.0) * (n - 81)));  //Calculate the angle of declination
   float tracking_hours = (finish_hour - start_hour) + (finish_minute - start_minute)/60.0; //Amount of daily hours to track
   tracker_positions = (int)(tracking_hours/0.25); //Calculate amount of positions for every 15 minutes
   H[0] = ((12 - start_hour) + (0 - start_minute)/60.0) * 15; //Calculate the hour angle for the first position
   H_Start = H[0];
   
   for(int x = 1; x < tracker_positions; x++){ 
      H[x] = H[x - 1] - 3.75;     //Calculate all the daily hour angles for each position 
   }
   H_Finish = H[tracker_positions - 1];  //Calculate the hour angle for the final position
   
   float beta[96];  //Altitude angles per day
   float phi_s[96]; //Sun azimuth angles per day
   float z;
   float y;
   float cos_theta;
   float H_Solar_Time; //Hour angle in Solar Time
   float E_min = 9.87*sin((PI/180.0)*2*(360.0/364.0)*(n - 81)) - 7.53*cos((PI/180.0)*(360.0/364.0)*(n - 81)) - 1.5*sin((PI/180.0)*(360.0/364.0)*(n - 81));
   for(int x = 0; x < tracker_positions; x++){ 
     H_Solar_Time = ((H[x]/15.0) + (4.0/60.0)*(ltm - longitude) - E_min/60.0) * 15;  //Convert from Civil Time to Solar Time and calculate the hour angle again
     beta[x] = (180.0/PI)*asin(cos((PI/180.0)*latitude)*cos((PI/180.0)*delta)*cos((PI/180.0)*H_Solar_Time) + sin((PI/180.0)*latitude)*sin((PI/180.0)*delta)); //Calculate altitude angles
     phi_s[x] = (180.0/PI)*asin((cos((PI/180.0)*delta)*sin((PI/180.0)*H_Solar_Time))/cos((PI/180.0)*beta[x])) ;  //Calculate azimuth angles of the sun;
     z = cos((PI/180.0)*H_Solar_Time);                                                 
     y = tan((PI/180.0)*delta)/tan((PI/180.0)*latitude);
     if(y > z){                                       // Test if (tan_delta/tan_L) > cos_H
       if(phi_s[x] > 0){ 
         phi_s[x] = 180 - phi_s[x];
       }
       else{ 
         phi_s[x] = -180 - phi_s[x];
       }
     }
  }
 
 if(tracking_type == 1){
    for(int x = 0; x < tracker_positions; x++){  
       cos_theta = sqrt(1 - (cos((PI/180.0)*beta[x])*cos((PI/180)*phi_s[x]))*(cos((PI/180.0)*beta[x])*cos((PI/180.0)*phi_s[x])));
       if(!isnan(cos_theta) && (sin((PI/180.0)*beta[x]) <= cos_theta) && cos_theta != 0 && sin((PI/180.0)*beta[x]) >= 0){
          tilt_angles[x] = (180.0/PI)*acos(sin((PI/180.0)*beta[x])/cos_theta);
          if(phi_s[x] > 0){ 
              tilt_angles[x] = -1 * tilt_angles[x];
          }
       }
       else{ 
          tilt_angles[x] = 100;
       }
     }
  } 
 if(tracking_type == 2){
    for(int x = 0; x < tracker_positions; x++){  
       cos_theta = sqrt(1 - (cos((PI/180.0)*beta[x])*sin((PI/180.0)*phi_s[x]))*(cos((PI/180.0)*beta[x])*sin((PI/180.0)*phi_s[x])));
       if(!isnan(cos_theta) && (sin((PI/180.0)*beta[x]) <= cos_theta) && cos_theta != 0 && sin((PI/180.0)*beta[x]) >= 0){
          tilt_angles[x] = (180.0/PI)*acos(sin((PI/180)*beta[x])/cos_theta);
          if(phi_s[x] > 90 ||  phi_s[x] < -90){ 
              tilt_angles[x] = -1 * tilt_angles[x];
          }
       }
       else{ 
          tilt_angles[x] = 100;
       }
    }
   } 
   for(int x = 0; x < tracker_positions; x++){ 
    Serial.print("Position:");
    Serial.println(x);
    Serial.print("Beta:");
    Serial.println(beta[x]);
    Serial.print("Phi:");
    Serial.println(phi_s[x]);
    Serial.print("H:");
    Serial.println(H[x]);
    Serial.print("Tilt:");
    Serial.println(tilt_angles[x]);
    Serial.println();
    }
    
    
    float H_current = ((12 - hour) + (0 - minute)/60) * 15;  //Calculate the current hour angle
    boolean motorset = false;
  
    if(H_current <= H_Start && H_current > (H_Finish - 3.75)){
      for(int x = 0; x < tracker_positions; x++){ 
        if(H_current >= H[x] && motorset == false){ 
           Serial.println("ONCE");
           Serial.println(hour);
           Serial.println(minute);
           if(H_current == H[x]){
             set_Position(x, hour, minute);      //Set tracker position
             current_position = x; //Store/track the current position
           }
           else{
             set_Position(x - 1, hour, minute);      //Set tracker position
             current_position = x - 1; //Store/track the current position
           }   
           motorset = true;          //Set it only once
        }
     }
     if(motorset == false && H_current > H_Finish - 3.75){ 
           Serial.println("ONCE2");
           Serial.println(hour);
           Serial.println(minute);
           set_Position(tracker_positions - 1, hour, minute);       //Set tracker position
           current_position = tracker_positions - 1;  //Store/track the current position
     }
    }
    else if(H_current <= (H_Finish - 3.75)){ 
      increment_day = true;
    }
    value_update = false;
  }
  
  if(system_stopped == false){ 
   RTCTime currentTime;
   RTC.getTime(currentTime);
   float current_hour = (float)currentTime.getHour();
   float current_minute = (float)currentTime.getMinutes();
   
   float H_current = ((12 - current_hour) + (0 - current_minute)/60.0) * 15;  //Calculate the current hour angle
  
   if(H_current <= H_Start && H_current > (H_Finish - 3.75)){
     if(H_current <= H[current_position + 1] && (current_position + 1 < tracker_positions)){ 
        set_Position(current_position + 1, current_hour, current_minute);
        current_position++;
     }
     if(once == true){ 
        once = false;
     }
   }
   else if(H_current <= (H_Finish - 3.75)){ 
     if(once == false){
        increment_day = true;
     }
   }
   if(lcd_update == true){
     
     PV_voltage = (analogRead(A0) * (5.0/1023.0)) * (27.0/5.0);
     PV_voltage = 1.04*PV_voltage - 1.1788;
     PV_current = (analogRead(A1) * (5.0/1023.0) - 2.5)/0.066;
     PV_power = PV_voltage * PV_current;
     int temp_PV_power = (int)PV_power;
     char lcd_power[5];
     lcd_power[0] = (temp_PV_power / 100 ) % 10 + 48; 
     lcd_power[1] = (temp_PV_power / 10) % 10 + 48; 
     lcd_power[2] = temp_PV_power % 10 + 48; 

     int temp_decimal = (int)((PV_power - (float)temp_PV_power) * 100);
     
     lcd_power[3] = (temp_decimal / 10) % 10 + 48; 
     lcd_power[4] = temp_decimal % 10 + 48; 
     
     lcd.setCursor(0, 0);
     lcd.print("PV Power:");
     lcd.print(lcd_power[0]);
     lcd.print(lcd_power[1]);
     lcd.print(lcd_power[2]);
     lcd.print(".");
     lcd.print(lcd_power[3]);
     lcd.print(lcd_power[4]);
     lcd.setCursor(0, 1);
     lcd.print("Time:");
     char lcd_time[4];
     lcd_time[0] = (currentTime.getHour() / 10 ) % 10 + 48; 
     lcd_time[1] = currentTime.getHour() % 10 + 48; 
     lcd_time[2] = (currentTime.getMinutes() / 10 ) % 10 + 48; 
     lcd_time[3] = currentTime.getMinutes() % 10 + 48; 
     lcd.print(lcd_time[0]);
     lcd.print(lcd_time[1]);
     lcd.print(":");
     lcd.print(lcd_time[2]);
     lcd.print(lcd_time[3]);
     
     lcd_update = false;
   }
   
   
   
   if(increment_day == true && once == false){ 
     Serial.println("Done");
     if(n + 1 == 366){ 
        n = 1;
     }
     else{
        n = n + 1;
     }
     float delta = 23.45 * sin((PI/180)*((360/365) * (n - 81))) ;
     float beta[96];  //Altitude angles per day
     float phi_s[96]; //Sun azimuth angles per day
     float z;
     float y;
     float cos_theta;
     float H_Solar_Time; //Hour angle in Solar Time
     float E_min = 9.87*sin((PI/180)*2*(360/364)*(n - 81)) - 7.53*cos((PI/180)*(360/364)*(n - 81)) - 1.5*sin((PI/180)*(360/364)*(n - 81));
   
     for(int x = 0; x < tracker_positions; x++){ 
        H_Solar_Time = ((H[x]/15) + (4/60)*(ltm - longitude) - E_min/60) * 15;  //Convert from Civil Time to Solar Time and calculate the hour angle again
        beta[x] = (180.0/PI)*asin(cos((PI/180.0)*latitude)*cos((PI/180.0)*delta)*cos((PI/180.0)*H_Solar_Time) + sin((PI/180.0)*latitude)*sin((PI/180.0)*delta)); //Calculate altitude angles
        phi_s[x] = (180.0/PI)*asin((cos((PI/180.0)*delta)*sin((PI/180.0)*H_Solar_Time))/cos((PI/180.0)*beta[x])) ;  //Calculate azimuth angles of the sun;
        z = cos((PI/180.0)*H_Solar_Time);                                                 
        y = tan((PI/180.0)*delta)/tan((PI/180.0)*latitude);
        if(y > z){                                       // Test if (tan_delta/tan_L) > cos_H
           if(phi_s[x] > 0){ 
             phi_s[x] = 180 - phi_s[x];
           }  
           else{ 
             phi_s[x] = -180 - phi_s[x];
           }
       }
     }
 
    if(tracking_type == 1){
       for(int x = 0; x < tracker_positions; x++){  
          cos_theta = sqrt(1 - (cos((PI/180.0)*beta[x])*cos((PI/180.0)*phi_s[x]))*(cos((PI/180.0)*beta[x])*cos((PI/180.0)*phi_s[x])));
          if(!isnan(cos_theta) && (sin((PI/180.0)*beta[x]) <= cos_theta) && cos_theta != 0 && sin((PI/180.0)*beta[x]) >= 0){
             tilt_angles[x] = (180.0/PI)*acos(sin((PI/180.0)*beta[x])/cos_theta);
             if(phi_s[x] > 0){ 
              tilt_angles[x] = -1 * tilt_angles[x];
             }
          }
          else{ 
             tilt_angles[x] = 100;
          }
       }
    } 
    if(tracking_type == 2){
      for(int x = 0; x < tracker_positions; x++){  
         cos_theta = sqrt(1 - (cos((PI/180.0)*beta[x])*sin((PI/180.0)*phi_s[x]))*(cos((PI/180.0)*beta[x])*sin((PI/180.0)*phi_s[x])));
         if(!isnan(cos_theta) && (sin((PI/180.0)*beta[x]) <= cos_theta) && cos_theta != 0 && sin((PI/180.0)*beta[x]) >= 0){
             tilt_angles[x] = (180.0/PI)*acos(sin((PI/180.0)*beta[x])/cos_theta);
             if(phi_s[x] > 90 ||  phi_s[x] < -90){ 
              tilt_angles[x] = -1 * tilt_angles[x];
             }
         }
         else{ 
            tilt_angles[x] = 100;
         }
      }
   }
     current_position = -1;
     increment_day = false;
     once = true;
   }
 }
  if(print_on_lcd == true){ 
       lcd.clear();
     if(system_stopped == true){
       lcd.setCursor(0, 0);
       lcd.print("SYSTEM STOPPED");
     }
     print_on_lcd = false;
  }
 
 }

char Get_Key(){ 
   if(digitalRead(D8) == LOW){ //Determine which column is pressed
      column = 1; 
   }
   else if(digitalRead(D7) == LOW){ 
      column = 2;
   }
   else if(digitalRead(D6) == LOW){ 
      column = 3;
   }
   
   if(column == 1){           //Determine which row is pressed when 1st column pressed
    digitalWrite(D12, HIGH);
    if(digitalRead(D8) == HIGH){ 
     key = '1';
    }  
    digitalWrite(D12, LOW);
    
    digitalWrite(D11, HIGH);
    if(digitalRead(D8) == HIGH){ 
     key = '4';
    }  
    digitalWrite(D11, LOW);
    
    digitalWrite(D10, HIGH);
    if(digitalRead(D8) == HIGH){ 
     key = '7';
    }  
    digitalWrite(D10, LOW);
   
    digitalWrite(D9, HIGH);
    if(digitalRead(D8) == HIGH){ 
     key = '*';
    }  
    digitalWrite(D9, LOW);
   }
   
   else if(column == 2){     //Determine which row is pressed when 2nd column pressed
    digitalWrite(D12, HIGH);
    if(digitalRead(D7) == HIGH){ 
     key = '2';
    }  
    digitalWrite(D12, LOW);
    
    digitalWrite(D11, HIGH);
    if(digitalRead(D7) == HIGH){ 
     key = '5';
    }  
    digitalWrite(D11, LOW);
    
    digitalWrite(D10, HIGH);
    if(digitalRead(D7) == HIGH){ 
     key = '8';
    }  
    digitalWrite(D10, LOW);
   
    digitalWrite(D9, HIGH);
    if(digitalRead(D7) == HIGH){ 
     key = '0';
    }  
    digitalWrite(D9, LOW);
   }
    else if(column == 3){     //Determine which row is pressed when 3rd column pressed
    digitalWrite(D12, HIGH);
    if(digitalRead(D6) == HIGH){ 
     key = '3';
    }  
    digitalWrite(D12, LOW);
    
    digitalWrite(D11, HIGH);
    if(digitalRead(D6) == HIGH){ 
     key = '6';
    }  
    digitalWrite(D11, LOW);
    
    digitalWrite(D10, HIGH);
    if(digitalRead(D6) == HIGH){ 
     key = '9';
    }  
    digitalWrite(D10, LOW);
   
    digitalWrite(D9, HIGH);
    if(digitalRead(D6) == HIGH){ 
     key = '#';
    }  
    digitalWrite(D9, LOW);
   }
}

void set_Position(int pos, float h, float m){ 
  Serial.println("POSITION CHANGE:");
  Serial.print("Day:"); 
  Serial.println(n); 
  Serial.print("Position:"); 
  Serial.println(pos); 
  Serial.print("Time:");
  Serial.print((int)h);
  Serial.print(":");
  Serial.println((int)m);
  Serial.print("Tilt:");
  Serial.println(tilt_angles[pos]);
  Serial.print("Hours before solar noon:"); 
  Serial.println(H[pos]/15);

  battery_voltage = (analogRead(A3) * (5.0 / 1023.0)) * (27.0 / 5.0);
  battery_voltage = 1.197 * battery_voltage - 1.219;
  Serial.print("Battery voltage:"); 
  Serial.println(battery_voltage);
 /* float buck_duty_cycle = (12.0 / battery_voltage) * 100; 
  pwm1.pulse_perc(buck_duty_cycle); 
  pwm2.pulse_perc(50);
  
  if(tilt_angles[pos] <= tilt_limit && tilt_angles[pos] >= -tilt_limit){
    module_reposition = true;
  } 
  
  while(module_reposition == true){*/
    int sensor_value = analogRead(A2);
    float tilt_voltage = sensor_value * (5.0 / 1023.0);
    if(tilt_voltage < 0.74){
      measured_tilt = 117.65*tilt_voltage - 157.06;
    }
    else if((tilt_voltage < 1.09) && (tilt_voltage >= 0.74)){
      measured_tilt = 57.14*tilt_voltage - 112.28;
    }
    else if((tilt_voltage < 4.31) && (tilt_voltage >= 1.09)){
      measured_tilt = 31.06*tilt_voltage - 83.86;
    }
    else if((tilt_voltage < 4.67) && (tilt_voltage >= 4.31)){
      measured_tilt = 55.55*tilt_voltage - 189.42;
    }
    else if(tilt_voltage >= 4.67){ 
      measured_tilt = 153.85*tilt_voltage - 648.48; 
    }
      measured_tilt = round(measured_tilt);
      Serial.print("Measured tilt:"); 
      Serial.println(measured_tilt); 
      Serial.println();
    /*if((measured_tilt <= tilt_angles[pos] + half_deadband) && (measured_tilt >= tilt_angles[pos] - half_deadband)){ 
       pwm2.pulse_perc(0);
       pwm1.pulse_perc(0);
       module_reposition = false;
    }
    else if(measured_tilt < tilt_angles[pos] - half_deadband){ 
       digitalWrite(D4, LOW);
    }
    else if(measured_tilt > tilt_angles[pos] + half_deadband){ 
       digitalWrite(D4, HIGH);
    }
  }*/
}

void actuator_return(){ 
    /*battery_voltage = (analogRead(A3) * (5.0 / 1023.0)) * (27.0 / 5.0);
    battery_voltage = 1.197 * battery_voltage - 1.219;
    float buck_duty_cycle = (12.0 / battery_voltage) * 100; 
    pwm1.pulse_perc(buck_duty_cycle); 
    pwm2.pulse_perc(50);
 
    module_reposition = true;
  
    while(module_reposition == true){
       int sensor_value = analogRead(A2);
       float tilt_voltage = sensor_value * (5.0 / 1023.0);
       
       if(tilt_voltage < 0.74){
          measured_tilt = 117.65*tilt_voltage - 157.06;
       }
       else if((tilt_voltage < 1.09) && (tilt_voltage >= 0.74)){
          measured_tilt = 57.14*tilt_voltage - 112.28;
       }
       else if((tilt_voltage < 4.31) && (tilt_voltage >= 1.09)){
         measured_tilt = 31.06*tilt_voltage - 83.86;
       }
       else if((tilt_voltage < 4.67) && (tilt_voltage >= 4.31)){
         measured_tilt = 55.55*tilt_voltage - 189.42;
       }
       else if(tilt_voltage >= 4.67){ 
         measured_tilt = 153.85*tilt_voltage - 648.48; 
       }
       measured_tilt = round(measured_tilt);

       if((measured_tilt <= -tilt_limit + half_deadband) && (measured_tilt >= -tilt_limit - half_deadband)){ 
         pwm2.pulse_perc(0);
         pwm1.pulse_perc(0);
         module_reposition = false;
       }
       else if(measured_tilt < -tilt_limit - half_deadband){ 
         digitalWrite(D4, LOW);
       }
       else if(measured_tilt > -tilt_limit + half_deadband){ 
       digitalWrite(D4, HIGH);
       }
   }*/
}

void blink(){   
   if(button_flag == false){
     button_flag = true;   //Set flag when button is pressed
     menu_state = 10;      // Set menu state
   }
   else{
       button_flag = false;
       menu_state = 12;
       print_on_lcd = true;
       digit_position = 0;
       already_print = false;
   }
}

void rtc_interrupt() {
  //Serial.println("PERIODIC INTERRUPT");
  lcd_update = true;
}
