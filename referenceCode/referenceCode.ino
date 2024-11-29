// RTC Programmable Clock
// sw1 Menu and Select
// sw2 and sw3 Change Values
// SW4 Home, back , Alarm Off
#include <EEPROM.h>
#include "RTClib.h"
RTC_DS3231 rtc;
int dot_status = 0;
int my_hour = 0, my_min = 0, my_date = 0, my_month = 0, my_second = 0, my_year;
int alarm_hour = 0, alarm_min = 0;
const int a = 10, b = 11, c = 4, d = 3, e = 2, f = A3, g = A2;
const int dot = 9;
const int s1 = A1, s2 = A0, s3 = 13, s4 = 12;
boolean sw1_status = 0, sw2_status = 0, sw3_status = 0, sw4_status = 0;
int switch_status = 0;
const int sw1 = 5, sw2 = 6, sw3 = 8, sw4 = 7;
int delay_time = 3 ;
char my_display[3] = "   ";
int buzzer_pin = 1;
int buzzer_status = 0;
int beep_delay = 20;
int default_alarm_hour = 19, default_alarm_min = 00;
boolean alarm_status = 1;

int hour_address = 0, min_address = 1;

void setup() {
  // put your setup code here, to run once:
#ifndef ESP8266
  while (!Serial);
#endif

  delay(100);

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  DateTime now = rtc.now();
  my_year = now.year();

  alarm_hour = EEPROM.read(hour_address);
  alarm_min = EEPROM.read(min_address);
  
  pinMode(dot, OUTPUT);

  pinMode(a, OUTPUT);
  pinMode(b, OUTPUT);
  pinMode(c, OUTPUT);
  pinMode(d, OUTPUT);
  pinMode(e, OUTPUT);
  pinMode(f, OUTPUT);
  pinMode(g, OUTPUT);

  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);
  pinMode(s4, OUTPUT);

  pinMode(sw1, INPUT);
  pinMode(sw2, INPUT);
  pinMode(sw3, INPUT);
  pinMode(sw4, INPUT);

  pinMode(buzzer_pin, OUTPUT);
}

void loop() {

  DateTime now = rtc.now(); // get_time_date();
  
  check_switch();           // check if any switch is pressed
  my_hour = now.hour();
  my_min = now.minute();
  my_date = now.day();
  my_month = now.month();

  int_to_string(my_hour, my_min);         // convert the data into a string
  digitalWrite(dot, (now.second()) % 2);  // blink the dot for 1 sec
  send_to_display();                      // display the data to seven segments

  if ((my_min == alarm_min ) && alarm_status )  // Check for alarm
  {
    if (my_hour == alarm_hour )
    {
      if (digitalRead(sw4))
      {
        delay(100);
        buzzer_status = 0;
      }
      Serial.println("Alarm!!");
      digitalWrite(buzzer_pin, ((now.second() % 2)) && buzzer_status);
    }
  }
  else
  {
    buzzer_status = 1;
    digitalWrite(buzzer_pin, 0);
  }
}



void get_time_date()              
{
  DateTime now = rtc.now();
  my_hour = now.hour();
  my_min = now.minute();
  my_date = now.day();
  my_month = now.month();
  my_second = now.second();
}


void int_to_string(int i1, int i2 )
{
  if (i1 < 10 && (i2 < 10))
  {
    sprintf(my_display, "0%d0%d", i1, i2);
  }
  else if (i1 < 10 && (i2 >= 10))
  {
    sprintf(my_display, "0%d%d", i1, i2);
  }
  else if (i1 >= 10 && (i2 < 10))
  {
    sprintf(my_display, "%d0%d", i1, i2);
  }
  else if (i1 >= 10 && (i2 >= 10))
  {
    sprintf(my_display, "%d%d", i1, i2);
  }
}


void send_to_display()
{
  segment_1();
  display_(0);           // to display 1st digit
  delay(delay_time);
  low();

  segment_2();
  display_(1);           // to display 2nd digit
  delay(delay_time);
  low();

  segment_3();
  display_(2);           // to display 3rd digit
  delay(delay_time);
  low();

  segment_4();
  display_(3);           // to display 4th digit
  delay(delay_time);
  low();
}


void display_(int i)
{
  switch (my_display[i])
  {
    case '0' :
      zero();
      break;

    case '1' :
      one();
      break;

    case '2' :
      two();
      break;

    case '3':
      three();
      break;

    case '4':
      four();
      break;

    case '5':
      five();
      break;

    case '6':
      six();
      break;

    case '7':
      seven();
      break;


    case '8':
      eight();
      break;

    case '9':
      nine();
      break;

    case 'p':
      pattern();
      break;

    case 'q':
      pattern_2();
      break;

    case 'A':
      capital_A();
      break;

    case 'L':
      capital_L();
      break;

    case 'F':
      capital_F();
      break;

    case 'n':
      small_n();
      break;
  }
}

void segment_1()
{
  digitalWrite(s1, 0);
  digitalWrite(s2, HIGH);
  digitalWrite(s3, HIGH);
  digitalWrite(s4, HIGH);
}

void segment_2()
{
  digitalWrite(s1, HIGH);
  digitalWrite(s2, 0);
  digitalWrite(s3, HIGH);
  digitalWrite(s4, HIGH);
}

void segment_3()
{
  digitalWrite(s1, HIGH);
  digitalWrite(s2, HIGH);
  digitalWrite(s3, 0);
  digitalWrite(s4, HIGH);
}

void segment_4()
{
  digitalWrite(s1, HIGH);
  digitalWrite(s2, HIGH);
  digitalWrite(s3, HIGH);
  digitalWrite(s4, 0);
}

void all_segment_off()
{
  digitalWrite(s1, HIGH);
  digitalWrite(s2, HIGH);
  digitalWrite(s3, HIGH);
  digitalWrite(s4, HIGH);
}

void all_segment_on()
{
  digitalWrite(s1, 0);
  digitalWrite(s2, 0);
  digitalWrite(s3, 0);
  digitalWrite(s4, 0);
}

void low()
{
  digitalWrite(a, 0);
  digitalWrite(b, 0);
  digitalWrite(c, 0);
  digitalWrite(d, 0);
  digitalWrite(e, 0);
  digitalWrite(f, 0);
  digitalWrite(g, 0);
}                         

void one()
{
  digitalWrite(b, HIGH);
  digitalWrite(c, HIGH);
  digitalWrite(a, LOW);
  digitalWrite(f, LOW);
  digitalWrite(g, LOW);
  digitalWrite(e, LOW);
  digitalWrite(d, LOW);

}

void two()
{
  digitalWrite(b, 1);
  digitalWrite(c, 0);
  digitalWrite(a, 1);
  digitalWrite(f, 0);
  digitalWrite(g, 1);
  digitalWrite(e, 1);
  digitalWrite(d, 1);

}

void three()
{
  digitalWrite(a, 1);
  digitalWrite(b, 1);
  digitalWrite(c, 1);
  digitalWrite(d, 1);
  digitalWrite(e, 0);
  digitalWrite(f, 0);
  digitalWrite(g, 1);

}

void four()
{
  digitalWrite(a, 0);
  digitalWrite(b, 1);
  digitalWrite(c, 1);
  digitalWrite(d, 0);
  digitalWrite(e, 0);
  digitalWrite(f, 1);
  digitalWrite(g, 1);

}

void five()
{
  digitalWrite(a, 1);
  digitalWrite(b, 0);
  digitalWrite(c, 1);
  digitalWrite(d, 1);
  digitalWrite(e, 0);
  digitalWrite(f, 1);
  digitalWrite(g, 1);

}

void six()
{
  digitalWrite(a, 1);
  digitalWrite(b, 0);
  digitalWrite(c, 1);
  digitalWrite(d, 1);
  digitalWrite(e, 1);
  digitalWrite(f, 1);
  digitalWrite(g, 1);
}

void seven()
{
  digitalWrite(a, 1);
  digitalWrite(b, 1);
  digitalWrite(c, 1);
  digitalWrite(d, 0);
  digitalWrite(e, 0);
  digitalWrite(f, 0);
  digitalWrite(g, 0);

}

void eight()
{
  digitalWrite(a, 1);
  digitalWrite(b, 1);
  digitalWrite(c, 1);
  digitalWrite(d, 1);
  digitalWrite(e, 1);
  digitalWrite(f, 1);
  digitalWrite(g, 1);

}

void nine()
{
  digitalWrite(a, 1);
  digitalWrite(b, 1);
  digitalWrite(c, 1);
  digitalWrite(d, 1);
  digitalWrite(e, 0);
  digitalWrite(f, 1);
  digitalWrite(g, 1);

}

void zero()
{
  digitalWrite(a, 1);
  digitalWrite(b, 1);
  digitalWrite(c, 1);
  digitalWrite(d, 1);
  digitalWrite(e, 1);
  digitalWrite(f, 1);
  digitalWrite(g, 0);
}

void pattern()
{
  digitalWrite(a, 1);
  digitalWrite(b, 1);
  digitalWrite(c, 1);
  digitalWrite(d, 0);
  digitalWrite(e, 0);
  digitalWrite(f, 0);
  digitalWrite(g, 0);
}

void pattern_2()
{
  digitalWrite(a, 0);
  digitalWrite(b, 0);
  digitalWrite(c, 0);
  digitalWrite(d, 1);
  digitalWrite(e, 1);
  digitalWrite(f, 1);
  digitalWrite(g, 0);
}

void capital_A()
{
  digitalWrite(b, HIGH);
  digitalWrite(c, LOW);
  digitalWrite(a, HIGH);
  digitalWrite(f, HIGH);
  digitalWrite(g, HIGH);
  digitalWrite(e, HIGH);
  digitalWrite(d, HIGH);
}

void capital_L()
{
  digitalWrite(b, LOW);
  digitalWrite(c, LOW);
  digitalWrite(a, LOW);
  digitalWrite(f, HIGH);
  digitalWrite(g, LOW);
  digitalWrite(e, HIGH);
  digitalWrite(d, HIGH);
}

void capital_F()
{
  digitalWrite(b, LOW);
  digitalWrite(c, LOW);
  digitalWrite(a, HIGH);
  digitalWrite(f, HIGH);
  digitalWrite(g, HIGH);
  digitalWrite(e, HIGH);
  digitalWrite(d, LOW);
}

void small_n()
{
  digitalWrite(b, LOW);
  digitalWrite(c, HIGH);
  digitalWrite(a, LOW);
  digitalWrite(f, LOW);
  digitalWrite(g, HIGH);
  digitalWrite(e, HIGH);
  digitalWrite(d, LOW);
}

void check_switch()
{
  switch_status = read_switch_status();
  if (switch_status)
  {
    if (sw4_status)
    {
      display_alarm();
    }

    if (sw1_status)
    {
      beep();
      delay(300);
      menu();                     // go to menu if switch one is pressed
      delay(200);
    }

    else if (sw2_status)
    {
      display_date();             // display date if switch 2 is pressed
    }

    else if (sw3_status)
    {
      display_temperature();      // display tempreature if switch 3 is pressed
    }
  }
}

int display_on()
{
  long t1 = 0;
  t1 = millis();
  my_display[0]='A';
  my_display[1]='L';
  my_display[2]='0';
  my_display[3]='n';
  while (millis() - t1 >= 800)
  {
    send_to_display();
  }
}

int display_off()
{
  long t1 = 0;
  t1 = millis();
  my_display[0]='A';
  my_display[1]='0';
  my_display[2]='F';
  my_display[3]='F';
  while (millis() - t1 >= 800)
  {
    send_to_display();
  }
}
boolean read_switch_status()
{
  sw1_status = digitalRead(sw1);
  sw2_status = digitalRead(sw2);
  sw3_status = digitalRead(sw3);
  sw4_status = digitalRead(sw4);
  return (sw1_status || sw2_status || sw3_status || sw4_status);
}

void display_temperature()
{
  Serial.println("Display Temperature");
  float temperature = 0;
  int x1 = 0, x2 = 0;
  long t = 0;
  temperature = rtc.getTemperature();
  x1 = int(temperature);
  x2 = (temperature - x1) * 100;
  Serial.println(temperature);
  int_to_string(x1, x2);
  t = millis();
  while ((millis() - t) <= 2000)        // display temperature for 2 seconds
  {
    send_to_display();
    digitalWrite(dot, HIGH);
  }
}

void display_alarm()
{
  Serial.println("Display Alarm");
  long t = 0;
  int_to_string(alarm_hour, alarm_min);
  t = millis();
  while ((millis() - t) <= 2000)        // display Alarm time for 2 seconds
  {
    send_to_display();
    digitalWrite(dot, HIGH);
  }
}

void display_date()
{
  long t = 0;
  int_to_string(my_date, my_month);
  t = millis();
  while ((millis() - t) <= 1500)        // display date for 1.5 seconds
  {
    send_to_display();
    digitalWrite(dot, HIGH);
  }
  int_to_string(int(my_year / 100), (my_year - 2000));   // divide 4 digits into two seperate two digits
  t = millis();
  while ((millis() - t) <= 1000)        // display year for 0.5 seconds
  {
    send_to_display();
    digitalWrite(dot, LOW);
  }
}

void menu()
{
  Serial.println("entered menu");
  delay(100);
  while (!read_switch_status())
  {
    digitalWrite(dot, 0);
    DateTime now = rtc.now();
    if (now.second() % 2)
    {
      display_pattern_menu_2();              // display an animation when entered into menu
    }
    else
    {
      display_pattern_menu();
    }
  }
  
  if (sw1_status)
  { 
    // Set the clock parameters
    delay(100);
    if (set_time())
    {
      return 1;
    }
    else if (set_date())
    {
      return 1;
    }
    else if (set_year())
    {
      return 1;
    }
    else if (set_alarm())
    {
      return 1;
    }
  }
  else
  {
    beep();
  }
}

int set_time()
{

  int time_chandged_status = 0;
  beep();
  delay(100);
  read_switch_status();

  delay(100);
  while (!sw1_status)
  {
    read_switch_status();
    if (sw2_status)
    {
      time_chandged_status = 1;
      delay(150);
      my_hour++;
      if (my_hour > 23)
      {
        my_hour = 0;
      }
    }
    else if (sw3_status)
    {
      time_chandged_status = 1;
      delay(100);
      my_min++;
      if (my_min > 59)
      {
        my_min = 0;
      }
    }
    else if (sw4_status)
    {
      beep();
      return 1;
    }
    int_to_string(my_hour, my_min);
    send_to_display();
    digitalWrite(dot, HIGH);
  }

  if (time_chandged_status)
  {
    DateTime now = rtc.now();
    my_date = now.day();              // get the current date
    my_month = now.month();
    //now set the current date as it is and set updated time
    rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));;
    beep();
    delay(50);
  }
  get_time_date();
  delay(200);
  return 0;
}

int set_date()
{
  int date_chandged_status = 0;
  read_switch_status();
  beep();
  delay(100);
  while (!sw1_status)
  {
    read_switch_status();
    if (sw2_status)
    {
      date_chandged_status = 1;
      delay(150);
      my_date++;
      if (my_date > 31)
      {
        my_date = 0;
      }
    }
    else if (sw3_status)
    {
      date_chandged_status = 1;
      delay(150);
      my_month++;
      if (my_month > 12)
      {
        my_month = 0;
      }
    }
    else if (sw4_status)
    {
      beep();
      return 1;
    }
    int_to_string(my_date, my_month);
    send_to_display();
    digitalWrite(dot, HIGH);
  }

  if (date_chandged_status)
  {
    DateTime now = rtc.now();
    my_hour = now.hour();              // get the current time
    my_min = now.minute();
    rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
    beep();
    delay(50);
  }
  get_time_date();
  delay(200);
  return 0;
}

int set_year()
{
  int year_chandged_status = 0;
  read_switch_status();
  beep();
  while (!sw1_status)
  {
    read_switch_status();
    if (sw2_status)
    {
      year_chandged_status = 1;
      delay(150);
      my_year--;
      if (my_year < 2000 )
      {
        my_year = 2000;          // Constrain the year lower limit to 2000
      }
    }
    else if (sw3_status)
    {
      year_chandged_status = 1;
      delay(150);
      my_year++;
      if (my_year > 2070)        // Constrain the year upper limit to 2070
      {
        my_year = 2070;
      }
    }
    else if (sw4_status)
    {
      beep();
      return 1;
    }
    int_to_string(int(my_year / 100), (my_year - 2000));
    send_to_display();
    digitalWrite(dot, LOW);
  }

  if (year_chandged_status)
  {
    DateTime now = rtc.now();
    my_hour = now.hour();              // get the current time
    my_min = now.minute();
    my_date = now.day();               // get the current date
    my_month = now.month();
    //now set the current time and date as it is and set updated year
    rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
    beep();
    delay(50);
  }
  get_time_date();
  delay(200);
  return 0;
}


int set_alarm()
{
  int alarm_chandged_status = 0;
  read_switch_status();
  beep();
  while (!sw1_status)
  {
    read_switch_status();
    if (sw2_status)
    {
      alarm_chandged_status = 1;
      delay(150);
      alarm_hour++;
      if (alarm_hour > 23 )
      {
        alarm_hour = 0;
      }
    }
    else if (sw3_status)
    {
      alarm_chandged_status = 1;
      delay(150);
      alarm_min++;
      if (alarm_min > 59)
      {
        alarm_min = 0;
      }
    }
    else if (sw4_status)
    {
      beep();
      return 1;
    }
    int_to_string(alarm_hour, alarm_min);
    send_to_display();
    digitalWrite(dot, HIGH);
  }
  EEPROM.write(hour_address, alarm_hour);        // Set the Alarm parameters to EEPROM of ATmega328p
  EEPROM.write(min_address, alarm_min);
  get_time_date();
  beep();
  delay(200);
  return 0;
}

void display_pattern_menu()
{
  my_display[0] = 'q';
  my_display[1] = 'p';
  my_display[2] = 'q';
  my_display[3] = 'p';
  send_to_display();
}

void display_pattern_menu_2()
{
  my_display[0] = 'p';
  my_display[1] = 'q';
  my_display[2] = 'p';
  my_display[3] = 'q';
  send_to_display();
}

void beep()
{
  digitalWrite(buzzer_pin, HIGH);
  delay(beep_delay);
  digitalWrite(buzzer_pin, LOW);
}
