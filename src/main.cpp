// Ruby Zhang QZ2432, Conghui Duan CD3792, Mukund Ramakrishnan MR5950

#include "mbed.h"
#include <cstring>
#include <cmath>

// =================================================
// * Embedded Sentry: Gesture-Based Security System *
// =================================================

// --- Register Addresses and Configuration Values ---
#define CTRL_REG1 0x20               
#define CTRL_REG1_CONFIG 0b01'10'1'1'1'1  // ODR=100Hz, Enable X/Y/Z axes
#define CTRL_REG4 0x23               
#define CTRL_REG4_CONFIG 0b0'0'01'0'00'0  // High-resolution, 2000dps
#define OUT_X_L 0x28

// --- Constants ---
#define SPI_FLAG 1
#define BUTTON_FLAG 2
#define MAX_SAMPLES 100
#define GESTURE_DURATION 3000
#define SAMPLE_INTERVAL 30
#define SIMILARITY_THRESHOLD 0.5
#define DEG_TO_RAD (17.5f * 0.0174532925199432957692236907684886f / 1000.0f)

// --- Data Structures ---
struct GestureData {
    float x[MAX_SAMPLES];
    float y[MAX_SAMPLES];
    float z[MAX_SAMPLES];
    int samples;
};

// --- Global Variables ---
EventFlags flags;
GestureData savedGesture;
bool gestureStored = false;
bool isRecordMode = true;  // Toggle between record and verify modes

// --- Hardware Objects ---
SPI spi(PF_9, PF_8, PF_7, PC_1, use_gpio_ssel);  // MOSI, MISO, SCLK, CS
InterruptIn userBtn(PA_0);    // USER_BUTTON
DigitalOut greenLed(PG_13);   // Green LED
DigitalOut redLed(PG_14);     // Red LED
Timer debounceTimer;

// --- Function Prototypes ---
void spi_cb(int event);
void initializeGyroscope();
void readGyroscopeData(float &gx, float &gy, float &gz);
void recordGesture();
void verifyGesture();
float calculateSimilarity(const GestureData& g1, const GestureData& g2);

// --- Callback Functions ---
void spi_cb(int event) {
    flags.set(SPI_FLAG);
}

// --- Button Handler (ISR Context) ---
void handleButton() {
    if (debounceTimer.read_ms() > 200) {
        debounceTimer.reset();
        flags.set(BUTTON_FLAG);
    }
}

// --- Gyroscope Functions ---
void initializeGyroscope() {
    uint8_t write_buf[2], read_buf[2];

    // Configure SPI
    spi.format(8, 3);  // 8-bit data, mode 3
    spi.frequency(1'000'000);  // 1MHz

    // Configure CTRL_REG1
    write_buf[0] = CTRL_REG1;
    write_buf[1] = CTRL_REG1_CONFIG;
    spi.transfer(write_buf, 2, read_buf, 2, spi_cb);
    flags.wait_all(SPI_FLAG);

    // Configure CTRL_REG4
    write_buf[0] = CTRL_REG4;
    write_buf[1] = CTRL_REG4_CONFIG;
    spi.transfer(write_buf, 2, read_buf, 2, spi_cb);
    flags.wait_all(SPI_FLAG);
}

void readGyroscopeData(float &gx, float &gy, float &gz) {
    uint8_t write_buf[7], read_buf[7];
    write_buf[0] = OUT_X_L | 0x80 | 0x40;  // Read mode + auto-increment

    spi.transfer(write_buf, 7, read_buf, 7, spi_cb);
    flags.wait_all(SPI_FLAG);

    // Convert raw data
    int16_t raw_x = (((int16_t)read_buf[2]) << 8) | read_buf[1];
    int16_t raw_y = (((int16_t)read_buf[4]) << 8) | read_buf[3];
    int16_t raw_z = (((int16_t)read_buf[6]) << 8) | read_buf[5];

    // Convert to rad/s
    gx = raw_x * DEG_TO_RAD;
    gy = raw_y * DEG_TO_RAD;
    gz = raw_z * DEG_TO_RAD;

    // Teleplot visualization
    printf(">x_axis: %.2f\n", gx);
    printf(">y_axis: %.2f\n", gy);
    printf(">z_axis: %.2f\n", gz);
}

// --- Gesture Functions ---
void recordGesture() {
    printf("Recording gesture...\n");
    greenLed = 1;  // Indicate recording mode
    redLed = 0;
    
    memset(&savedGesture, 0, sizeof(GestureData));
    Timer recordTimer;
    recordTimer.start();
    int sampleCount = 0;
    
    while (recordTimer.read_ms() < GESTURE_DURATION && sampleCount < MAX_SAMPLES) {
        readGyroscopeData(
            savedGesture.x[sampleCount],
            savedGesture.y[sampleCount],
            savedGesture.z[sampleCount]
        );
        sampleCount++;
        ThisThread::sleep_for(SAMPLE_INTERVAL);
    }
    
    savedGesture.samples = sampleCount;
    gestureStored = true;
    greenLed = 0;
    
    printf("Gesture recorded with %d samples\n", sampleCount);
}

void verifyGesture() {
    if (!gestureStored) {
        printf("No gesture stored! Please record a gesture first.\n");
        redLed = 1;
        ThisThread::sleep_for(1000);
        redLed = 0;
        isRecordMode = true;  // Switch back to record mode
        return;
    }

    printf("Verifying gesture...\n");
    greenLed = 1;  // Indicate verification in progress
    
    GestureData currentGesture;
    memset(&currentGesture, 0, sizeof(GestureData));
    Timer verifyTimer;
    verifyTimer.start();
    int sampleCount = 0;
    
    while (verifyTimer.read_ms() < GESTURE_DURATION && sampleCount < MAX_SAMPLES) {
        readGyroscopeData(
            currentGesture.x[sampleCount],
            currentGesture.y[sampleCount],
            currentGesture.z[sampleCount]
        );
        sampleCount++;
        ThisThread::sleep_for(SAMPLE_INTERVAL);
    }
    
    currentGesture.samples = sampleCount;
    greenLed = 0;
    
    float similarity = calculateSimilarity(savedGesture, currentGesture);
    printf("Similarity: %.2f\n", similarity);
    
    if (similarity > SIMILARITY_THRESHOLD) {
        printf("Access granted!\n");
        for (int i = 0; i < 3; i++) {
            greenLed = 1;
            ThisThread::sleep_for(200);
            greenLed = 0;
            ThisThread::sleep_for(200);
        }
    } else {
        printf("Access denied!\n");
        redLed = 1;
        ThisThread::sleep_for(1000);
        redLed = 0;
    }
}

float calculateSimilarity(const GestureData& g1, const GestureData& g2) {
    float totalDiff = 0;
    int minSamples = min(g1.samples, g2.samples);
    
    for (int i = 0; i < minSamples; i++) {
        float dx = g1.x[i] - g2.x[i];
        float dy = g1.y[i] - g2.y[i];
        float dz = g1.z[i] - g2.z[i];
        
        totalDiff += sqrt(dx*dx + dy*dy + dz*dz);
    }
    
    float avgDiff = totalDiff / minSamples;
    return 1.0f / (1.0f + avgDiff);  // Convert difference to similarity score
}

int main() {
    printf("Embedded Sentry - Starting...\n");
    
    // Initialize system
    initializeGyroscope();
    debounceTimer.start();
    
    // Ensure LEDs are off at startup
    greenLed = 0;
    redLed = 0;
    
    // Configure button interrupt
    userBtn.rise(handleButton);
    
    // Main loop - Process flags in thread context
    while (true) {
        uint32_t flags_read = flags.wait_any(BUTTON_FLAG);
        
        if (flags_read & BUTTON_FLAG) {
            if (isRecordMode) {
                recordGesture();
            } else {
                verifyGesture();
            }
            isRecordMode = !isRecordMode;  // Toggle mode after each operation
        }
        
        ThisThread::sleep_for(100);
    }
}