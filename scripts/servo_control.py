#!/usr/bin/env python3
# Usage: python3 servo_control.py [port]
# Commands: <0-300> = set degrees, r = read position, q = quit
import serial
import sys

port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyACM0'
ser = serial.Serial(port, 115200, timeout=0.5)

while True:
    try:
        cmd = input("> ").strip()
        if cmd == 'q':
            break
        
        if cmd == 'r':  # Read position
            ser.write(bytes([0x02]))
            resp = ser.read(3)  # [status, pos_high, pos_low]
            if len(resp) == 3 and resp[0] == 0:
                pos = (resp[1] << 8) | resp[2]
                print(f"{pos * 300 // 1023}°")
        else:  # Set position (degrees 0-300)
            deg = int(cmd)
            pos = deg * 1023 // 300
            ser.write(bytes([0x01, pos >> 8, pos & 0xFF]))
            if ser.read(1)[0] == 0:
                print("OK")
    except (KeyboardInterrupt, EOFError):
        break
    except:
        pass

ser.close()
