#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MPU6050.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define MPU6050_ADDRESS 0x68

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
MPU6050 mpu;

// Road boundaries
int baseLeft = 20;
int baseRight = SCREEN_WIDTH - 20;
int roadLeft = baseLeft;
int roadRight = baseRight;
int frameCount = 0;
bool gameRunning = true;

// Stick man's initial position
int stickManX = (baseLeft + baseRight) / 2;
int stickManY = SCREEN_HEIGHT - 30;

// Obstacle variables
int obstacleGap = 10;
int movingObstacleX = (baseLeft + baseRight) / 2;
int movingObstacleY = 0;
int horizontalObstacleX = baseLeft + 10;
int horizontalObstacleY = random(10, SCREEN_HEIGHT - 10);
int horizontalObstacleDirection = 1;

// Score and high score tracking
int score = 0;
int highScore = 0;
unsigned long startTime;

void setup() {
    Serial.begin(115200);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed");
        while (true);
    }
    display.clearDisplay();

    Wire.begin();
    mpu.initialize();
    if (!mpu.testConnection()) {
        Serial.println("Failed to connect to MPU6050");
        while (true);
    }

    startTime = millis(); // Start the game timer
}

void loop() {
    if (gameRunning) {
        int16_t ax, ay, az;
        mpu.getAcceleration(&ax, &ay, &az);

        // Map accelerometer values to stick man position
        stickManX = map(ay, 17000, -17000, baseLeft + 10, SCREEN_WIDTH - 10);
        stickManY = map(az, 17000, -17000, SCREEN_HEIGHT - 10, 10);

        // Draw road, obstacles, and stick man
        drawBoundaryAndRoad();
        stickManX = constrain(stickManX, roadLeft + 10, roadRight - 10);
        stickManY = constrain(stickManY, 10, SCREEN_HEIGHT - 10);
        drawStickMan(stickManX, stickManY);
        updateMovingObstacle();
        drawMovingObstacle();
        updateHorizontalObstacle();
        drawHorizontalObstacle();

        display.display();

        // Check for game over conditions
        if (checkBoundaryCrossing() || checkCollisionWithObstacles()) {
            gameReset();
        }

        delay(10);
        display.clearDisplay(); // Clear screen for next frame
        frameCount++;
    }
}

// Function to draw the road
void drawBoundaryAndRoad() {
    for (int i = 0; i < SCREEN_HEIGHT; i += 5) {
        int waveOffsetLeft = sin((i + frameCount) * 0.2) * 3;
        int waveOffsetRight = sin((i + frameCount + 50) * 0.2) * 3;

        roadLeft = baseLeft + waveOffsetLeft;
        roadRight = baseRight - waveOffsetRight;

        display.drawPixel(roadLeft, i, SSD1306_WHITE);
        display.drawPixel(roadRight, i, SSD1306_WHITE);
    }

    drawObstacles();
    drawWaveStripes();
}

// Function to draw obstacles (trees, shrubs, grass)
void drawObstacles() {
    for (int i = 0; i < SCREEN_HEIGHT; i += obstacleGap) {
        if (i % 20 == 0) {
            display.fillRect(baseLeft - 5, i, 10, 5, SSD1306_WHITE); // Trees or shrubs on left
            display.fillRect(baseRight - 5, i, 10, 5, SSD1306_WHITE); // Trees or shrubs on right
        }
    }
}

// Function to draw wave stripes outside the road boundaries
void drawWaveStripes() {
    int stripeHeight = 5;w
    int frequency = 10;

    for (int y = 0; y < SCREEN_HEIGHT; y += stripeHeight) {
        int waveOffset = sin((y + frameCount) * 0.15) * frequency;
        int xEnd = baseLeft - waveOffset - 5;
        display.drawLine(xEnd, y, xEnd, y + stripeHeight - 1, SSD1306_WHITE);
    }

    for (int y = 0; y < SCREEN_HEIGHT; y += stripeHeight) {
        int waveOffsetRight = sin((y + frameCount + 30) * 0.15) * frequency;
        int xEndRight = baseRight + waveOffsetRight + 5;
        display.drawLine(xEndRight, y, xEndRight, y + stripeHeight - 1, SSD1306_WHITE);
    }
}

// Function to update and reset the moving vertical obstacle
void updateMovingObstacle() {
    movingObstacleY += 1; // Move obstacle downwards

    if (movingObstacleY > SCREEN_HEIGHT) {
        movingObstacleY = 0;
        movingObstacleX = random(roadLeft + 10, roadRight - 10); // Randomize X position within road boundaries
    }
}

// Function to draw the moving vertical obstacle
void drawMovingObstacle() {
    display.fillRect(movingObstacleX - 3, movingObstacleY - 3, 6, 6, SSD1306_WHITE); // Draw a 6x6 square
}

// Function to update the horizontal obstacle's position and reset it at a random position when reaching the boundary
void updateHorizontalObstacle() {
    horizontalObstacleX += horizontalObstacleDirection;

    // Reset to a new random position along the left side when it reaches the right boundary
    if (horizontalObstacleX >= roadRight - 10) {
        horizontalObstacleX = roadLeft + 10;
        horizontalObstacleY = random(10, SCREEN_HEIGHT - 10); // Randomize Y position
    }
}

// Function to draw the moving horizontal obstacle
void drawHorizontalObstacle() {
    display.fillRect(horizontalObstacleX - 3, horizontalObstacleY - 3, 6, 6, SSD1306_WHITE); // Draw a 6x6 square
}

// Function to check if the stick man has crossed the road boundaries
bool checkBoundaryCrossing() {
    return (stickManX <= roadLeft + 10 || stickManX >= roadRight - 10);
}

// Function to check if the stick man collides with any obstacles
bool checkCollisionWithObstacles() {
    // Check for collision with vertical obstacle
    if (abs(stickManX - movingObstacleX) < 6 && abs(stickManY - movingObstacleY) < 6) {
        return true;
    }

    // Check for collision with horizontal obstacle
    if (abs(stickManX - horizontalObstacleX) < 6 && abs(stickManY - horizontalObstacleY) < 6) {
        return true;
    }

    return false;
}

// Function to reset the game
void gameReset() {
    gameRunning = false;

    // Calculate score based on time played
    unsigned long elapsedTime = (millis() - startTime) / 1000;
    score = 10 * elapsedTime;

    // Update high score if the current score is higher
    if (score > highScore) {
        highScore = score;
    }

    // Display "Game Over", score, and high score
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 10);
    display.print("Game Over");

    display.setTextSize(1);
    display.setCursor(10, 30);
    display.print("Score: ");
    display.print(score);

    display.setCursor(10, 45);
    display.print("High Score: ");
    display.print(highScore);

    display.display();
    delay(2000);

    // Reset game variables
    gameRunning = true;
    score = 0;
    startTime = millis(); // Reset start time
    stickManX = (baseLeft + baseRight) / 2;
    stickManY = SCREEN_HEIGHT - 30;
    movingObstacleY = 0;
    movingObstacleX = random(roadLeft + 10, roadRight - 10); // Randomize vertical obstacle position
    horizontalObstacleX = roadLeft + 10;
    horizontalObstacleY = random(10, SCREEN_HEIGHT - 10); // Randomize horizontal obstacle position
    display.clearDisplay();
}

// Function to draw the stick man
void drawStickMan(int x, int y) {
    display.drawCircle(x, y - 6, 4, SSD1306_WHITE);
    display.drawLine(x, y - 2, x, y + 6, SSD1306_WHITE);
    display.drawLine(x - 5, y, x + 5, y, SSD1306_WHITE);
    display.drawLine(x, y + 6, x - 5, y + 12, SSD1306_WHITE);
    display.drawLine(x, y + 6, x + 5, y + 12, SSD1306_WHITE);
}
