#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// Core Settings: Pins, Game Rules, and Display Config
#define i2c_Address 0x3C

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define POT_LEFT A0
#define TRIG_PIN 2
#define ECHO_PIN 3
#define BUTTON_YES 4
#define BUTTON_NO 5

const int PADDLE_WIDTH = 3;
const int PADDLE_HEIGHT = 14;
const int BALL_SIZE = 3;
const int BALL_SPEED = 2;
const int MAX_SCORE = 5;

int paddleLeftY = 0;
int paddleRightY = 0;

#define MIN_DISTANCE 5
#define MAX_DISTANCE 15

float ballX, ballY;
float ballDX, ballDY;

int scoreLeft = 0;
int scoreRight = 0;

void setup() {
  // Initializes all hardware and start the game
  Serial.begin(9600);
  
  display.begin(i2c_Address, true);
  display.clearDisplay();
  display.display();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(BUTTON_YES, INPUT_PULLUP);
  pinMode(BUTTON_NO, INPUT_PULLUP);

  titleScreenLoop();
  showCenteredMessage("Game Start", 2, 1500);

  resetScores();
  resetGame();
}

void loop() {
  // Main loop: read sensors, move ball, redraw screen
  paddleLeftY = mapPaddle(analogRead(POT_LEFT));
  float dist = measureDistance();
  paddleRightY = mapDistanceToPaddle(dist);
  updateBall();
  drawScene();
  checkForGameOver();
  delay(20);
}

void titleScreenLoop() {
  while (true) {
    if (digitalRead(BUTTON_YES) == LOW) {
      return;
    }

    display.clearDisplay();
    display.setTextSize(3);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(19, 20);
    display.println("PONG!");
    display.display();
    delay(1000);

    display.clearDisplay();
    display.display();
    delay(1000);
  }
}

void showGameOver(const char* winner) {
  String msg = String(winner) + " Wins!";
  showCenteredMessage(msg.c_str(), 2, 3000);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  int x = 7;
  int y = 20;
  display.setCursor(x, y);
  display.println("restart ?");

  y += 10;
  display.setCursor(x, y);
  display.println("yes : start button");

  y += 10;
  display.setCursor(x, y);
  display.println("no  : quit button");

  display.display();
  waitForYesNo();
}

void waitForYesNo() {
  while (true) {
    if (digitalRead(BUTTON_YES) == LOW) {
      resetScores();
      resetGame();
      showCenteredMessage("Game Start", 2, 1500);
      return;
    }
    if (digitalRead(BUTTON_NO) == LOW) {
      titleScreenLoop();
      resetScores();
      resetGame();
      showCenteredMessage("Game Start", 2, 1500);
      return;
    }
    delay(50);
  }
}

void showCenteredMessage(const char* msg, int textSize, int durationMs) {
  display.clearDisplay();
  display.setTextSize(textSize);
  display.setTextColor(SH110X_WHITE);

  int msgLen = strlen(msg);
  int charWidth = 6 * textSize;
  int textWidth = msgLen * charWidth;
  int lineHeight = 8 * textSize;
  int x = (SCREEN_WIDTH - textWidth) / 2;
  int y = (SCREEN_HEIGHT - lineHeight) / 2;

  display.setCursor(x, y);
  display.println(msg);
  display.display();
  delay(durationMs);
  display.clearDisplay();
  display.display();
}

void checkForGameOver() {
  if (scoreLeft >= MAX_SCORE) {
    showGameOver("Left");
  }
  else if (scoreRight >= MAX_SCORE) {
    showGameOver("Right");
  }
}

void resetScores() {
  scoreLeft = 0;
  scoreRight = 0;
}

void resetGame() {
  ballX = SCREEN_WIDTH / 2;
  ballY = SCREEN_HEIGHT / 2;
  ballDX = BALL_SPEED;
  ballDY = BALL_SPEED;
}

// Converts raw sensor values to on-screen paddle positions
int mapPaddle(int potVal) {
  return map(potVal, 0, 1023, 0, SCREEN_HEIGHT - PADDLE_HEIGHT);
}

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  float distance = (float)duration / 58.2;
  return distance;
}

int mapDistanceToPaddle(float distance) {
  if (distance < MIN_DISTANCE) distance = MIN_DISTANCE;
  if (distance > MAX_DISTANCE) distance = MAX_DISTANCE;
  int y = map((int)distance, MIN_DISTANCE, MAX_DISTANCE, 0, (SCREEN_HEIGHT - PADDLE_HEIGHT));
  return y;
}

void updateBall() {
  // Handles all ball movement and collision physics
  ballX += ballDX;
  ballY += ballDY;

  if (ballY <= 0) {
    ballY = 0;
    ballDY = -ballDY;
  }
  else if (ballY >= (SCREEN_HEIGHT - BALL_SIZE)) {
    ballY = SCREEN_HEIGHT - BALL_SIZE;
    ballDY = -ballDY;
  }

  if (ballX <= PADDLE_WIDTH) {
    if (ballY + BALL_SIZE >= paddleLeftY && ballY <= paddleLeftY + PADDLE_HEIGHT) {
      ballX = PADDLE_WIDTH;
      ballDX = -ballDX;
    } else {
      scoreRight++;
      resetBallPosition();
    }
  }

  if (ballX >= SCREEN_WIDTH - PADDLE_WIDTH - BALL_SIZE) {
    if (ballY + BALL_SIZE >= paddleRightY && ballY <= paddleRightY + PADDLE_HEIGHT) {
      ballX = SCREEN_WIDTH - PADDLE_WIDTH - BALL_SIZE;
      ballDX = -ballDX;
    } else {
      scoreLeft++;
      resetBallPosition();
    }
  }
}

void resetBallPosition() {
  ballX = SCREEN_WIDTH / 2;
  ballY = SCREEN_HEIGHT / 2;
  ballDX = -ballDX;
  ballDY = BALL_SPEED;
}

void drawScene() {
  display.clearDisplay();
  display.fillRect(0, paddleLeftY, PADDLE_WIDTH, PADDLE_HEIGHT, SH110X_WHITE);
  display.fillRect(SCREEN_WIDTH - PADDLE_WIDTH, paddleRightY, PADDLE_WIDTH, PADDLE_HEIGHT, SH110X_WHITE);
  display.fillRect((int)ballX, (int)ballY, BALL_SIZE, BALL_SIZE, SH110X_WHITE);

  for (int i = 0; i < SCREEN_HEIGHT; i += 4) {
    display.drawPixel(SCREEN_WIDTH / 2, i, SH110X_WHITE);
  }

  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(10, 0);
  display.print(scoreLeft);
  display.setCursor(SCREEN_WIDTH - 20, 0);
  display.print(scoreRight);

  display.display();
}