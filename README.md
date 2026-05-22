# UART Frame Parser (Embedded C)

## Overview
This project implements a UART frame parser in C using a state machine.  
It simulates how embedded systems receive data byte-by-byte over UART.

The parser supports:
- Frame detection
- Payload extraction
- Checksum validation
- Timeout handling
- Error recovery

---

## Frame Format

[SOF][CMD][LEN][PAYLOAD][CHECKSUM]

| Field    | Size | Description                      |
|----------|------|----------------------------------|
| SOF      | 1    | Start of Frame (0xAA)           |
| CMD      | 1    | Command byte                    |
| LEN      | 1    | Payload length                  |
| PAYLOAD  | N    | Data bytes                      |
| CHECKSUM | 1    | XOR of CMD, LEN, PAYLOAD        |

---

## State Machine

WAIT_FOR_SOF → RECEIVE_CMD → RECEIVE_LEN → RECEIVE_PAYLOAD → RECEIVE_CHECKSUM

---

## Features

- State machine based parsing
- Inter-byte timeout detection
- Automatic reset on error
- XOR checksum verification
- Recovery from corrupted frames
- Supports multiple frames

---

## Timeout Handling

If the delay between bytes exceeds the configured timeout:
- Current frame is discarded
- Parser resets automatically
- New frame detection starts

---

## Test Cases

### Test 1: Clean Frame
Valid frame → FRAME OK

### Test 2: Timeout Recovery
Timeout occurs → parser resets → next frame parsed correctly

### Test 3: Multiple Frames
Two frames received → both parsed successfully

### Test 4: Timeout Disabled
No timeout → corrupted data → CHECKSUM ERROR

---

## Compilation (Windows - MinGW)

gcc embed.c -o embed -mconsole

Run:

./embed

---

## Sample Output

t= 30ms byte=0x02 -> FRAME OK CMD=0x01 LEN=3 PAYLOAD=[10 20 30]  
t=200ms byte=0xAA -> TIMEOUT -- parser reset  
t=220ms byte=0x7B -> FRAME OK CMD=0x05 LEN=1 PAYLOAD=[7F]

---

## Project Structure

embed.c      → main source code  
README.md    → documentation  

---

## Key Learnings

- UART communication handling
- State machine design
- Timeout handling in embedded systems
- Error recovery techniques
- Real-time data processing

---

## Applications

- IoT devices
- Automotive systems
- Sensor communication
- Industrial automation
- Embedded firmware development

---

## Author

Siddhi Gaikwad

---


