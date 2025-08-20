import asyncio
from bleak import BleakScanner, BleakClient
import struct
import time

# UUIDs from the C code
# Service UUID: BLE_SVC_THYMIO_UUID16 -> 0xABF0
SERVICE_UUID = "0000abf0-0000-1000-8000-00805f9b34fb"
# Commands Characteristic UUID -> 0xABF1
CMD_CHARACTERISTIC_UUID = "0000abf1-0000-1000-8000-00805f9b34fb"
# Sensors stream Characteristic UUID -> 0xABF2
SENSORS_STREAM_CHARACTERISTIC_UUID = "0000abf2-0000-1000-8000-00805f9b34fb"
# Python Characteristic UUID -> 0xABF3
PYTHON_CHARACTERISTIC_UUID = "0000abf3-0000-1000-8000-00805f9b34fb"

# Packet lengths
CMD_WRITE_OTHERS_ACTUATORS = 0x02
CMD_WRITE_OTHERS_ACTUATORS_LEN = 8
CMD_WRITE_MOST_ACTUATORS_LEN = 26 # Length of the command packet for actuators
STREAM_WRITE_STATE_LEN = 2
STREAM_NOTIFY_MOST_SENSORS = 0x01
STREAM_NOTIFY_MOST_SENSORS_LEN = 39 # Length of the response packet from all sensors
STREAM_NOTIFY_OTHERS_SENSORS = 0x02

# Global variable to store the last notification time
last_notification_time = 0.0

# Callback for receiving notifications
def notification_handler(sender, data):
    """
    Handles incoming data notifications from the BLE device.
    It unpacks the data to extract the proximity sensor values and calculates the refresh rate.
    """
    global last_notification_time
    current_time = time.time()

    #print("Notification " + str(sender))

    refresh_rate_hz = 0.0
    if last_notification_time != 0.0:
        time_diff = current_time - last_notification_time
        #print("time_diff = " + str(time_diff))
        if time_diff > 0:
            refresh_rate_hz = 1.0 / time_diff
    
    last_notification_time = current_time

    if data[0] == STREAM_NOTIFY_OTHERS_SENSORS:
        color_raw = struct.unpack('<hhhh', data[1:9])
        color_detected = data[9];
        ground_ambient = struct.unpack('<HH', data[10:14])
        ground_reflected = struct.unpack('<HH', data[14:18])
        angle_degrees = struct.unpack('<h', data[18:20])
        event_flags = data[20]
        mot_speed = struct.unpack('<hh', data[21:25])
        mot_pwm = struct.unpack('<hh', data[25:29])
        batt_volt = struct.unpack('<H', data[29:31])
        print(f"color_raw: {color_raw}")
        print(f"color_detected: {color_detected}")
        print(f"ground_ambient: {ground_ambient}")
        print(f"ground_reflected: {ground_reflected}")
        print(f"angle_degrees: {angle_degrees}")
        print(f"event_flags: {event_flags}")
        print(f"mot_speed: {mot_speed}")
        print(f"mot_pwm: {mot_pwm}")
        print(f"batt_volt: {batt_volt}")
        print()
    else:
        print(f"[{current_time:.2f}] Received data of unexpected length: {len(data)} bytes")


async def find_thymio_device():
    """
    Scans for BLE devices and returns the first one with a name starting with "THYMIO".
    """
    print("Searching for a device named 'THYMIO'...")
    devices = await BleakScanner.discover()
    for d in devices:
        if d.name and d.name.startswith("THYMIO"):
            print(f"Found Thymio device: {d.name} with address {d.address}")
            return d.address
    return None

async def list_characteristics(client):
    """
    Lists all services and characteristics of the connected BLE device.
    """
    print("Discovering services and characteristics...")
    services = client.services
    for service in services:
        print(f"\nService UUID: {service.uuid}")
        print(f"  Description: {service.description}")
        for char in service.characteristics:
            properties = ",".join(char.properties)
            print(f"  - Characteristic UUID: {char.uuid}")
            print(f"    Description: {char.description}")
            print(f"    Properties: {properties}")

async def main():
    """
    Connects to the Thymio BLE device, continuously changes its RGB color,
    sets motor speeds for pivoting, and prints proximity sensor data.
    It will run for 15 seconds before disconnecting and exiting.
    """
    ble_address = await find_thymio_device()
    if not ble_address:
        print("Could not find a Thymio device. Exiting.")
        return

    print(f"Connecting to {ble_address}...")

    cmd_actuators = bytearray([0] * CMD_WRITE_OTHERS_ACTUATORS_LEN)
    cmd_notif = bytearray([0] * STREAM_WRITE_STATE_LEN)

    test_actuators_state = 0

    try:
        async with BleakClient(ble_address) as client:
            print(f"Connected: {client.is_connected}")

            #await list_characteristics(client)
            
            # Subscribe to the sensors stream characteristic for notifications
            await client.start_notify(SENSORS_STREAM_CHARACTERISTIC_UUID, notification_handler)
            print("Subscribed to notifications from SENOSRS STREAM characteristic.")

            cmd_notif[0] = 0x01 # setup notification command
            cmd_notif[1] = 0x02 # enable others sensors stream
            await client.write_gatt_char(SENSORS_STREAM_CHARACTERISTIC_UUID, cmd_notif, response=True)

            # Define motor speeds for pivoting
            left_motor_speed = -200
            right_motor_speed = 200

            print(f"Setting motors to pivot (Left: {left_motor_speed}, Right: {right_motor_speed})")
            print("Continuously changing front-left LED color every second...")

            # Start time for the 15-second duration
            start_time = time.time()
            duration = 30 # seconds

            while True:
                # Check if "duration" seconds have passed
                if time.time() - start_time > duration:
                    print(f"Time limit of {duration} seconds reached. Disconnecting...")
                    break # Exit the loop

                # Prepare the command packet
                cmd_actuators[:] = [0] * CMD_WRITE_OTHERS_ACTUATORS_LEN # reset content
                cmd_actuators[0] = CMD_WRITE_OTHERS_ACTUATORS

                if test_actuators_state == 0: # RGB small bottom
                    # red half brightness
                    cmd_actuators[1] = 0x08  # Green, red
                    cmd_actuators[2] = 0x00  # Blue
                    print(f"Red half brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)

                    # green half brightness
                    cmd_actuators[1] = 0x80  # Green, red
                    cmd_actuators[2] = 0x00  # Blue
                    print(f"Green half brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)

                    # blue half brightness
                    cmd_actuators[1] = 0x00  # Green, red
                    cmd_actuators[2] = 0x08  # Blue
                    print(f"Blue half brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)          

                    # red full brightness
                    cmd_actuators[1] = 0x0F  # Green, red
                    cmd_actuators[2] = 0x00  # Blue
                    print(f"Red full brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)

                    # green full brightness
                    cmd_actuators[1] = 0xF0  # Green, red
                    cmd_actuators[2] = 0x00  # Blue
                    print(f"Green full brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)

                    # blue full brightness
                    cmd_actuators[1] = 0x00  # Green, red
                    cmd_actuators[2] = 0x0F  # Blue
                    print(f"Blue full brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)                                

                    # Off
                    cmd_actuators[1] = 0x00  # Green, red
                    cmd_actuators[2] = 0x00  # Blue
                    print(f"Off")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0) 

                    test_actuators_state = 1

                elif test_actuators_state == 1: # RGB small back
                    # red half brightness
                    cmd_actuators[3] = 0x08  # Green, red
                    cmd_actuators[4] = 0x00  # Blue
                    print(f"Red half brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)

                    # green half brightness
                    cmd_actuators[3] = 0x80  # Green, red
                    cmd_actuators[4] = 0x00  # Blue
                    print(f"Green half brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)

                    # blue half brightness
                    cmd_actuators[3] = 0x00  # Green, red
                    cmd_actuators[4] = 0x08  # Blue
                    print(f"Blue half brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)          

                    # red full brightness
                    cmd_actuators[3] = 0x0F  # Green, red
                    cmd_actuators[4] = 0x00  # Blue
                    print(f"Red full brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)

                    # green full brightness
                    cmd_actuators[3] = 0xF0  # Green, red
                    cmd_actuators[4] = 0x00  # Blue
                    print(f"Green full brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)

                    # blue full brightness
                    cmd_actuators[3] = 0x00  # Green, red
                    cmd_actuators[4] = 0x0F  # Blue
                    print(f"Blue full brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)                                

                    # Off
                    cmd_actuators[3] = 0x00  # Green, red
                    cmd_actuators[4] = 0x00  # Blue
                    print(f"Off")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0) 

                    test_actuators_state = 2

                elif test_actuators_state == 2: # buttons leds
                    # half brightness
                    cmd_actuators[5] = 0x88
                    cmd_actuators[6] = 0x88
                    print(f"Buttons leds half brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)

                    # full brightness
                    cmd_actuators[5] = 0xFF
                    cmd_actuators[6] = 0xFF
                    print(f"Buttons leds half brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)

                    # off
                    cmd_actuators[5] = 0x00
                    cmd_actuators[6] = 0x00
                    print(f"Buttons leds off")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)

                    test_actuators_state = 3

                elif test_actuators_state == 3: # receiver and mic led
                    # receiver half brightness
                    cmd_actuators[7] = 0x08
                    print(f"Receiver led half brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)

                    # receiver full brightness
                    cmd_actuators[7] = 0x0F
                    print(f"Receiver led full brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)

                    # mic led on
                    cmd_actuators[7] = 0x10
                    print(f"Mic led on")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)

                    # mic led off
                    cmd_actuators[7] = 0x00
                    print(f"Mic led off")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.0)

                    test_actuators_state = 0


            cmd_notif[0] = 0x01 # setup notification command
            cmd_notif[1] = 0x00 # disable sensors stream
            await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_notif, response=True)

    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        # Ensure notifications are stopped and client is disconnected if an error occurs or the script exits
        if 'client' in locals() and client.is_connected:
            await client.stop_notify(SENSORS_STREAM_CHARACTERISTIC_UUID)
            print("Stopped receiving notifications.")
            # BleakClient's async with statement handles disconnection automatically upon exiting the block
            # No explicit client.disconnect() is needed here if using 'async with'
        print("Script finished.")

if __name__ == "__main__":
    asyncio.run(main())
