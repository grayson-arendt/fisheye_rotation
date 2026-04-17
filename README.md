# Fisheye Rotation - SCS0009 Servo Control

## Hardware
- MCU: STM32G0B1
- Servo: SCS0009 (Feetech SCSCL series)
- UART: USART1 @ 1Mbaud (half-duplex on PB6)
- USB: CDC for PC communication

## Current Status
**ISSUE: USB CDC transmission not working**
- Device enumerates as /dev/ttyACM0
- Can connect with serial terminal
- **NO data being transmitted from MCU to PC**
- Receiving data from PC works (CDC_Receive_FS is called)
- CDC_Transmit_FS appears to fail silently

## Protocol
SCS0009 uses Feetech SCS protocol:
- Packet: `[0xFF, 0xFF, ID, LEN, CMD, ADDR, DATA..., CHECKSUM]`
- Checksum: `~(ID + LEN + CMD + ADDR + DATA)`
- **Byte order: BIG-ENDIAN** (high byte first)
- Instructions: READ (0x02), WRITE (0x03)

## Register Addresses
```c
#define SCS0009_TORQUE_ENABLE      40
#define SCS0009_GOAL_POSITION_L    42  // Big-endian pair
#define SCS0009_GOAL_POSITION_H    43
#define SCS0009_GOAL_TIME_L        44
#define SCS0009_GOAL_TIME_H        45
#define SCS0009_GOAL_SPEED_L       46
#define SCS0009_GOAL_SPEED_H       47
#define SCS0009_EEPROM_LOCK        48
#define SCS0009_PRESENT_POSITION_L 56
#define SCS0009_PRESENT_POSITION_H 57
```

## Files Modified
- `Core/Inc/servo.h` - Servo API
- `Core/Src/servo.c` - SCS0009 driver (half-duplex UART)
- `Core/Src/main.c` - USB command processing
- `USB_Device/App/usbd_cdc_if.c` - CDC RX callback (minimal changes)
- `scripts/servo_control.py` - Python control script
- `Makefile` - Added servo.c to build

## Commands (when USB works)
- Send degrees `0-300` to move
- Send `R` to read position
- Firmware continuously sends heartbeat: `ALIVE:xxxxx`

## Next Steps to Debug
1. Check if CDC_Transmit_FS returns USBD_BUSY
2. Verify USB device state is CONFIGURED
3. Check if TxState is blocking transmission
4. Try USB CDC loopback test
