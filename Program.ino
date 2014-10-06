#include <i2cmaster.h>
#include <LiquidCrystal.h>

// arrow right
byte newChar3[8] = {
  B11000,
  B11100,
  B11110,
  B11111,
  B11110,
  B11100,
  B11000,
  B00000
};
 
// arrow left
byte newChar4[8] = {
  B00011,
  B00111,
  B01111,
  B11111,
  B01111,
  B00111,
  B00011,
  B00000
};

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
String Target = "";
String ToPrint = "";

const int LeftButtonPin = 6;
const int RightButtonPin = 7;
const int OkButtonPin = 8;
int CurrentTarg = 0;

float TempInC = 0;
float MinTemp = 0;
float AvgTemp = 0;
float MaxTemp = 0;
float StoredTemp[10];

boolean IsDebug = true;
boolean ClearScreen = false;
boolean ShowTemp = false;
boolean isSick = false;
boolean Step[10];
boolean ButtonsState[10];

#define ButRight 0
#define ButLeft 1
#define ButOk 2

//Function dependant stuffs
//CheckButtonState
int LastButton;
boolean Act;
//getData
float LastFloatValue;
boolean LastBoolValue;
  
void setup(){
	Serial.begin(9600);
	Serial.println("Setup...");

        for(int i = 1; i < 10; i ++)
          Step[i] = false;
          
        Step[0] = true; //erm
        
        for(int i = 0; i < 10; i ++)
          StoredTemp[i] = 0;
        
        for(int i = 0; i < 10; i ++)
          ButtonsState[i] = 0;
          
	lcd.createChar(2, newChar3);
        lcd.createChar(3, newChar4);
        lcd.begin(16, 2);
        pinMode(LeftButtonPin, INPUT); 
        pinMode(RightButtonPin, INPUT); 
        pinMode(OkButtonPin, INPUT); 
      	i2c_init(); //Initialise the i2c bus
	PORTC = (1 << PORTC4) | (1 << PORTC5);//enable pullups
}

void getTempData()
{
    int dev = 0x5A<<1;
    int data_low = 0;
    int data_high = 0;
    int pec = 0;
    
    i2c_start_wait(dev+I2C_WRITE);
    i2c_write(0x07);
    
    // read
    i2c_rep_start(dev+I2C_READ);
    data_low = i2c_readAck(); //Read 1 byte and then send ack
    data_high = i2c_readAck(); //Read 1 byte and then send ack
    pec = i2c_readNak();
    i2c_stop();
    
    //This converts high and low bytes together and processes temperature, MSB is a error bit and is ignored for temps
    double tempFactor = 0.02; // 0.02 degrees per LSB (measurement resolution of the MLX90614)
    double tempData = 0x0000; // zero out the data
    int frac; // data past the decimal point
    
    // This masks off the error bit of the high byte, then moves it left 8 bits and adds the low byte.
    tempData = (double)(((data_high & 0x007F) << 8) + data_low);
    tempData = (tempData * tempFactor)-0.01;
    
    TempInC = tempData - 273.15;
}

void UpdateScreen()
{
    if(ClearScreen)
       lcd.clear();
       
    lcd.setCursor(0, 0);
    
    lcd.print("Target:  ");
    
    lcd.write(3);
    lcd.print(" ");
    lcd.print(Target);
    lcd.print(" ");
    lcd.write(2);
    
    lcd.setCursor(0, 1);
    if(ShowTemp)
    {
      lcd.print(TempInC);
      lcd.print("C");
    } else 
    {
        lcd.print(ToPrint);
        ToPrint = "";
    }
}

void UpdateDebug()
{
    if(IsDebug)
    {
      Serial.print("Celcius: ");
      Serial.println(TempInC);
      Serial.print("CurrentTarg: ");
      Serial.println(CurrentTarg);
      Serial.print("Target: ");
      Serial.println(Target);
    }
}
void getData()
{ 
  LastFloatValue = CurrentTarg;
  ClearScreen = false;
          
  if(ButtonsState[ButRight])
    CurrentTarg = CurrentTarg + 1;
    
  else if(ButtonsState[ButLeft])
    CurrentTarg = CurrentTarg - 1;
  
  if(CurrentTarg > 1)
  CurrentTarg = 1;
  else if(CurrentTarg < 0)
  CurrentTarg = 0;
  
  if(LastFloatValue != CurrentTarg)
    ClearScreen = true;

  if(CurrentTarg == 0)
    Target = "Humain";
  else if (CurrentTarg == 1)
    Target = "Any";
  
  if(Target == "Any")
  {
    ShowTemp = true;
  } else
  {
    ShowTemp = false;
    if(Target == "Humain")
    {
      MinTemp = 36.5;
      AvgTemp = 37;
      MaxTemp = 38.5;
    } else 
    {
      MinTemp = 0;
      AvgTemp = 50;
      MaxTemp = 100;
    }
    
    //Sickness calculation
    LastBoolValue = isSick;
    if(Step[0])
    {
      ToPrint = "Aim target's head";
      if(ButtonsState[ButOk])
      {
        Step[0] = false;
        Step[1] = true;
        StoredTemp[0] = TempInC;
        ToPrint = "";
      }
    }
    else if(Step[1])
    {
      ToPrint = "Aim target's body";
      if(ButtonsState[ButOk])
      {
        Step[1] = false;
        Step[2] = true;
        StoredTemp[1] = TempInC;
        ToPrint = "";
      }
    }
    else if(Step[2])
    {
      ToPrint = "Aim target's hand";
      if(ButtonsState[ButOk])
      {
        Step[2] = false;
        Step[3] = true;
        StoredTemp[2] = TempInC;
        ToPrint = "";
        ClearScreen = true;
      }
    }
    else if(Step[3])
    {
      if(((StoredTemp[0] + StoredTemp[1] + StoredTemp[2])/3) < MinTemp)
      {
        isSick = true;
        ToPrint = "Warning";
      }
      else if (((StoredTemp[0] + StoredTemp[1] + StoredTemp[2])/3) > MaxTemp)
      {
        isSick = true;
        ToPrint = "Warning";
      }
      else
      {
        isSick = false;
        ToPrint = "Safe";
      }
      
      if(ButtonsState[ButOk])
      {
        Step[3] = false;
        Step[0] = true;
        ToPrint = "";
        ClearScreen = true;
        //StoredTemp[2] = TempInC;
      }
    }
    
    if(LastBoolValue != isSick)
      ClearScreen = true;
  }

}

void getButtonState()
{
  if(digitalRead(OkButtonPin) == HIGH)
    ButtonsState[ButOk] = true;
  else
    ButtonsState[ButOk] = false;
    
  if(digitalRead(LeftButtonPin) == HIGH)
    ButtonsState[ButLeft] = true;
  else
    ButtonsState[ButLeft] = false;
    
  if(digitalRead(RightButtonPin) == HIGH)
    ButtonsState[ButRight] = true;
  else
    ButtonsState[ButRight] = false;
}

void CheckButtonState()
{ 
  if(Act)
  {
    if(LastButton == ButRight)
    {
      if(digitalRead(RightButtonPin) == HIGH)
        ButtonsState[ButRight] = false;
      else 
        Act = false;
    }
    else if(LastButton == ButLeft)
    {
      if(digitalRead(LeftButtonPin) == HIGH)
        ButtonsState[ButLeft] = false;
      else 
        Act = false;
    }    
    else if(LastButton == ButOk)
    {
      if(digitalRead(OkButtonPin) == HIGH)
        ButtonsState[ButOk] = false;
      else 
        Act = false;
    }
  }
  else
  {
    for(int i = 0; i < 10; i ++)
    {
      if(ButtonsState[i] == true)
      {
        LastButton = i;
        Act = true;
      }
    }
  }
}

void loop(){
    getButtonState();
    CheckButtonState();
    getTempData();
    getData();
    UpdateScreen();
    UpdateDebug();
    delay(50); // wait a second before printing again
}
