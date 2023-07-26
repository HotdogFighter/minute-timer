// minute-timer project by HotdogFighter
// 14 July 2023
// step on a pc keyboard that's hooked up to an arduino to trigger pomodoro timers. 6 led's for 6 sessions total. plays tones after each pomodoro.

//accidentally double triggers sometime when keyboard presses notes in quick succession. dunno why keyboard.clear() isn't working

//did a hacky workaround for turning the led's on pin 3 and pin 11 back on after using tone. there's a conflict between tone library and the pin3 & pin11.
// https://forum.arduino.cc/t/tone-function-affecting-parts-of-code/678590

//unsigned long instead of int for blink_for_x_seconds to prevent arithmetic errors. could not make timer_length_seconds above 40seconds without unsigned long.

#include <PS2Keyboard.h>
#include "pitches.h"

// notes in the melody:
int melody[] = {
  NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 4, 4, 4
};

// PWM for brightness on pins: 3, 5, 6, 9, 10, 11
// int led_1 = 3;
// int led_2 = 5;
// int led_3 = 6;
// int led_4 = 9;
// int led_5 = 10;
// int led_6 = 11;
int led_pins[] = { 3, 5, 6, 9, 10, 11 };
int led_pins_count = 6;
int led_pins_state[] = { 0, 0, 0, 0, 0, 0 };
int led_pins_state_count = 6;

//led brightness goes from 0 to 255
int led_max_brightness = 20;
int led_brightness_increment = 5;
int led_blink_delay_millis = 800;  //blink delay in milliseconds //debug using 200, supposed to be 800

//how long to blink for (in seconds)
unsigned long timer_length_seconds = 50;  //debug using 4, supposed to be 50

//how quickly to flash the led's when a pomodoro is finished
int flash_delay_millis = 300;
int between_flash_delay_millis = 800;

//keyboard buttons that control the arduino
char keyboard_buttons_to_start_timer[] = { 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ' ' };
int keyboard_buttons_to_start_timer_count = 17;
char keyboard_button_to_reset_led = { PS2_ESC };

//keyboard is hooked up to DataPin, IRQpin, VCC, GND. IRQpin can only be pin 2 or pin 3 on arduino pro mini
const int DataPin = 4;
const int IRQpin = 2;
PS2Keyboard keyboard;

void setup() {
  // make pins into dimmable LED's:
  for (int this_pin = 0; this_pin < 6; this_pin++) {
    pinMode(led_pins[this_pin], OUTPUT);
  }
  keyboard.begin(DataPin, IRQpin);
  Serial.begin(9600);
  Serial.println("My Sketch has started");
  delay(2000);
  Serial.println();
}

void loop() {

  if (keyboard.available()) {
    // read the next key
    char c = keyboard.read();
    Serial.print(c);

    //for every keyboard button that is pressed, check if it is in my array of stompable keyboard buttons
    for (int current_button_index = 0; current_button_index < keyboard_buttons_to_start_timer_count; current_button_index++) {
      char stompable = keyboard_buttons_to_start_timer[current_button_index];
      if (c == stompable) {
        //choose the first turned-off led and start blinking it
        for (int current_pin_index = 0; current_pin_index < led_pins_state_count; current_pin_index++) {
          if (led_pins_state[current_pin_index] == 0) {  //if pin is currently off:
            int currently_off_pin = led_pins[current_pin_index];

            blink_pin_for_x_seconds(currently_off_pin, timer_length_seconds);
            led_pins_state[current_pin_index] = 1;
            play_buzzer();

            // if (current_pin_index>=0){ // tone() function uses Timer2 so it messes with pin3 and pin11
            //   analogWrite(led_pins[0], led_max_brightness);
            // }
            // if (current_pin_index>=5){ // tone() function uses Timer2 so it messes with pin3 and pin11
            //   analogWrite(led_pins[5], led_max_brightness);
            // }

            flash_all_active_led(current_pin_index);

            break;
          }
  
        }
        break;
      }
    }

    //turn all led off if keyboard reset button is pressed
    if (c == keyboard_button_to_reset_led) {
      for (int current_pin_index = 0; current_pin_index < led_pins_state_count; current_pin_index++) {
        led_pins_state[current_pin_index] = 0;
        analogWrite(led_pins[current_pin_index], LOW);
      }
    }

    clear_keyboard_buffer();
  }

}

void blink_pin_for_x_seconds(int led_pin, unsigned long seconds)  //end with the led on, not off
{
  unsigned long current_millis = millis();
  unsigned long end_millis = current_millis + (seconds * 1000);

  Serial.println("start blinking");
  while (millis() <= end_millis) {
    analogWrite(led_pin, LOW);
    delay(led_blink_delay_millis);
    analogWrite(led_pin, led_max_brightness);
    delay(led_blink_delay_millis);
    // Serial.println("end blink round");
    Serial.print("millis: ");
    Serial.print(millis());
    Serial.print("  ");
    Serial.print("end_millis: ");
    Serial.println(end_millis);
  }
  Serial.println("end blink");
  return 0;
}

void play_buzzer() {
  for (int thisNote = 0; thisNote < 4; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(8, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(8);
  }
  return 0;
}

void flash_all_active_led(int current_pin_index) {
  //burst of three blinks performed twice. does it for all led's leading up to led_pin
  int how_many_sets_to_flash = 2;
  int how_many_reps_to_flash = 3;

  for (int i = 0; i < how_many_sets_to_flash; i++) {
    for (int i = 0; i < how_many_reps_to_flash; i++) {
      for (int i = 0; i <= current_pin_index; i++) {
        analogWrite(led_pins[i], LOW);
      }
      delay(flash_delay_millis);
      for (int i = 0; i <= current_pin_index; i++) {
        analogWrite(led_pins[i], led_max_brightness);
      }
      delay(flash_delay_millis);
    }
    delay(between_flash_delay_millis);
  }

  return 0;
}

void clear_keyboard_buffer(){
  int clear_counter = 0;
  while (keyboard.available()) {
    keyboard.clear();
    Serial.println(clear_counter);
    clear_counter = clear_counter++;
  }
}