 /*
Smart_Alarm.pde
Jimmy Zhang
9/3/2021

Don't try to understand this program before I sort out this program.
Most time, variables are modified directly in functions.
I didn't used DS3231's alarm, also didn't used SQW.

*/

#include <Wire.h>
#include <DS3231.h>
#include <LiquidCrystal.h>

//base of clock.
DS3231 Clock;
bool Century=false;
bool h12=false;
bool PM=false;//It's unused now, because h12 is false.

//some variable of alarm.
//If first is 255, it isn't enable.
//The ordinary alarm. first mean it's 5 alarm, second is 'DoW, month, date, hour,minute' (0 means it isn't use to match. if DoW isn't 0, use DoW to match).
byte alarm[6][5]={{255},{255},{255},{255},{255}};

//Alarm for sleep, use the 'human body infrared detector 1' (D11), the last one is 1 or 0 for get up or sleep(0 is up). 
byte alarmForBed[6][6]={{255},{255},{255},{255},{255},{255}};

//Alarm for work, use the 'human body infrared detector 2' (D12), the last one is 1 or 0 for begin or stop learning(1 is begin).
byte alarmForWork[6][6]={{255},{255},{255},{255},{255},{255}};

//time and temperature
byte year, month, date, DoW, hour, minute, second;
double temperature;

//Some variable.
bool isTic=false;

//interfaces
byte x1=8;
byte x2=9;
byte x3_8=10;
byte ID1=11;//Infrared 1
byte ID2=12;//Infrared 2
byte soundGND=13;
byte isAlarming=0; //use to the ordinary alarm.
byte whichBedAlarming=0;
byte whichWorkAlarming=0;

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// analog interface:
// 0=set/ok
// 1=plus
// 2=minus
// 3=mode change/next
//
// digital interface:
// 0,1=null
// 2-7=lcd
// 8=x1
// 9=x2
// 10=x3_8
// 11=ID1
// 12=ID2
// 13=GND of DY-SV8F


void setup() {
  // Start the I2C interface
  Wire.begin();
  // Start the serial interface
  Serial.begin(115200);
  // Start the LCD
  lcd.begin(16,2);

  // Set and initialization the alarm pin. 
  pinMode(x1,OUTPUT);
  pinMode(x2,OUTPUT);
  pinMode(x3_8,OUTPUT);
  pinMode(soundGND,OUTPUT);
  pinMode(ID1,INPUT);
  pinMode(ID2,INPUT);
  digitalWrite(x1,HIGH);
  digitalWrite(x2,HIGH);
  digitalWrite(x3_8,HIGH);
  digitalWrite(soundGND,LOW);
  Clock.setClockMode(false);

}


void loop() {
  
  //Get and print the time.
  year=Clock.getYear();
  month=Clock.getMonth(Century);
  date=Clock.getDate();
  DoW=Clock.getDoW();
  hour=Clock.getHour(h12,PM);
  minute=Clock.getMinute();
  second=Clock.getSecond();
  temperature=Clock.getTemperature();
  
  
  //reset alarm status.
  if(isAlarming==1 && (analogRead(A0)==0 || analogRead(A1)==0 || analogRead(A2)==0 || analogRead(A3)==0))
  {
    digitalWrite(x1,HIGH);
    digitalWrite(x2,HIGH);
    isAlarming=0;
    Serial.println("Alarming end!");
    delay(500);
  }
  if(whichBedAlarming!=0 && digitalRead(ID1)==alarmForBed[whichBedAlarming-1][5])
  {
    digitalWrite(x1,HIGH);
    whichBedAlarming=0;
    Serial.println("Bed Alarming end!");
    delay(500);
  }
  if(whichWorkAlarming!=0 && digitalRead(ID2)==alarmForWork[whichWorkAlarming-1][5])
  {
    digitalWrite(x2,HIGH);
    whichWorkAlarming=0;
    Serial.println("Work Alarming end!");
    delay(500);
  }
  
  //set time
  for(byte i=0;i<2;i++)
  {
    if(analogRead(A0)==0 && isAlarming!=1)
    {
      Serial.println("set time");
      setTime();
      Serial.println("set alarm");
      setAlarm();
      Serial.println("set smart");
      setSmartAlarm();

      //show
      showArray();
      //Delay by milliseconds
      delay(1000);
    }
    else
    {
      show(year,month,date,hour,minute,second,DoW,temperature);
      ticTime();
      delay(485);
    }
      
  }
  
  //if the time match.
  checkAlarm();  
  Serial.println(digitalRead(ID1));
  Serial.println(digitalRead(ID2));
}


void show(byte year,byte month,byte date,byte hour,byte minute,byte second,byte DoW,double temperature)
{
  lcd.setCursor(0,0);
  //If some variable>9, we can say it have two characters, so we don't have to put zeros in front of it.
  //The year just have one or two character, so we should plus "20" in front it.

  //First line.
  lcd.print((year>9)?String("20")+year:String("200")+year); 
  lcd.print((month>9)?String("-")+month:String("-0")+month);
  lcd.print((date>9)?String("-")+date:String("-0")+date);
  //Print day of week.
  lcd.print("  "+getDoW_Str(DoW));
  
  //Second line.
  lcd.setCursor(0,1);
  lcd.print((hour>9)?hour:String("0")+hour);
  lcd.print((minute>9)?String(":")+minute:String(":0")+minute);
  lcd.print((second>9)?String(":")+second:String(":0")+second);
  //Print the temperature.
  lcd.print(String(" #")+temperature);
}
  
void setTime()
{ 
  for(byte i=1;i<7;(analogRead(A0)==0?i++:i=i),(analogRead(A3)==0?i--:i=i))
  {
    second=Clock.getSecond();
    switch (i)
    {
    case 1:
      check(&year,99,0);
      show(year,month,date,hour,minute,second,DoW,temperature);
      Serial.println("set year");
      ticSet(0,0,4);
      break;
    case 2:
      check(&month,12,1);
      show(year,month,date,hour,minute,second,DoW,temperature);
      Serial.println("set month");
      ticSet(5,0,2);
      break;
    case 3:
      check(&date,maxDate(month),1);
      show(year,month,date,hour,minute,second,DoW,temperature);
      Serial.println("set date");
      ticSet(8,0,2);
      break;
    case 4:
      check(&hour,23);
      show(year,month,date,hour,minute,second,DoW,temperature);
      Serial.println("set hour");
      ticSet(0,1,2);
      break;
    case 5:
      check(&minute,59);
      show(year,month,date,hour,minute,second,DoW,temperature);
      Serial.println("set minute");
      ticSet(3,1,2);
      break;
    case 6:
      if(analogRead(A1)==0 || analogRead(A2)==0)
      {
        second=0;
        Clock.setSecond(second);
        second=Clock.getSecond();
      }
      show(year,month,date,hour,minute,second,DoW,temperature); 
      ticSet(6,1,2);
      Serial.println("set second");
      break;         
    }    
    DoW=getDoW();
    delay(485);
  }
  Clock.setYear(year);
  Clock.setMonth(month);
  Clock.setDate(date);
  Clock.setHour(hour);
  Clock.setMinute(minute);
  Clock.setDoW(getDoW());
  Clock.setClockMode(h12); 
}

void check(byte *in,byte max,byte min)
{
  if((*in)>max)
  {
    (*in)=min;
  }
  if(analogRead(A1)==0)
  {
    (*in)++;
    if((*in)>max)
    {
      (*in)=min;
    }
  }
  else if(analogRead(A2)==0)
  {
    (*in)--;
    if((*in)>max ||(*in)<min)
    {
      (*in)=max;
    }
  }
}
void check(byte *in,byte max)
{
  check(in,max,0);
}

void setAlarm()
{
  for(byte j=0;j<6;j++)
  {
    Serial.print(j);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(String("Alarm ")+(j+1));
    delay(500);
    while(analogRead(A0)!=0 && analogRead(A3)!=0)
    {
      Serial.println("waiting...");
      delay(250);
    }
    if(analogRead(A3)==0)
    {
      delay(250);
      continue;
    }
    else if(analogRead(A0)==0)
    {
      bool isDoW=false;
      byte isEnable=1;
      for(byte i=1;i<7;(analogRead(A0)==0?i++:i=i))
      {
        if(analogRead(A0)==0 || analogRead(A1)==0 || analogRead(A2)==0)
        {
          lcd.clear();
        }
        if(analogRead(A0)==0 && isEnable==0)
        {
          alarm[j][0]=255;
          break;
        }
        if(analogRead(A0)==0 && alarm[j][0]!=0 && isDoW)
        {
          i=5;
          isDoW=false;
        }
        switch (i)
        {
        case 1:
          lcd.setCursor(0,0);
          lcd.print("Enable or not?");
          check(&isEnable,1);
          lcd.setCursor(0,1);
          if(isEnable==0)
          {
            lcd.print("Do not enable");
          }
          else
          {
            lcd.print("Enable");
          }
          break;
        case 2:
          lcd.setCursor(0,0);
          lcd.print("date of week:");
          check(&alarm[j][0],7);
          lcd.setCursor(0,1);
          if(alarm[j][0]==0)
          {
            lcd.println("Do not enable   ");
          }
          else
          {
            lcd.print(getDoW_Str(alarm[j][0]));
            isDoW=true;
          }
          Serial.println("alarm DoW");
          break;
        case 3:
          lcd.setCursor(0,0);
          lcd.print("month:");
          check(&alarm[j][1],12);
          lcd.setCursor(0,1);
          if(alarm[j][1]==0)
          {
            lcd.print("Do not enable");
          }
          else
          {
            lcd.print(alarm[j][1]<10?String("0")+alarm[j][1]:alarm[j][1]);
          }
          Serial.println("alarm month");
          break;
        case 4:
          lcd.setCursor(0,0);
          lcd.print("date:");
          check(&alarm[j][2],maxDate(alarm[j][1]));
          lcd.setCursor(0,1);
          if(alarm[j][2]==0)
          {
            lcd.print("Do not enable");
          }
          else
          {
            lcd.print(alarm[j][2]<10?String("0")+alarm[j][2]:alarm[j][2]);
          }
          Serial.println("alarm date");
          break;
        case 5:
          lcd.setCursor(0,0);
          lcd.print("hour:");
          check(&alarm[j][3],24);
          lcd.setCursor(0,1);
          if(alarm[j][3]==0)
          {
            lcd.print("Do not enable");
          }
          else
          {
            lcd.print(alarm[j][3]<10?String("0")+alarm[j][3]:alarm[j][3]);
          }
          Serial.println("alarm hour");
          break;
        case 6:
          lcd.setCursor(0,0);
          lcd.print("minute:");
          check(&alarm[j][4],59);
          lcd.setCursor(0,1);
          lcd.print(alarm[j][4]<10?String("0")+alarm[j][4]:alarm[j][4]);
          Serial.println("alarm minute");
          break;
        }
        //lcd.clear();
        Serial.println("delay1");
        delay(250);
      }
    }
    showArray();
  }
}

void setSmartAlarm()
{
  
  for(byte j=0;j<6;j++)
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(String("Alarm for sleep")+String(j+1));
    delay(500);
    while(analogRead(A0)!=0 && analogRead(A3)!=0)
    {
      delay(250);
    }
    if(analogRead(A3)==0)
    {
      delay(250);
      continue;
    }
    else if(analogRead(A0)==0)
    {
      byte isEnable=1;
      byte isSleep=1;
      bool isDoW=false;
      for(byte i=1;i<8;(analogRead(A0)==0?i++:i=i))
      {
        if(analogRead(A0)==0 || analogRead(A1)==0 || analogRead(A2)==0)
        {
          lcd.clear();
        }
        if(analogRead(A0)==0 && isEnable==0)
        {
          alarmForBed[j][0]=255;
          break;
        }
        if(analogRead(A0)==0 && alarmForBed[j][0]!=0 && isDoW)
        {
          i=5;
          isDoW=false;
        }
        switch (i)
        {
        case 1:
          lcd.setCursor(0,0);
          lcd.print("Enable or not?");
          check(&isEnable,1);
          lcd.setCursor(0,1);
          if(isEnable==0)
          {
            lcd.print("Do not enable");
          }
          else
          {
            lcd.print("Enable");
          }
          break;
        case 2:
          lcd.setCursor(0,0);
          lcd.print("date of week:");
          check(&alarmForBed[j][0],7);
          lcd.setCursor(0,1);
          if(alarmForBed[j][0]==0)
          {
            lcd.print("Do not enable    ");
          }
          else
          {
            lcd.print(getDoW_Str(alarmForBed[j][0]));
            isDoW=true;
          }
          Serial.println("alarm DoW");
          break;
        case 3:
          lcd.setCursor(0,0);
          lcd.print("month:");
          check(&alarmForBed[j][1],12);
          lcd.setCursor(0,1);
          if(alarmForBed[j][1]==0)
          {
            lcd.print("Do not enable");
          }
          else
          {
            lcd.print(alarmForBed[j][1]<10?String("0")+alarmForBed[j][1]:alarmForBed[j][1]);
          }
          Serial.println("alarmForBed month");
          break;
        case 4:
          lcd.setCursor(0,0);
          lcd.print("date:");
          check(&alarmForBed[j][2],maxDate(alarmForBed[j][1]));
          lcd.setCursor(0,1);
          if(alarmForBed[j][2]==0)
          {
            lcd.print("Do not enable");
          }
          else
          {
            lcd.print(alarmForBed[j][2]<10?String("0")+alarmForBed[j][2]:alarmForBed[j][2]);
          }
          Serial.println("alarm date");
          break;
        case 5:
          lcd.setCursor(0,0);
          lcd.print("hour:");
          check(&alarmForBed[j][3],24);
          lcd.setCursor(0,1);
          if(alarmForBed[j][3]==0)
          {
            lcd.print("Do not enable");
          }
          else
          {
            lcd.print(alarmForBed[j][3]<10?String("0")+alarmForBed[j][3]:alarmForBed[j][3]);
          }
          Serial.println("alarm hour");
          break;
        case 6:
          lcd.setCursor(0,0);
          lcd.print("minute:");
          check(&alarmForBed[j][4],59);
          lcd.setCursor(0,1);
          lcd.print(alarmForBed[j][4]<10?String("0")+alarmForBed[j][4]:alarmForBed[j][4]);
          Serial.println("alarm minute");
          break;
        case 7:
          lcd.setCursor(0,0);
          lcd.print("Sleep or get up?");
          check(&isSleep,1);
          lcd.setCursor(0,1);
          if(isSleep==0)
          {
            lcd.print("Sleep");
            alarmForBed[j][5]=1;
          }
          else
          {
            lcd.print("Get up");
            alarmForBed[j][5]=0;
          }
        }
        delay(250);
      }
    }
    showArray();
  }
  for(byte j=0;j<6;j++)
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(String("Alarm for work")+(j+1));
    delay(500);
    while(analogRead(A0)!=0 && analogRead(A3)!=0)
    {
      delay(250);
    }
    if(analogRead(A3)==0)
    {
      delay(250);
      continue;
    }
    else if(analogRead(A0)==0)
    {
      byte isEnable=1;
      byte isSleep=1;
      bool isDoW=false;
      for(byte i=1;i<8;(analogRead(A0)==0?i++:i=i))
      {
        if(analogRead(A0)==0 || analogRead(A1)==0 || analogRead(A2)==0)
        {
          lcd.clear();
        }
        if(analogRead(A0)==0 && isEnable==0)
        {
          alarmForWork[j][0]=255;
          break;
        }
        if(analogRead(A0)==0 && alarmForWork[j][0]!=0 && isDoW)
        {
          i=5;
          isDoW=false;
        }
        switch (i)
        {
        case 1:
          lcd.setCursor(0,0);
          lcd.print("Enable or not?");
          check(&isEnable,1);
          lcd.setCursor(0,1);
          if(isEnable==0)
          {
            lcd.print("Do not enable");
          }
          else
          {
            lcd.print("Enable");
          }
          break;
        case 2:
          lcd.setCursor(0,0);
          lcd.print("date of week:");
          check(&alarmForWork[j][0],7);
          lcd.setCursor(0,1);
          if(alarmForWork[j][0]==0)
          {
            lcd.print("Do not enable");
          }
          else
          {
            lcd.print(getDoW_Str(alarmForWork[j][0]));
            isDoW=true;
          }
          Serial.println("alarm DoW");
          break;
        case 3:
          lcd.setCursor(0,0);
          lcd.print("month:");
          check(&alarmForWork[j][1],12);
          lcd.setCursor(0,1);
          if(alarmForWork[j][1]==0)
          {
            lcd.print("Do not enable");
          }
          else
          {
            lcd.print(alarmForWork[j][1]<10?String("0")+alarmForWork[j][1]:alarmForWork[j][1]);
          }
          Serial.println("alarm month");
          break;
        case 4:
          lcd.setCursor(0,0);
          lcd.print("date:");
          check(&alarmForWork[j][2],maxDate(alarmForWork[j][1]));
          lcd.setCursor(0,1);
          if(alarmForWork[j][2]==0)
          {
            lcd.print("Do not enable");
          }
          else
          {
            lcd.print(alarmForWork[j][2]<10?String("0")+alarmForWork[j][2]:alarmForWork[j][2]);
          }
          Serial.println("alarm date");
          break;
        case 5:
          lcd.setCursor(0,0);
          lcd.print("hour:");
          check(&alarmForWork[j][3],24);
          lcd.setCursor(0,1);
          if(alarmForWork[j][3]==0)
          {
            lcd.print("Do not enable");
          }
          else
          {
            lcd.print(alarmForWork[j][3]<10?String("0")+alarmForWork[j][3]:alarmForWork[j][3]);
          }
          Serial.println("alarm hour");
          break;
        case 6:
          lcd.setCursor(0,0);
          lcd.print("minute:");
          check(&alarmForWork[j][4],59);
          lcd.setCursor(0,1);
          lcd.print(alarmForWork[j][4]<10?String("0")+alarmForWork[j][4]:alarmForWork[j][4]);
          Serial.println("alarm minute");
          break;
        case 7:
          lcd.setCursor(0,0);
          lcd.print("Work or rest?");
          check(&isSleep,1);
          lcd.setCursor(0,1);
          if(isSleep==0)
          {
            lcd.print("Work");
            alarmForWork[j][5]=1;
          }
          else
          {
            lcd.print("Rest");
            alarmForWork[j][5]=0;
          }

        }
        delay(250);
      }
    }
    showArray();
  }

}

byte getDoW()
{
    byte y=year;
    byte m=month;
    byte d=date;
    if(m==1||m==2) {
        m+=12;
        y--;
    }
    byte DoW=((d+2*m+3*(m+1)/5+y+y/4-y/100+y/400)%7)+1;
    return DoW;
}

void checkAlarm()
{
  for(int i=0;i<5;i++)
  {
    if(((alarm[i][0]==DoW && (alarm[i][3]==hour || alarm[i][3]==0) && alarm[i][4]==minute) 
    || (alarm[i][0]==0 && (alarm[i][1]==month || alarm[i][1]==0)&& (alarm[i][2]==date || alarm[i][2]==0)&& (alarm[i][3]==hour || alarm[i][3]==0)&& alarm[i][4]==minute))
    && second==0)//If ordinary alarm match, ' &&second==0' mean it just once in one minute.
    {
      isAlarming=1;
      digitalWrite(x1,LOW);
      digitalWrite(x2,LOW);
    }
  }
  for(int i=0;i<6;i++)
  {
    if((((alarmForBed[i][0]==DoW && (alarmForBed[i][3]==hour || alarmForBed[i][3]==0) && alarmForBed[i][4]==minute) 
    || (alarmForBed[i][0]==0 && (alarmForBed[i][1]==month || alarmForBed[i][1]==0)&& (alarmForBed[i][2]==date || alarmForBed[i][2]==0)
    && (alarmForBed[i][3]==hour || alarmForBed[i][3]==0)&& alarmForBed[i][4]==minute)) 
    && digitalRead(ID1)!=alarmForBed[i][5])&&second==0)
    {
      whichBedAlarming=i+1;
      digitalWrite(x1,LOW);
    }
    if((((alarmForWork[i][0]==DoW && (alarmForWork[i][3]==hour || alarmForWork[i][3]==0) && alarmForWork[i][4]==minute)
    || (alarmForWork[i][0]==0 && (alarmForWork[i][1]==month || alarmForWork[i][1]==0) && (alarmForWork[i][2]==date || alarmForWork[i][2]==0)
    && (alarmForWork[i][3]==hour || alarmForWork[i][3]==0)&& alarmForWork[i][4]==minute)) 
    && digitalRead(ID2)!=alarmForWork[i][5])&&second==0)
    {
      whichWorkAlarming=i+1;
      digitalWrite(x2,LOW);
    }
  }
}

void ticSet(int x,int y,int l)
{
  if(isTic)
  {
    lcd.setCursor(x,y);
    while(l!=0)
    {
      lcd.print(" ");
      l--;
    }
  }
  isTic=!isTic;
}

void ticTime()
{ 
  Serial.println("in tic");
  if(isTic)
  {
    Serial.println("tic...");
    lcd.setCursor(2,1);
    lcd.print(" ");
    lcd.setCursor(5,1);
    lcd.print(" ");
  }
  isTic=!isTic;
}

int maxDate(int month)
{
  switch(month)
  {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
      return 31;
      break;
    case 2:
      if(year%100==0?year%4==0:year%400==0)
      {
        return 29;
      }
      else
      {
        return 28;
      }
    case 0:
    case 4:
    case 6:
    case 9:
    case 11:
      return 30;
  }
}

void showArray()
{
  for(int i=0;i<6;i++)
  {
    for(int j=0;j<5;j++)
    {
      Serial.print(alarm[i][j]);
      if(j<4)
        Serial.print("-");
    }
    Serial.println(" ");
  }
  Serial.println("++++++++++++++++++++++++");
  for(int i=0;i<6;i++)
  {
    for(int j=0;j<6;j++)
    {
      Serial.print(alarmForBed[i][j]);
      if(j<5)
        Serial.print("-");

    }
    Serial.println(" ");
  }
  Serial.println("----------------");
  for(int i=0;i<6;i++)
  {
    for(int j=0;j<6;j++)
    {
      Serial.print(alarmForWork[i][j]);
      if(j<5)
        Serial.print("-");

    }
    Serial.println(" ");
  }

}

String getDoW_Str(int DoW)
{
    switch (DoW)
    {
    case 1:
      return("MON");
    case 2:
      return("TUE");
    case 3:
      return("WED");
    case 4:
      return("THU");
    case 5:
      return("FRI");
    case 6:
      return("SAT");
    case 7:
      return("SUN");
    }

}
