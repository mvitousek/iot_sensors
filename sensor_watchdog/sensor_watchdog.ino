#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#define FET_PIN   1
#define COMM_PIN  2
#define RESET_PIN 3
#define WAIT_TIME 2000
#define UP_TIMEOUT_TIME 30000ul
#define SUCCESS_TIMEOUT_TIME 500 
#define SUCCESS_DOWNTIME 900000ul
#define FAIL_DOWNTIME 8000ul # Min time actually asleep is just over 8s




int read_state;
unsigned long start_time;
unsigned long cur_time;
unsigned long slept_time;
uint8_t mcucr1, mcucr2;

void setup() {
  wdt_reset();
  wdtDisable();
  pinMode(FET_PIN, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);
  pinMode(COMM_PIN, INPUT);
  digitalWrite(RESET_PIN, HIGH);
}

void loop() {
  digitalWrite(FET_PIN, HIGH);
  delay(WAIT_TIME);

  read_state = digitalRead(COMM_PIN);
  if (read_state == HIGH) {
    wait_for_signal();
  } else {
    fail();
  }
}

void wait_for_signal() {
  start_time = millis();
  while(true) {
    delay(5);
    read_state = digitalRead(COMM_PIN);
    if (read_state == LOW) {
      wait_for_success_signal();
      return;
    }
    cur_time = millis();
    if (cur_time - start_time > UP_TIMEOUT_TIME) {
      fail();
      return;
    }
  }
}

void wait_for_success_signal() {
  start_time = millis();
  while(true) {
    delay(5);
    read_state = digitalRead(COMM_PIN);
    if (read_state == HIGH) {
      success();
      return;
    }
    cur_time = millis();
    if (cur_time - start_time > SUCCESS_TIMEOUT_TIME) {
      fail();
      return;
    }
  }
}

void success() {
  digitalWrite(FET_PIN, LOW);
  sleep(SUCCESS_DOWNTIME);
}

void fail() {
  digitalWrite(FET_PIN, LOW);
  sleep(FAIL_DOWNTIME);
}

// Based on https://github.com/JChristensen/millionOhms_SW/blob/master/millionOhms.cpp
void sleep(unsigned long downtime) {
     slept_time = 0;
     do {
        ACSR |= _BV(ACD);                         //disable the analog comparator
        ADCSRA &= ~_BV(ADEN);                     //disable ADC
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        wdtEnable();              //start the WDT
        //turn off the brown-out detector.
        //must have an ATtiny45 or ATtiny85 rev C or later for software to be able to disable the BOD.
        //current while sleeping will be <0.5uA if BOD is disabled, <25uA if not.
        cli();
        mcucr1 = MCUCR | _BV(BODS) | _BV(BODSE);  //turn off the brown-out detector
        mcucr2 = mcucr1 & ~_BV(BODSE);
        MCUCR = mcucr1;
        MCUCR = mcucr2;
        sei();                         //ensure interrupts enabled so we can wake up again
        sleep_cpu();                   //go to sleep
                                       //----zzzz----zzzz----zzzz----zzzz
        sleep_disable();
        wdtDisable();                  //don't need the watchdog while we're awake
        slept_time += 8192;
        if (slept_time >= downtime)) {
            break;
        }
     } while (true);
} 

//enable the wdt for 8sec interrupt
void wdtEnable(void)
{
    wdt_reset();
    cli();
    MCUSR = 0x00;
    WDTCR |= _BV(WDCE) | _BV(WDE);
    WDTCR = _BV(WDIE) | _BV(WDP3) | _BV(WDP0);    //8192ms
    sei();
}

//disable the wdt
void wdtDisable(void)
{
    wdt_reset();
    cli();
    MCUSR = 0x00;
    WDTCR |= _BV(WDCE) | _BV(WDE);
    WDTCR = 0x00;
    sei();
}
