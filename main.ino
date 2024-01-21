
//
/*

Heufeeder Arduino


Simple Multi-Level menu to control a 16 pin relay station and an i2c display. 

GNU GPLv2.0
Simon Duerr
dev@simonduerr.eu


*/
#include "RTC.h"

#include <LiquidCrystal_I2C.h> // Driver Library for the LCD Module

int seconds = 0;

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27,16,2); // Adjust to (0x27,20,4) for 20x4 LCD

int menuIndex = 0;
int subMenuIndex = 0;
int inSubMenu = 0;

int buttonLeft = 0;
int buttonSelect = 0;
int buttonRight = 0;

int currentFeeding = 0;
int feedingActive = 1;

// default feeding slots
// int days[][5][2] = {{{6,0}, {10,30}, {15,0}, {19,30}, {23,59}},{{6,0}, {12,30}, {15,0}, {19,30}, {23,59}} };
int days[5][2] = {{11,01}, {16,00}, {20,30}, {1,01}, {6,10}};

int relay[5] ={2,3,4,5,6};

int queue[0] = {};

int buttonChanged = 1;

int hourDiff;
int minDiff;

int currMin;
int oldMin;

int BtnLeft = A1; //left
int BtnSelect = A2; //middle
int BtnRight = A3; //right

int slotSelected = 0;

int userTime[4] = {0, 0, 0, 0};  // HH:mm format
int timeInputState = 0; // 0: Hour tens, 1: Hour ones, 2: Minute tens, 3: Minute ones

int userDateTime[10] = {0,0,0,0,0,0,0, 0, 0, 0};  // HH:mm format
int timeDateInputState = 0; // dd.mm.YY HH:mm


int lastactive = 0;
int currentlyactive =1;

void setup()
{
  lcd.init();          // Initiate the LCD module
  lcd.backlight();
  Serial.begin(9600);

  
  RTC.begin();
  
  RTCTime startTime(21, Month::JANUARY, 2024, 9, 15, 00, DayOfWeek::SUNDAY, SaveLight::SAVING_TIME_ACTIVE);

  RTC.setTime(startTime);
 
  pinMode(BtnRight, INPUT_PULLUP); //right
  pinMode(BtnSelect, INPUT_PULLUP); //select
  pinMode(BtnLeft, INPUT_PULLUP); //left


  lastactive = startTime.getUnixTime();
  oldMin = 99;

  for (int i = 0; i <5; i++) {
    Serial.println("setting relay"+String(i)+" as output");
      pinMode(relay[i], OUTPUT);
      digitalWrite(relay[i], HIGH);
  }

  pinMode(13,OUTPUT);
  digitalWrite(13,HIGH);
}


void loop()
{ 
  RTCTime currentTime;
  // Get current time from RTC
  RTC.getTime(currentTime);

  currMin = currentTime.getMinutes();
  if (currentTime.getUnixTime()>lastactive+60){
    currentlyactive = 0;
    lcd.noBacklight();
  }else{
    lcd.backlight();
  }
//timediff to next feeding
//check if time is on this day or next
  if (days[currentFeeding][0] >currentTime.getHour() ){
    hourDiff =  days[currentFeeding][0] -currentTime.getHour();
  }else{
    hourDiff = 24-currentTime.getHour() + days[currentFeeding][0];
  }
  if (days[currentFeeding][1] >currentTime.getMinutes() ){
      minDiff =  days[currentFeeding][1] -currentTime.getMinutes();
  }else{
      hourDiff--; 
      minDiff = 60 - currentTime.getMinutes() + days[currentFeeding][1]; 
  }

 //work with feeding index here to cycle through pins
  if (feedingActive == 1){

    //check if there is some hay in the queue and drop the feeding
      int arrayLength = sizeof(queue) / sizeof(queue[0]);
      if (arrayLength>0){
              for (int i = 0; i < arrayLength; i++) {
                       toggleRelay(queue[i]);
                       delay(5000);
              }
            int queue[] = {};
      }
     if (currentTime.getHour() == days[currentFeeding][0] && currentTime.getMinutes() == days[currentFeeding][1]) {
       toggleRelay(currentFeeding);
       currentFeeding++;
       if (currentFeeding >5){
        currentFeeding = 0;
       }
     }
  }else{
    if (currentTime.getHour() == days[currentFeeding][0] && currentTime.getMinutes() == days[currentFeeding][1]) {
      //add to queue  
      int arrayLength = sizeof(queue) / sizeof(queue[0]);
      if (arrayLength>0){
        int oldqueue[arrayLength] = {};
        for (int i = 0; i < arrayLength; i++) {
          oldqueue[i] = queue[i];
        }
        int queue[arrayLength+1];
        for (int i = 0; i < arrayLength; i++) {
          queue[i] = oldqueue[i];
        }
        queue[arrayLength] = currentFeeding;
      }else{
        int queue[]={currentFeeding};
      }
    }
  }
  buttonLeft = digitalRead(BtnLeft);
  if (buttonLeft == 0) {
    if (inSubMenu == 0){
      menuIndex = (menuIndex - 1 + 4) % 4;
    }else if (inSubMenu == 1 && slotSelected == 0 && menuIndex<3){
     subMenuIndex = (subMenuIndex - 1 + 6) % 6;
    }else if (menuIndex == 3 && inSubMenu == 1){
      decrementDateTime();
    }else if (slotSelected == 1) {
    decrementTime();
    }
     delay(100);
     buttonChanged =1;
  }
  buttonSelect= digitalRead(BtnSelect);
  if (buttonSelect == 0) {
    if (currentlyactive==0){
      lastactive = currentTime.getUnixTime();
    }else{
      selectMenuItem();
      buttonChanged =1;
    }
    delay(100); 
  }

  buttonRight = digitalRead(BtnRight);
  if (buttonRight == 0) {    
    if (inSubMenu == 0){
      menuIndex = (menuIndex + 1) % 4;
    }else if (inSubMenu == 1 && slotSelected == 0 && menuIndex<3){
     subMenuIndex = (subMenuIndex + 1 + 6) % 6;
    }else if (menuIndex == 3 && inSubMenu == 1){
      incrementDateTime();
    } else if (slotSelected == 1) {
      incrementTime();
    }
      delay(100);
      buttonChanged =1;
  }
  //update menu if button changes
  if(buttonChanged ==1){
      displayMenu();
      lastactive = currentTime.getUnixTime();
      currentlyactive = 1;
      buttonChanged = 0;
  }
  if (oldMin!=currMin){
    displayMenu();
    oldMin = currMin;
  }
  delay(300);
}

void toggleRelay(int i){
  Serial.println("relay"+String(i));
  digitalWrite(relay[i], LOW); 
  delay(1200);
  digitalWrite(relay[i], HIGH);
}

Month getMonth(int m){
  switch (m) {
  case 1:
  return Month::JANUARY;
  break;
  case 2:
  return Month::FEBRUARY;
  break;
  case 3:
  return Month::MARCH;
  break;
  case 4:
  return Month::APRIL;
  break;
  case 5:
  return Month::MAY;
  break;
  case 6:
  return Month::JUNE;
  break;
  case 7:
  return Month::JULY;
  break;
  case 8:
  return Month::AUGUST;
  break;
  case 9:
  return Month::SEPTEMBER;
  break;
  case 10:
  return Month::OCTOBER;
  break;
  case 11:
  return Month::NOVEMBER;
  break;
  case 12:
  return Month::DECEMBER;
  break;
  }
  return Month::JANUARY;
}

void displayMenu() {
    RTCTime currentTime;
  // Get current time from RTC
  RTC.getTime(currentTime);
  lcd.clear();
  lcd.noBlink();
  lcd.setCursor(0,0);
  if (feedingActive == 1){
    lcd.print("next in "+String(hourDiff)+"h"+String(minDiff)+"min");
  }else{
    lcd.print("Paused");
     lcd.backlight(); 
  }
  lcd.setCursor(0,1);


  switch (menuIndex) {
    case 0:
      char minString[2];
      sprintf(minString, "%02d",currentTime.getMinutes());
      lcd.print(">On/Off "+String(currentTime.getHour())+":"+String(minString));
      break;
    case 1:
      lcd.backlight();
      lcd.clear();
      lcd.setCursor(0,0);
      if (inSubMenu==0){
    	lcd.print(">Feeding settings");
      	lcd.setCursor(0,1);
        lcd.print("Manual open");
      }else{
      
      if (slotSelected == 0){
       lcd.print("Feeding settings");
      	lcd.setCursor(0,1);
    	if (subMenuIndex <5){
        	lcd.print(">Slot "+String(subMenuIndex+1));
      	}else{
        	lcd.print("^");
      	}
      }else{
        lcd.print("Slot "+String(subMenuIndex+1));
      	lcd.setCursor(0,1);
        char timeString[6];
		    sprintf(timeString, "%02d:%02d", days[subMenuIndex][0],days[subMenuIndex][1]);
        char userTimeString[6];
        switch (timeInputState){
        	case 0:
              sprintf(userTimeString, "%01d_:__", userTime[0]);
              lcd.print(String(userTimeString)+" akt."+String(timeString));
          	  lcd.setCursor(0, 1);
  			      lcd.blink();
            break;
          case 1:
              sprintf(userTimeString, "%01d%01d:__", userTime[0], userTime[1]);
              lcd.print(String(userTimeString)+" akt."+String(timeString));
              lcd.setCursor(1, 1);
  			      lcd.blink();
            break;
          case 2:
              sprintf(userTimeString, "%01d%01d:%01d_", userTime[0], userTime[1],userTime[2]);
              lcd.print(String(userTimeString)+" akt."+String(timeString));
              lcd.setCursor(3, 1);
  			      lcd.blink();
            break;
          case 3:
              sprintf(userTimeString, "%01d%01d:%01d%01d", userTime[0],userTime[1],userTime[2],userTime[3]);
              lcd.print(String(userTimeString)+" akt."+String(timeString));
              lcd.setCursor(4, 1);
  			      lcd.blink();
            break;
          
        }    
      }
     
    }
      break;
    case 2:
     lcd.clear();
      lcd.setCursor(0,0);
      if (inSubMenu==0){
    	lcd.print(">Manual open ");
      	lcd.setCursor(0,1);
        lcd.print("Set time");
      }else{
      
      if (slotSelected == 0){
        lcd.print("Manual open");
        lcd.setCursor(0,1);
    	  if (subMenuIndex <5){
        	lcd.print(">Slot "+String(subMenuIndex+1));
      	}else{
        	lcd.print("^");
      	}
        }
    }
    break;
    case 3:
      if (inSubMenu == 0){
        lcd.clear();
        lcd.print("Manual open");
        lcd.setCursor(0, 1);
        lcd.print(">Set time");
      }else if (inSubMenu == 1 ){
         lcd.clear();
      lcd.print("Set time");
      lcd.setCursor(0, 1);
      char userDateTimeString[14];
      switch (timeDateInputState){
        	case 0:
              sprintf(userDateTimeString, "%01d_.__.__ __:__", userDateTime[0]);
              lcd.print(String(userDateTimeString));
          	  lcd.setCursor(0, 1);
  			      lcd.blink();
            break;
          case 1:
               sprintf(userDateTimeString, "%01d%01d.__.__ __:__", userDateTime[0], userDateTime[1]);
              lcd.print(String(userDateTimeString));
          	  lcd.setCursor(1, 1);
  			      lcd.blink();
            break;
          case 2:
              sprintf(userDateTimeString, "%01d%01d.%01d_.__ __:__",userDateTime[0], userDateTime[1], userDateTime[2]);
              lcd.print(String(userDateTimeString));
          	  lcd.setCursor(3, 1);
  			      lcd.blink();
            break;
          case 3:
              sprintf(userDateTimeString, "%01d%01d.%01d%01d.__ __:__",userDateTime[0], userDateTime[1],userDateTime[2], userDateTime[3]);
              lcd.print(String(userDateTimeString));
          	  lcd.setCursor(4, 1);
  			      lcd.blink();
            break;
            case 4:
              sprintf(userDateTimeString, "%01d%01d.%01d%01d.%01d_ __:__", userDateTime[0], userDateTime[1],userDateTime[2], userDateTime[3],userDateTime[4]);
              lcd.print(String(userDateTimeString));
          	  lcd.setCursor(6, 1);
  			      lcd.blink();
            break;
            case 5:
              sprintf(userDateTimeString, "%01d%01d.%01d%01d.%01d%01d __:__",userDateTime[0], userDateTime[1],userDateTime[2], userDateTime[3],userDateTime[4], userDateTime[5]);
              lcd.print(String(userDateTimeString));
          	  lcd.setCursor(7, 1);
  			      lcd.blink();
            break;
            case 6:
              sprintf(userDateTimeString, "%01d%01d.%01d%01d.%01d%01d %01d_:__",userDateTime[0], userDateTime[1],userDateTime[2], userDateTime[3],userDateTime[4], userDateTime[5], userDateTime[6]);
              lcd.print(String(userDateTimeString));
          	  lcd.setCursor(9, 1);
  			      lcd.blink();
            break;
            case 7:
              sprintf(userDateTimeString, "%01d%01d.%01d%01d.%01d%01d %01d%01d:__",userDateTime[0], userDateTime[1],userDateTime[2], userDateTime[3],userDateTime[4], userDateTime[5], userDateTime[6], userDateTime[7]);
              lcd.print(String(userDateTimeString));
          	  lcd.setCursor(10, 1);
  			      lcd.blink();
            break;
            case 8:
              sprintf(userDateTimeString, "%01d%01d.%01d%01d.%01d%01d %01d%01d:%01d_", userDateTime[0], userDateTime[1],userDateTime[2], userDateTime[3],userDateTime[4], userDateTime[5], userDateTime[6], userDateTime[7],userDateTime[8]);
              lcd.print(String(userDateTimeString));
          	  lcd.setCursor(12, 1);
  			      lcd.blink();
            break;
            case 9:
              sprintf(userDateTimeString, "%01d%01d.%01d%01d.%01d%01d %01d%01d:%01d%01d", userDateTime[0], userDateTime[1],userDateTime[2], userDateTime[3],userDateTime[4], userDateTime[5], userDateTime[6], userDateTime[7],userDateTime[8],userDateTime[9]);
              lcd.print(String(userDateTimeString));
          	  lcd.setCursor(13, 1);
  			      lcd.blink();
            break;
        }    
      }
     
    break;
    default:
      break;
  }
}

void selectMenuItem() {
  // Perform actions based on the selected menu item
  switch (menuIndex) {
    case 0:
      // Action for Item 1
      feedingActive = !feedingActive;
      break;
    case 1:
      // Action for Item 2
      if (inSubMenu == 0){
        inSubMenu = 1;
        timeInputState=0;
      }else   if (inSubMenu == 1 && subMenuIndex==5){
        inSubMenu = 0;
        subMenuIndex=0;
      }else if (inSubMenu == 1 && subMenuIndex<=4 && slotSelected ==0){
        slotSelected = 1;        
      }else if (slotSelected == 1 && timeInputState<3){
       timeInputState++;
      }else if(slotSelected == 1 && timeInputState==3){
      	slotSelected = 0;
        timeInputState = 0;
        // save to days
        convertToHoursMinutes(subMenuIndex);
        userTime[0] = 0;
        userTime[1] = 0;
        userTime[2] = 0;
        userTime[3] = 0;  
      }
      break;
    case 2:
      // Action for Item 3
       if (inSubMenu == 0){
        inSubMenu = 1;
        timeInputState=0;
      }else   if (inSubMenu == 1 && subMenuIndex==5){
        inSubMenu = 0;
        subMenuIndex=0;
      }else if (inSubMenu == 1 && subMenuIndex<=4 && slotSelected ==0){
       toggleRelay(subMenuIndex);       
      }
      break;
    case 3:
     if (inSubMenu == 0){
        inSubMenu = 1;
        timeDateInputState=0;
        slotSelected == 1;
      }else if (inSubMenu == 1 && timeDateInputState<9){
        timeDateInputState++;
      }else if(inSubMenu == 1 && timeDateInputState ==9){
        inSubMenu = 0;
        int dd = userDateTime[0] * 10 + userDateTime[1];
        int mm = userDateTime[2] * 10 + userDateTime[3];
        int yy = 2000+userDateTime[4] * 10 + userDateTime[5];
        int hh = userDateTime[6] * 10 + userDateTime[7];
        int min = userDateTime[8] * 10 + userDateTime[9];

        RTCTime startTime(dd,getMonth(mm), yy, hh, min, 00, DayOfWeek::WEDNESDAY, SaveLight::SAVING_TIME_ACTIVE);
        RTC.setTime(startTime);
      }
    break;
    default:
      break;
  }
}


void decrementTime() {
  // Decrement the currently selected digit
  userTime[timeInputState]--;
  
  // Handle rollover for hours and minutes
  if (timeInputState == 0){
    if (userTime[timeInputState] < 0) {
      userTime[timeInputState] = 2;
    }
  }else if (timeInputState == 2){
    if (userTime[timeInputState] < 0) {
        userTime[timeInputState] = 5;
      }
  }else{
    if (userTime[timeInputState] < 0) {
        userTime[timeInputState] = 9;
      }
  }
}

void incrementTime() {
  // Increment the currently selected digit
     userTime[timeInputState]++;
  if (timeInputState == 0){
    if (userTime[timeInputState] > 2) {
      userTime[timeInputState] = 0;
    }
  }else if (timeInputState == 2){
    if (userTime[timeInputState] > 5) {
        userTime[timeInputState] = 0;
      }
  }else{
    if (userTime[timeInputState] > 9) {
        userTime[timeInputState] = 0;
      }
  }
}

void incrementDateTime() {
  // Increment the currently selected digit
     userDateTime[timeDateInputState]++;
  if (timeDateInputState == 0){
    if (userDateTime[timeDateInputState] > 3) {
      userDateTime[timeDateInputState] = 0;
    }
  }else if (timeDateInputState == 2){
    if (userDateTime[timeDateInputState] > 1) {
        userDateTime[timeDateInputState] = 0;
      }
  }else if (timeDateInputState == 6){
      if (userDateTime[timeDateInputState] > 2) {
        userDateTime[timeDateInputState] = 0;
      }
  }else if (timeDateInputState == 8){
      if (userDateTime[timeDateInputState] > 5) {
        userDateTime[timeDateInputState] = 0;
      }
  }else{
    if (userDateTime[timeDateInputState] > 9) {
        userDateTime[timeDateInputState] = 0;
      }
  }
}

void decrementDateTime() {
  // Increment the currently selected digit
     userDateTime[timeDateInputState]--;
  if (timeDateInputState == 0){
    if (userDateTime[timeDateInputState] <0) {
      userDateTime[timeDateInputState] = 3;
    }
  }else if (timeDateInputState == 2){
    if (userDateTime[timeDateInputState] <0) {
        userDateTime[timeDateInputState] = 1;
      }
  }else if (timeDateInputState == 8){
      if (userDateTime[timeDateInputState] <0) {
        userDateTime[timeDateInputState] = 5;
      }
  }else if (timeDateInputState == 6){
      if (userDateTime[timeDateInputState] <0) {
        userDateTime[timeDateInputState] = 2;
      }
  }else{
    if (userDateTime[timeDateInputState] <0) {
        userDateTime[timeDateInputState] = 9;
      }
  }
}

void convertToHoursMinutes(int slot) {
  // Ensure that the userTime array is within the specified ranges
  if (userTime[0] >= 0 && userTime[0] <= 2 &&
      userTime[1] >= 0 && userTime[1] <= 9 &&
      userTime[2] >= 0 && userTime[2] <= 5 &&
      userTime[3] >= 0 && userTime[3] <= 9) {

    // Calculate hours and minutes directly from the userTime array
    days[slot][0] = userTime[0] * 10 + userTime[1];  // hours
    days[slot][1] = userTime[2] * 10 + userTime[3];  // minutes
  } else {
    // Handle invalid userTime array
    Serial.println("Invalid userTime array");
  }
}
