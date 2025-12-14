#include <M5Stack.h>

// State machine states
enum State {
  SETUP,        // Setting up timer (hours, minutes, seconds)
  COUNTDOWN,    // Timer is running
  DISARMING,    // Disarm button is being held
  DISARMED,     // Bomb successfully disarmed
  BOOM          // Timer reached zero
};

// Current state
State currentState = SETUP;

// Timer values (in seconds)
unsigned long totalSeconds = 0;
unsigned long remainingSeconds = 0;
unsigned long lastUpdate = 0;

// Setup values
int hours = 0;
int minutes = 0;
int seconds = 0;
int setupSelection = 0; // 0=hours, 1=minutes, 2=seconds

// Store initial time values to restore after disarm
int initialHours = 0;
int initialMinutes = 0;
int initialSeconds = 0;

// Disarm button tracking (any button can disarm)
bool disarmButtonPressed = false;
int disarmButtonId = -1; // -1 = none, 0 = A, 1 = B, 2 = C
unsigned long disarmPressStart = 0;
bool disarmingScreenInitialized = false; // Track if disarming screen was drawn
const unsigned long DISARM_HOLD_TIME = 5000; // 5 seconds in milliseconds

// BOOM flashing
bool boomFlashState = false;
unsigned long lastFlashTime = 0;
const unsigned long FLASH_INTERVAL = 200; // milliseconds

// Button debouncing
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_TIME = 200;

// Sound settings
const int CLICK_FREQUENCY = 2500;  // Hz - higher frequency for sharp click sound
const int CLICK_DURATION = 10;      // milliseconds - very short for click sound
const int COUNTDOWN_BEEP_FREQUENCY = 2000;  // Hz - frequency for countdown beeps
const int COUNTDOWN_BEEP_DURATION = 100;    // milliseconds - duration of countdown beeps
const int LONG_BEEP_FREQUENCY = 1500;       // Hz - frequency for long beep
const int LONG_BEEP_DURATION = 10000;       // milliseconds - 10 seconds

// Countdown beep tracking
unsigned long lastMinuteBeep = 0;
unsigned long last10SecondBeep = 0;
unsigned long lastSecondBeep = 0;
unsigned long lastFastBeep = 0;
bool longBeepPlaying = false;
unsigned long longBeepStartTime = 0;

void playClickSound() {
  // Play a short, sharp click sound using tone
  M5.Speaker.tone(CLICK_FREQUENCY, CLICK_DURATION);
}

void playCountdownBeep() {
  // Play a countdown beep
  M5.Speaker.tone(COUNTDOWN_BEEP_FREQUENCY, COUNTDOWN_BEEP_DURATION);
}

void displaySetup() {
  M5.Lcd.clear();
  
  // Title with larger font - centered
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(WHITE);
  // "SET TIMER" is 9 characters, size 3 is ~18px per char = ~162px total
  // Center: (320 - 162) / 2 = 79
  M5.Lcd.setCursor(79, 10);
  M5.Lcd.println("SET TIMER");
  
  // Line positions
  int lineHeight = 50;
  int startY = 70;
  int lineWidth = 280;
  int lineX = 20;
  
  // Display hours with background for selected
  int yPos = startY;
  if (setupSelection == 0) {
    // Draw background for selected line
    M5.Lcd.fillRect(lineX, yPos - 5, lineWidth, lineHeight, NAVY);
    M5.Lcd.setTextColor(YELLOW);
  } else {
    M5.Lcd.setTextColor(WHITE);
  }
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(lineX + 10, yPos);
  M5.Lcd.printf("Hours:   %02d", hours);
  
  // Display minutes with background for selected
  yPos = startY + lineHeight;
  if (setupSelection == 1) {
    // Draw background for selected line
    M5.Lcd.fillRect(lineX, yPos - 5, lineWidth, lineHeight, NAVY);
    M5.Lcd.setTextColor(YELLOW);
  } else {
    M5.Lcd.setTextColor(WHITE);
  }
  M5.Lcd.setCursor(lineX + 10, yPos);
  M5.Lcd.printf("Minutes: %02d", minutes);
  
  // Display seconds with background for selected
  yPos = startY + (lineHeight * 2);
  if (setupSelection == 2) {
    // Draw background for selected line
    M5.Lcd.fillRect(lineX, yPos - 5, lineWidth, lineHeight, NAVY);
    M5.Lcd.setTextColor(YELLOW);
  } else {
    M5.Lcd.setTextColor(WHITE);
  }
  M5.Lcd.setCursor(lineX + 10, yPos);
  M5.Lcd.printf("Seconds: %02d", seconds);
  
  // Instructions with smaller font
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(20, 220);
  M5.Lcd.println("A: Select  B: +/-  C: Start");
}

void displayCountdown() {
  M5.Lcd.clear();
  
  // Calculate hours, minutes, seconds from remaining
  int h = remainingSeconds / 3600;
  int m = (remainingSeconds % 3600) / 60;
  int s = remainingSeconds % 60;
  
  // Display time - bigger and centered
  M5.Lcd.setTextSize(5);
  M5.Lcd.setTextColor(GREEN);
  
  // Center the text horizontally (screen width 320)
  // For "00:00:00" with size 5: better estimate is ~220 pixels wide
  // Center: (320 - 220) / 2 = 50
  int textX = 50;
  int textY = 80; // Vertically centered
  
  M5.Lcd.setCursor(textX, textY);
  M5.Lcd.printf("%02d:%02d:%02d", h, m, s);
  
  // Instructions
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setCursor(20, 200);
  M5.Lcd.println("Hold any button 5s to disarm");
}

void displayDisarming() {
  // Only clear and redraw everything on first call
  if (!disarmingScreenInitialized) {
    M5.Lcd.clear();
    
    // Calculate hours, minutes, seconds from remaining
    int h = remainingSeconds / 3600;
    int m = (remainingSeconds % 3600) / 60;
    int s = remainingSeconds % 60;
    
    // Display paused time with larger font
    M5.Lcd.setTextSize(5);
    M5.Lcd.setTextColor(YELLOW);
    
    // Center the text horizontally (same as countdown)
    int textX = 50; // Centered, same as countdown
    int textY = 50;
    M5.Lcd.setCursor(textX, textY);
    M5.Lcd.printf("%02d:%02d:%02d", h, m, s);
    
    // Display disarming progress with better font
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(ORANGE);
    M5.Lcd.setCursor(50, 110);
    M5.Lcd.println("DISARMING...");
    
    // Draw progress bar border (static)
    M5.Lcd.drawRect(20, 150, 280, 25, YELLOW);
    
    // Draw text area background for countdown text
    M5.Lcd.fillRect(20, 185, 280, 15, BLACK);
    
    disarmingScreenInitialized = true;
  }
  
  // Always update progress bar and text
  unsigned long holdTime = millis() - disarmPressStart;
  int progress = (holdTime * 100) / DISARM_HOLD_TIME;
  if (progress > 100) progress = 100;
  
  // Clear and redraw only the progress bar fill
  M5.Lcd.fillRect(22, 152, 276, 21, BLACK); // Clear old progress
  M5.Lcd.fillRect(22, 152, (276 * progress) / 100, 21, YELLOW); // Draw new progress
  
  // Update countdown text
  M5.Lcd.fillRect(20, 185, 280, 15, BLACK); // Clear text area
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setCursor(20, 185);
  int remainingSecs = ((DISARM_HOLD_TIME - holdTime) / 1000) + 1;
  if (remainingSecs < 0) remainingSecs = 0;
  M5.Lcd.printf("Hold for %d more seconds...", remainingSecs);
}

void displayDisarmed() {
  M5.Lcd.clear();
  M5.Lcd.fillScreen(DARKGREEN);
  
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(WHITE);
  
  // Center "BOMB" - size 4, 4 chars = ~96px, center at (320-96)/2 = 112
  M5.Lcd.setCursor(112, 80);
  M5.Lcd.println("BOMB");
  
  // Center "DISARMED" - size 4, 8 chars = ~192px, center at (320-192)/2 = 64
  M5.Lcd.setCursor(64, 130);
  M5.Lcd.println("DISARMED");
  
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(CYAN);
  // Center instruction text - "Press any button to return" is ~26 chars = ~156px, center at (320-156)/2 = 82
  M5.Lcd.setCursor(82, 200);
  M5.Lcd.println("Press any button to return");
}

void displayBoom() {
  // Flash red screen
  unsigned long currentTime = millis();
  if (currentTime - lastFlashTime > FLASH_INTERVAL) {
    boomFlashState = !boomFlashState;
    lastFlashTime = currentTime;
    
    if (boomFlashState) {
      M5.Lcd.fillScreen(RED);
      M5.Lcd.setTextSize(4);
      M5.Lcd.setTextColor(WHITE);
      // Center "BOOM!" - size 4, 5 chars = ~120px, center at (320-120)/2 = 100
      M5.Lcd.setCursor(100, 100);
      M5.Lcd.println("BOOM!");
    } else {
      M5.Lcd.fillScreen(BLACK);
    }
  }
}

void handleSetupButtons() {
  M5.update();
  
  if (M5.BtnA.wasPressed()) {
    // Cycle through hours, minutes, seconds
    playClickSound();
    setupSelection = (setupSelection + 1) % 3;
    displaySetup();
    delay(DEBOUNCE_TIME);
  }
  
  if (M5.BtnB.wasPressed()) {
    // Increment current selection
    playClickSound();
    if (setupSelection == 0) {
      hours = (hours + 1) % 24;
    } else if (setupSelection == 1) {
      minutes = (minutes + 1) % 60;
    } else if (setupSelection == 2) {
      seconds = (seconds + 1) % 60;
    }
    displaySetup();
    delay(DEBOUNCE_TIME);
  }
  
  if (M5.BtnB.pressedFor(500)) {
    // Hold B to decrement
    if (setupSelection == 0) {
      hours = (hours - 1 + 24) % 24;
    } else if (setupSelection == 1) {
      minutes = (minutes - 1 + 60) % 60;
    } else if (setupSelection == 2) {
      seconds = (seconds - 1 + 60) % 60;
    }
    displaySetup();
    delay(100);
  }
  
  if (M5.BtnC.wasPressed()) {
    // Start countdown
    playClickSound();
    totalSeconds = hours * 3600 + minutes * 60 + seconds;
    if (totalSeconds > 0) {
      // Store initial values for restoration after disarm
      initialHours = hours;
      initialMinutes = minutes;
      initialSeconds = seconds;
      remainingSeconds = totalSeconds;
      currentState = COUNTDOWN;
      lastUpdate = millis();
      // Reset beep tracking
      lastMinuteBeep = 0;
      last10SecondBeep = 0;
      lastSecondBeep = 0;
      lastFastBeep = 0;
      longBeepPlaying = false;
      displayCountdown();
    }
    delay(DEBOUNCE_TIME);
  }
}

void handleCountdownButtons() {
  M5.update();
  
  // Check if any button is pressed for disarming
  bool anyButtonPressed = M5.BtnA.isPressed() || M5.BtnB.isPressed() || M5.BtnC.isPressed();
  int pressedButtonId = -1;
  
  if (M5.BtnA.isPressed()) pressedButtonId = 0;
  else if (M5.BtnB.isPressed()) pressedButtonId = 1;
  else if (M5.BtnC.isPressed()) pressedButtonId = 2;
  
  if (anyButtonPressed && !disarmButtonPressed) {
    // Just started pressing any button
    playClickSound();
    disarmButtonPressed = true;
    disarmButtonId = pressedButtonId;
    disarmPressStart = millis();
    disarmingScreenInitialized = false; // Reset flag for new disarm attempt
    currentState = DISARMING;
    displayDisarming();
  } else if (!anyButtonPressed && disarmButtonPressed) {
    // Released before 10 seconds
    disarmButtonPressed = false;
    disarmButtonId = -1;
    disarmingScreenInitialized = false; // Reset flag
    currentState = COUNTDOWN;
    displayCountdown();
  } else if (anyButtonPressed && disarmButtonPressed) {
    // Still holding - check if same button or different button
    if (pressedButtonId == disarmButtonId) {
      // Same button still held
      unsigned long holdTime = millis() - disarmPressStart;
      if (holdTime >= DISARM_HOLD_TIME) {
        // Successfully disarmed!
        disarmButtonPressed = false;
        disarmButtonId = -1;
        disarmingScreenInitialized = false; // Reset flag
        currentState = DISARMED;
        displayDisarmed();
      } else {
        // Still disarming
        displayDisarming();
      }
    } else {
      // Different button pressed - reset disarm
      disarmButtonPressed = false;
      disarmButtonId = -1;
      disarmingScreenInitialized = false; // Reset flag
      currentState = COUNTDOWN;
      displayCountdown();
    }
  }
  
  // Update countdown if not disarming
  if (currentState == COUNTDOWN) {
    unsigned long currentTime = millis();
    
    // Handle countdown beeps based on time remaining (check every loop iteration)
    if (remainingSeconds > 60) {
      // More than 1 minute: beep every minute (at 120, 180, 240, etc.)
      if (remainingSeconds % 60 == 0 && remainingSeconds > 60 && currentTime - lastMinuteBeep >= 1000) {
        playCountdownBeep();
        lastMinuteBeep = currentTime;
      }
    } else if (remainingSeconds >= 10) {
      // Less than 1 minute but >= 10 seconds: beep every 10 seconds (at 50, 40, 30, 20, 10)
      if (remainingSeconds % 10 == 0 && currentTime - last10SecondBeep >= 1000) {
        playCountdownBeep();
        last10SecondBeep = currentTime;
      }
    } else if (remainingSeconds >= 3) {
      // Less than 10 seconds but >= 3 seconds: beep every second
      if (currentTime - lastSecondBeep >= 1000) {
        playCountdownBeep();
        lastSecondBeep = currentTime;
      }
    } else if (remainingSeconds > 0) {
      // Less than 3 seconds: beep 3 times per second (every ~333ms)
      if (currentTime - lastFastBeep >= 333) {
        playCountdownBeep();
        lastFastBeep = currentTime;
      }
    }
    
    // Update timer every second
    if (currentTime - lastUpdate >= 1000) {
      if (remainingSeconds > 0) {
        remainingSeconds--;
        lastUpdate = currentTime;
        displayCountdown();
      } else {
        // Timer reached zero!
        // Start long beep
        longBeepPlaying = true;
        longBeepStartTime = millis();
        M5.Speaker.tone(LONG_BEEP_FREQUENCY, LONG_BEEP_DURATION);
        currentState = BOOM;
        lastFlashTime = millis();
      }
    }
  }
}

void handleDisarmedButtons() {
  M5.update();
  
  if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
    // Return to setup with retained initial time values
    playClickSound();
    currentState = SETUP;
    hours = initialHours;
    minutes = initialMinutes;
    seconds = initialSeconds;
    setupSelection = 0;
    displaySetup();
    delay(DEBOUNCE_TIME);
  }
}

void handleBoomButtons() {
  M5.update();
  
  // Check if long beep is still playing
  if (longBeepPlaying) {
    unsigned long currentTime = millis();
    if (currentTime - longBeepStartTime >= LONG_BEEP_DURATION) {
      // Long beep finished
      longBeepPlaying = false;
      M5.Speaker.mute();
    }
  }
  
  if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) {
    // Return to menu with retained initial time values
    // Stop long beep if playing
    if (longBeepPlaying) {
      M5.Speaker.mute();
      longBeepPlaying = false;
    }
    playClickSound();
    currentState = SETUP;
    hours = initialHours;
    minutes = initialMinutes;
    seconds = initialSeconds;
    setupSelection = 0;
    displaySetup();
    delay(DEBOUNCE_TIME);
  } else {
    // Continue flashing
    displayBoom();
  }
}

void setup() {
  M5.begin();
  Serial.begin(115200);
  
  M5.Lcd.clear();
  M5.Lcd.setTextColor(WHITE);
  
  // Initialize setup screen
  displaySetup();
  
  Serial.println("Airsoft Bomb Timer initialized");
}

void loop() {
  switch (currentState) {
    case SETUP:
      handleSetupButtons();
      break;
      
    case COUNTDOWN:
      handleCountdownButtons();
      break;
      
    case DISARMING:
      handleCountdownButtons(); // Same logic, just different display
      break;
      
    case DISARMED:
      handleDisarmedButtons();
      break;
      
    case BOOM:
      handleBoomButtons();
      break;
  }
  
  delay(10); // Small delay to prevent excessive CPU usage
}
