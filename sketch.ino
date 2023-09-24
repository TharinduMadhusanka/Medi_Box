#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <time.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define BUZZER 5
#define LED 15
#define HumiLED 2
#define TempLED 4
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 12

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET_DST 0

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtsensor;

// The following variables are used to store the current date and time.
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;
int months = 0;
int years = 0;

// The following variables are used to store time values for comparisons.
unsigned long timeNow = 0;
unsigned long timeLast = 0;

bool alarm_enabled = true;
int n_alarms = 3;
int alarm_hours[] = {0, 1,0};
int alarm_minutes[] = {1, 10,0};
bool alarm_triggered[] = {false, false,false};

// The following variables are used to store information about notes for music output.
int n_notes = 3;
int C = 262;
int D = 294;
int E = 330;
int notes[] = {C, D, E};

// Store information about program modes and user interface.
int current_mode = 0;
int max_modes = 5;
String modes[] = {"1- Set Alarm 1", "2- Set Alarm 2","3- Set Alarm 3", "4- Disable Alarm","5- Set Time Zone"};

void setup() {
  // Initialize PinModes
  pinMode(BUZZER, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(HumiLED, OUTPUT);
  pinMode(TempLED, OUTPUT);

  dhtsensor.setup(DHTPIN,DHTesp::DHT22);

  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("ssd1306 allocation failed");
    for (;;);
  }

  display.display();
  delay(800);

  // Connect to the wifi
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI",0,0,2);

  }
  display.clearDisplay();
  print_line("Connecting to WIFI",0,0,2);

  // Config time zone
  int UTC_OFFSET = set_time_zone();
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  display.clearDisplay();

  // Print welcome message
  print_line("Welcome", 0, 0, 3);
  display.clearDisplay();

}

void loop() {
  // put your main code here, to run repeatedly:
  update_time_with_check_alarm();

  if (digitalRead((PB_OK)) == LOW) {
    delay(200);
    goto_menu();
  }
  check_temp();
}

int set_time_zone(){
  float timeZone = 0;

  // loop until user confirms or cancels the time zone setting
  while (true) {
    display.clearDisplay();
    print_line(" Time Zone " + String(timeZone), 0, 0, 2);

    if (digitalRead(PB_UP) == LOW) {
      delay(200);
      tone(BUZZER, 262, 10);
      noTone(BUZZER);
      timeZone += 0.5;
    }
    else if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      tone(BUZZER, 262, 10);
      noTone(BUZZER);
      timeZone -= 0.5;
    }
    else if (digitalRead(PB_OK) == LOW) {
      delay(200);
      tone(BUZZER, 262, 10);
      noTone(BUZZER);
      break;
    }
    else if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      tone(BUZZER, 262, 10);
      noTone(BUZZER);
      timeZone = 0;
      break;
    }
  }
  display.clearDisplay();
  print_line("Time Zone is set", 0, 0, 2);
  delay(2000);
  // Convert time zone into Seconds 
  return timeZone*3600;

}

void print_line(String text, int column, int row, int text_size) {
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);
  display.display();
}

void print_time_now(void) {
  display.clearDisplay();

  // Print Hours Minutes and Seconds
  print_line(String(hours), 0, 0, 2);
  print_line(":", 20, 0, 2);
  print_line(String(minutes), 30, 0, 2);
  print_line(":", 50, 0, 2);
  print_line(String(seconds), 60, 0, 2);

  // Print Days Months Years
  print_line(String(days), 0, 25, 1);
  print_line("/", 10, 25, 1);
  print_line(String(months), 20, 25, 1);
  print_line("/", 30, 25, 1);
  print_line(String(years),40, 25, 1);

}

// Assign date time values to variables 
void update_time() {

  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  hours = atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute, 3, "%M", &timeinfo);
  minutes = atoi(timeMinute);

  char timeSeconds[3];
  strftime(timeSeconds, 3, "%S", &timeinfo);
  seconds = atoi(timeSeconds);

  char timeDay[3];
  strftime(timeDay, 3, "%d", &timeinfo);
  days = atoi(timeDay);

  char timeMonth[3];
  strftime(timeMonth, 3, "%m", &timeinfo);
  months = atoi(timeMonth);

  char timeYear[5];
  strftime(timeYear, 5, "%Y", &timeinfo);
  years = atoi(timeYear);

}

// Ring alarm and lightup LED
void ring_alarm() {
  display.clearDisplay();
  print_line("Medicine Time...", 0, 0, 2);

  digitalWrite(LED, HIGH);

  bool break_happend = false;

  //Ring the Buzzer

  while (break_happend == false && digitalRead(PB_CANCEL) == HIGH) {
    for (int i = 0; i < n_notes; i++) {
      if (digitalRead(PB_CANCEL) == LOW) {
        delay(200);
        break_happend = true;
        break;
      }

      tone(BUZZER, notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(20);
    }
  }

  digitalWrite(LED, LOW);
  display.clearDisplay();
}


void update_time_with_check_alarm(void) {
  update_time();
  print_time_now();

  if (alarm_enabled == true) {
    for (int i = 0; i < n_alarms; i++) {
      if (alarm_triggered[i] == false  && alarm_hours[i] == hours && alarm_minutes[i] == minutes) {
        ring_alarm();
        alarm_triggered[i] = true;
      }
    }
  }
}

// Configure buttons
int wait_for_button_press() {
  while (true) {
    if (digitalRead(PB_UP) == LOW) {
      delay(200);
      tone(BUZZER, 262, 10);
      noTone(BUZZER);
      return PB_UP;
    }
    else if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      tone(BUZZER, 262, 10);
      noTone(BUZZER);
      return PB_DOWN;
    }
    else if (digitalRead(PB_OK) == LOW) {
      delay(200);
      tone(BUZZER, 262, 10);
      noTone(BUZZER);
      return PB_OK;
    }
    else if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      tone(BUZZER, 262, 10);
      noTone(BUZZER);
      return PB_CANCEL;
    }
    update_time();
  }
}

// Main menu
void goto_menu() {
  while (digitalRead(PB_CANCEL) == HIGH) {
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);

    int pressed = wait_for_button_press();

    if (pressed == PB_UP) {
      delay(200);
      current_mode += 1;
      current_mode %= max_modes;
    }
    else if (pressed == PB_DOWN) {
      delay(200);
      current_mode -= 1;
      if (current_mode < 0) {
        current_mode = max_modes - 1;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      Serial.println(current_mode);
      run_mode(current_mode);
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
}

// This function sets the alarm for the specified alarm.
void set_alarm(int alarm) {
  int temp_hour = alarm_hours[alarm];
  // Loop until the user sets the alarm hour
  while (true) {
    display.clearDisplay();
    print_line("Enter hour; " + String(temp_hour), 0, 0, 2);

    // Wait for a button press
    int pressed = wait_for_button_press();

    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour %= 24;
    }
    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0) {
        temp_hour = 23;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      alarm_hours[alarm] = temp_hour;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  // The function then repeats the process for the minutes.
  int temp_minute = alarm_minutes[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter minute; " + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();

    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute %= 60;
    }
    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59;
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
  // Once the hour and minutes have been set, the function clears the display and prints the message "Alarm is set". The function then waits for 3 seconds before exiting.
  display.clearDisplay();
  print_line("Alarm is set", 0, 0, 2);
  delay(3000);
}

void run_mode(int mode) {

  //Set alarms
  if (mode == 0 || mode == 1 || mode == 2) {
    set_alarm(mode - 1);
  }
  // Disable all alarms
  else if (mode == 3) {
    alarm_enabled = false;
    display.clearDisplay();
    print_line("Alarm is disabled", 0, 0, 2);
    delay(3000);
  }
  // set time zone
  else if (mode == 4){
    int UTC_OFFSET = set_time_zone();
    configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
  }
}

void check_temp(){
  TempAndHumidity data = dhtsensor.getTempAndHumidity();
  
  // High temp value
  if (data.temperature > 32){
    display.clearDisplay();
    print_line("Temp is HIGH",0,40,1);
    digitalWrite(TempLED, HIGH);
  }
  // Low temp value
  else if (data.temperature < 26){
    display.clearDisplay();
    print_line("Temp is LOW",0,40,1);
    digitalWrite(TempLED, HIGH);
  }
  else{
    digitalWrite(TempLED, LOW);
  }
  
  // High huma value
  if (data.humidity > 80){
    display.clearDisplay();
    print_line("Huma' is HIGH",0,50,1);
    digitalWrite(HumiLED, HIGH);
  }
  // Low temp value
  else if (data.humidity < 60){
    display.clearDisplay();
    print_line("Huma' is LOW",0,50,1);
    digitalWrite(HumiLED, HIGH);
  }
  else{
    digitalWrite(HumiLED, LOW);
  }
}

