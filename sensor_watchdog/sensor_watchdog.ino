#define FET_PIN   1
#define COMM_PIN  2
#define RESET_PIN 3
#define WAIT_TIME 2000
#define UP_TIMEOUT_TIME 30000 /* probably waay too high */
#define SUCCESS_TIMEOUT_TIME 500 
#define SUCCESS_DOWNTIME_SECS 20 /* way too low in practice, prob fine for test */
#define FAIL_DOWNTIME_SECS 10 /* way too low in practice, prob fine for test */


int read_state;
unsigned long start_time;
unsigned long cur_time;

void setup() {
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
  delay(SUCCESS_DOWNTIME_SECS * 1000);
}

void fail() {
  digitalWrite(FET_PIN, LOW);
  delay(FAIL_DOWNTIME_SECS * 1000);
}

