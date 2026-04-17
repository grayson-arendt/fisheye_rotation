#!/usr/bin/env python3
import serial
import sys

CMD_SET_POSITION = 0x01
CMD_READ_POSITION = 0x02

def degrees_to_position(degrees):
    """Convert degrees (0-300) to servo position (0-1024)"""
    if degrees > 300:
        degrees = 300
    return int((degrees * 1024) / 300)

def position_to_degrees(position):
    """Convert servo position (0-1024) to degrees (0-300)"""
    return int((position * 300) / 1024)

def main():
    port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyACM0'
    
    try:
        ser = serial.Serial(port, 115200, timeout=0.5)
        ser.dtr = True
        ser.rts = True
    except serial.SerialException:
        print(f"Failed to connect to {port}")
        return
    
    print(f"Connected to {port}")
    print("Commands: <degrees 0-300>, 'R' (read position), 'q' (quit)\n")
    
    while True:
        try:
            cmd = input("> ").strip()
            if cmd.lower() == 'q':
                break
            
            if cmd.upper() == 'R':
                # Read position
                ser.write(bytes([CMD_READ_POSITION]))
                response = ser.read(3)  # [status, pos_high, pos_low]
                
                if len(response) == 3:
                    status = response[0]
                    position = (response[1] << 8) | response[2]
                    degrees = position_to_degrees(position)
                    
                    if status == 0:
                        print(f"Position: {degrees}° (raw: {position})")
                    else:
                        print(f"Error reading position (status={status})")
                else:
                    print("No response")
            else:
                # Set position
                try:
                    degrees = int(cmd)
                    if 0 <= degrees <= 300:
                        position = degrees_to_position(degrees)
                        
                        # Send: [CMD, POS_HIGH, POS_LOW]
                        packet = bytes([CMD_SET_POSITION, (position >> 8) & 0xFF, position & 0xFF])
                        ser.write(packet)
                        
                        response = ser.read(1)  # [status]
                        if len(response) == 1:
                            status = response[0]
                            if status == 0:
                                print(f"Set to {degrees}° (raw: {position})")
                            else:
                                print(f"Error setting position (status={status})")
                        else:
                            print("No response")
                    else:
                        print("Degrees must be 0-300")
                except ValueError:
                    print("Invalid command")
                
        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"Error: {e}")
    
    ser.close()

if __name__ == '__main__':
    main()
