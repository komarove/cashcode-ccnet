#include <Arduino.h>

// Constants for CashCode protocol commands
const uint8_t CASHCODE_RESET[] = {0x02, 0x03, 0x06, 0x30, 0x41, 0xB3};
const uint8_t CASHCODE_ACK[] = {0x02, 0x03, 0x06, 0x00, 0xC2, 0x82};
const uint8_t CASHCODE_POLL[] = {0x02, 0x03, 0x06, 0x33, 0xDA, 0x81};
const uint8_t CASHCODE_ENABLE[] = {0x02, 0x03, 0x0C, 0x34, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x66, 0xC1};

// Timing constants
const uint32_t POLL_INTERVAL_MS = 500;
const uint32_t RESET_DELAY_MS = 5000;
const uint32_t ENABLE_DELAY_MS = 3000;
const uint32_t POLL_RESPONSE_DELAY_MS = 100;
const uint32_t MONEY_TIMEOUT_MS = 15000;

// UART2 pin definitions for ESP32
const uint8_t RX_PIN = 16;
const uint8_t TX_PIN = 17;

// Protocol constants
const uint8_t START_BYTE = 0x02;
const uint8_t ADDRESS = 0x03;
const uint8_t EXPECTED_PACKET_SIZE = 6;

// Command codes
const uint8_t CMD_MONEY_INSERTED = 0x14;
const uint8_t CMD_MONEY_ACCEPTED = 0x81;

// Error codes
const uint8_t ERR_DEFECTIVE_MOTOR = 0x10;
const uint8_t ERR_SENSOR_PROBLEM = 0x11;
const uint8_t ERR_BILL_JAM = 0x12;
const uint8_t ERR_STACKER_OPEN = 0x13;
const uint8_t ERR_STACKER_FULL = 0x14;
const uint8_t ERR_STACKER_PROBLEM = 0x15;
const uint8_t ERR_CHEATED = 0x1C;
const uint8_t ERR_PAUSE = 0x1D;
const uint8_t ERR_GENERIC_FAILURE = 0x1E;
const uint8_t ERR_INVALID_COMMAND = 0x1F;
const uint8_t ERR_ESCROW_POSITION = 0x20;
const uint8_t ERR_POWER_UP = 0x30;
const uint8_t ERR_POWER_UP_BILL_VALIDATOR = 0x41;
const uint8_t ERR_POWER_UP_BILL_STACKER = 0x42;
const uint8_t ERR_POWER_UP_BILL_ESCROW = 0x43;
const uint8_t ERR_STACKER_REMOVED = 0x44;
const uint8_t ERR_STACKER_INSERTED = 0x45;
const uint8_t ERR_DROP_CASSETTE_OUT = 0x46;
const uint8_t ERR_DROP_CASSETTE_JAMMED = 0x47;
const uint8_t ERR_DROP_CASSETTE_FULL = 0x48;
const uint8_t ERR_DROP_CASSETTE_SENSOR = 0x49;

const uint8_t ERROR_CODES[] = {
    ERR_DEFECTIVE_MOTOR,
    ERR_SENSOR_PROBLEM,
    ERR_BILL_JAM,
    ERR_STACKER_OPEN,
    ERR_STACKER_FULL,
    ERR_STACKER_PROBLEM,
    ERR_CHEATED,
    ERR_PAUSE,
    ERR_GENERIC_FAILURE,
    ERR_INVALID_COMMAND,
    ERR_ESCROW_POSITION,
    ERR_POWER_UP,
    ERR_POWER_UP_BILL_VALIDATOR,
    ERR_POWER_UP_BILL_STACKER,
    ERR_POWER_UP_BILL_ESCROW,
    ERR_STACKER_REMOVED,
    ERR_STACKER_INSERTED,
    ERR_DROP_CASSETTE_OUT,
    ERR_DROP_CASSETTE_JAMMED,
    ERR_DROP_CASSETTE_FULL,
    ERR_DROP_CASSETTE_SENSOR
};

// Device states
enum class CashCodeState {
    INITIALIZING,
    IDLE,
    MONEY_INSERTED,
    MONEY_ACCEPTED
};

// Global variables
uint32_t lastPollTime = 0;
uint32_t moneyInsertTime = 0;
CashCodeState deviceState = CashCodeState::INITIALIZING;
bool stackerOpenReported = false; // Track if Stacker Open error was logged

void setup() {
    // Initialize Serial for debugging (UART0, via USB)
    Serial.begin(9600);
    while (!Serial) {
        ; // Wait for Serial to connect
    }
    
    // Initialize Serial2 for CashCode communication (UART2)
    Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
    
    // Wait for serial port to stabilize
    delay(RESET_DELAY_MS);
    
    initializeCashCode();
}

void loop() {
    // Handle periodic polling
    if (millis() - lastPollTime >= POLL_INTERVAL_MS || lastPollTime == 0) {
        processPollCycle();
        lastPollTime = millis();
    }
    
    // Check for money insertion timeout (no reset, just return to IDLE)
    if (moneyInsertTime > 0 && (millis() - moneyInsertTime > MONEY_TIMEOUT_MS)) {
        Serial.println("Money insertion timed out, returning to IDLE");
        moneyInsertTime = 0;
        deviceState = CashCodeState::IDLE;
    }
}

void initializeCashCode() {
    Serial.println("Initializing CashCode...");
    deviceState = CashCodeState::INITIALIZING;
    
    // Send reset command
    Serial2.write(CASHCODE_RESET, sizeof(CASHCODE_RESET));
    delay(RESET_DELAY_MS);
    
    // Send enable command
    Serial2.write(CASHCODE_ENABLE, sizeof(CASHCODE_ENABLE));
    delay(ENABLE_DELAY_MS);
    
    // Clear input buffer
    clearSerialBuffer();
    
    // Transition to IDLE state after initialization
    deviceState = CashCodeState::IDLE;
    stackerOpenReported = false; // Reset error reporting flag
    Serial.println("CashCode initialized, ready to accept bills");
}

void processPollCycle() {
    // Send poll command
    Serial2.write(CASHCODE_POLL, sizeof(CASHCODE_POLL));
    delay(POLL_RESPONSE_DELAY_MS);
    
    // Process response if available
    if (Serial2.available() >= EXPECTED_PACKET_SIZE) {
        processSerialResponse();
    }
}

void processSerialResponse() {
    uint8_t packet[EXPECTED_PACKET_SIZE];
    
    // Read header
    packet[0] = Serial2.read(); // Start byte
    packet[1] = Serial2.read(); // Address
    packet[2] = Serial2.read(); // Packet length
    
    // Validate header
    if (packet[0] != START_BYTE || packet[1] != ADDRESS) {
        clearSerialBuffer();
        Serial2.write(CASHCODE_ACK, sizeof(CASHCODE_ACK));
        Serial.println("Error: Invalid packet header");
        deviceState = CashCodeState::IDLE; // Return to IDLE on invalid packet
        return;
    }
    
    // Read remaining packet
    packet[3] = Serial2.read(); // Command
    packet[4] = Serial2.read(); // Data/CRC
    packet[5] = Serial2.read(); // CRC
    
    // Clear any remaining bytes
    clearSerialBuffer();
    
    // Process command based on state
    switch (packet[3]) {
        case CMD_MONEY_INSERTED:
            // Only process if in IDLE state and no recent Stacker Open error
            if (deviceState == CashCodeState::IDLE && !stackerOpenReported) {
                moneyInsertTime = millis();
                deviceState = CashCodeState::MONEY_INSERTED;
                Serial.println("Money inserted, waiting for acceptance");
            }
            break;
            
        case CMD_MONEY_ACCEPTED:
            if (deviceState == CashCodeState::MONEY_INSERTED) {
                Serial2.write(CASHCODE_ACK, sizeof(CASHCODE_ACK));
                handleMoneyAccepted(packet[4]);
                deviceState = CashCodeState::IDLE;
                moneyInsertTime = 0;
                stackerOpenReported = false; // Reset error flag after successful acceptance
            }
            break;
            
        default:
            // Check for error codes
            for (uint8_t errorCode : ERROR_CODES) {
                if (packet[3] == errorCode) {
                    handleError(errorCode);
                    break;
                }
            }
            break;
    }
    
    // Send ACK to confirm receipt
    Serial2.write(CASHCODE_ACK, sizeof(CASHCODE_ACK));
}

void handleMoneyAccepted(uint8_t valueCode) {
    // Log the accepted bill value
    // Example: valueCode 0x01 might correspond to 10 UAH, adjust based on your currency mapping
    Serial.print("Money accepted successfully, value code: 0x");
    Serial.print(valueCode, HEX);
    
    // Example mapping for Ukrainian Hryvnia (adjust based on your CashCode configuration)
    int billValue = 0;
    switch (valueCode) {
        case 0x01: billValue = 10; break;  // 10 UAH
        case 0x02: billValue = 20; break;  // 20 UAH
        case 0x03: billValue = 50; break;  // 50 UAH
        case 0x04: billValue = 100; break; // 100 UAH
        case 0x05: billValue = 200; break; // 200 UAH
        case 0x06: billValue = 500; break; // 500 UAH
        default: billValue = -1; break;    // Unknown value
    }
    
    if (billValue > 0) {
        Serial.print(", amount: ");
        Serial.print(billValue);
        Serial.println(" UAH");
    } else {
        Serial.println(", unknown denomination");
    }
}

void handleError(uint8_t errorCode) {
    // Handle Stacker Open error specifically
    if (errorCode == ERR_STACKER_OPEN) {
        if (!stackerOpenReported) {
            Serial.println("Error: Stacker Open");
            stackerOpenReported = true; // Log only once until reset or money accepted
        }
        return; // Do not reset for this error
    }
    
    // Reset the error flag for other errors
    stackerOpenReported = false;
    
    // Log other error messages
    switch (errorCode) {
        case ERR_DEFECTIVE_MOTOR:
            Serial.println("Error: Defective Motor");
            break;
        case ERR_SENSOR_PROBLEM:
            Serial.println("Error: Sensor Problem");
            break;
        case ERR_BILL_JAM:
            Serial.println("Error: Bill Jam");
            break;
        case ERR_STACKER_FULL:
            Serial.println("Error: Stacker Full");
            break;
        case ERR_STACKER_PROBLEM:
            Serial.println("Error: Stacker Problem");
            return; // Do not reset for this error
        case ERR_CHEATED:
            Serial.println("Error: Cheating Attempt Detected");
            break;
        case ERR_PAUSE:
            Serial.println("Error: Pause State");
            break;
        case ERR_GENERIC_FAILURE:
            Serial.println("Error: Generic Failure");
            break;
        case ERR_INVALID_COMMAND:
            Serial.println("Error: Invalid Command");
            break;
        case ERR_ESCROW_POSITION:
            Serial.println("Error: Bill in Escrow Position");
            break;
        case ERR_POWER_UP:
            Serial.println("Error: Power Up");
            break;
        case ERR_POWER_UP_BILL_VALIDATOR:
            Serial.println("Error: Power Up with Bill in Validator");
            break;
        case ERR_POWER_UP_BILL_STACKER:
            Serial.println("Error: Power Up with Bill in Stacker");
            break;
        case ERR_POWER_UP_BILL_ESCROW:
            Serial.println("Error: Power Up with Bill in Escrow");
            break;
        case ERR_STACKER_REMOVED:
            Serial.println("Error: Stacker Removed");
            break;
        case ERR_STACKER_INSERTED:
            Serial.println("Error: Stacker Inserted");
            break;
        case ERR_DROP_CASSETTE_OUT:
            Serial.println("Error: Drop Cassette Out of Position");
            break;
        case ERR_DROP_CASSETTE_JAMMED:
            Serial.println("Error: Drop Cassette Jammed");
            break;
        case ERR_DROP_CASSETTE_FULL:
            Serial.println("Error: Drop Cassette Full");
            break;
        case ERR_DROP_CASSETTE_SENSOR:
            Serial.println("Error: Drop Cassette Sensor Error");
            break;
    }

    // Perform reset for critical errors
    if (errorCode != ERR_STACKER_OPEN && errorCode != ERR_STACKER_PROBLEM) {
        resetCashCode();
        deviceState = CashCodeState::IDLE;
    }
}

void resetCashCode() {
    Serial.println("Resetting CashCode...");
    Serial2.write(CASHCODE_RESET, sizeof(CASHCODE_RESET));
    delay(RESET_DELAY_MS);
    Serial2.write(CASHCODE_ENABLE, sizeof(CASHCODE_ENABLE));
    delay(ENABLE_DELAY_MS);
    Serial2.write(CASHCODE_POLL, sizeof(CASHCODE_POLL));
    deviceState = CashCodeState::IDLE;
    stackerOpenReported = false;
}

void clearSerialBuffer() {
    while (Serial2.available()) {
        Serial2.read();
    }
}