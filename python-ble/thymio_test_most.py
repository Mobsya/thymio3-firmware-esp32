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
CMD_WRITE_MOST_ACTUATORS_LEN = 26 # Length of the command packet for actuators
STREAM_WRITE_STATE_LEN = 2
STREAM_NOTIFY_MOST_SENSORS = 0x01
STREAM_NOTIFY_MOST_SENSORS_LEN = 39 # Length of the response packet from all sensors

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

    if data[0] == STREAM_NOTIFY_MOST_SENSORS:
        color_h = (data[2]<<8) | data[1]
        color_s = data[3];
        color_v = data[4]
        ground_values = struct.unpack('<HH', data[5:9])
        acc_raw = struct.unpack('<hhh', data[9:15])
        gyro_raw = struct.unpack('<hhh', data[15:21])
        btn_state = data[21]
        mic_vol = (data[23]<<8) | data[22]
        proximity_values = struct.unpack('<HHHHHHH', data[24:38])
        tv_remote = data[38]
        print(f"HSV: {color_h}, {color_s}, {color_v}")
        print(f"Ground: {ground_values}")
        print(f"Acc raw: {acc_raw}")
        print(f"Gyro raw: {gyro_raw}")
        print(f"Buttons: {btn_state}")
        print(f"Volume: {mic_vol}")
        print(f"Prox: {proximity_values}")
        print(f"TV remote: {tv_remote}")
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

    cmd_actuators = bytearray([0] * CMD_WRITE_MOST_ACTUATORS_LEN)
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
            cmd_notif[1] = 0x01 # enable most sensors stream
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
                # Check if 15 seconds have passed
                if time.time() - start_time > duration:
                    print(f"Time limit of {duration} seconds reached. Disconnecting...")
                    break # Exit the loop

                # Prepare the command packet
                cmd_actuators[:] = [0] * CMD_WRITE_MOST_ACTUATORS_LEN # reset content
                cmd_actuators[0] = 0x01

                if test_actuators_state == 0: # circle leds
                    # half brightness
                    cmd_actuators[1] = 0x88
                    cmd_actuators[2] = 0x88
                    cmd_actuators[3] = 0x88
                    cmd_actuators[4] = 0x88
                    print(f"Circle leds half brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    # full brightness
                    cmd_actuators[1] = 0xFF
                    cmd_actuators[2] = 0xFF
                    cmd_actuators[3] = 0xFF
                    cmd_actuators[4] = 0xFF
                    print(f"Circle leds full brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    # off
                    cmd_actuators[1] = 0x00
                    cmd_actuators[2] = 0x00
                    cmd_actuators[3] = 0x00
                    cmd_actuators[4] = 0x00
                    print(f"Circle leds off")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    test_actuators_state = 1

                elif test_actuators_state == 1: # front lego leds
                    # half brightness
                    cmd_actuators[5] = 0x88
                    cmd_actuators[6] = 0x88
                    cmd_actuators[7] = 0x88
                    cmd_actuators[8] = 0x88
                    print(f"Front lego leds half brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    # full brightness
                    cmd_actuators[5] = 0xFF
                    cmd_actuators[6] = 0xFF
                    cmd_actuators[7] = 0xFF
                    cmd_actuators[8] = 0xFF
                    print(f"Front lego leds full brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    # off
                    cmd_actuators[5] = 0x00
                    cmd_actuators[6] = 0x00
                    cmd_actuators[7] = 0x00
                    cmd_actuators[8] = 0x00
                    print(f"Front lego leds off")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    test_actuators_state = 2

                elif test_actuators_state == 2: # back lego leds
                    # half brightness
                    cmd_actuators[9] = 0x88
                    cmd_actuators[10] = 0x88
                    cmd_actuators[11] = 0x88
                    cmd_actuators[12] = 0x88
                    print(f"Back lego leds half brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    # full brightness
                    cmd_actuators[9] = 0xFF
                    cmd_actuators[10] = 0xFF
                    cmd_actuators[11] = 0xFF
                    cmd_actuators[12] = 0xFF
                    print(f"Back lego leds full brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    # off
                    cmd_actuators[9] = 0x00
                    cmd_actuators[10] = 0x00
                    cmd_actuators[11] = 0x00
                    cmd_actuators[12] = 0x00
                    print(f"Back lego leds off")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    test_actuators_state = 3

                elif test_actuators_state == 3: # rgb leds
                    # red half brightness
                    # front left
                    cmd_actuators[13] = 0x08  # Green, red
                    cmd_actuators[14] = 0x00  # Blue
                    # front right
                    cmd_actuators[15] = 0x08  # Green, red
                    cmd_actuators[16] = 0x00  # Blue
                    # back left
                    cmd_actuators[17] = 0x08  # Green, red
                    cmd_actuators[18] = 0x00  # Blue
                    # back right
                    cmd_actuators[19] = 0x08  # Green, red
                    cmd_actuators[20] = 0x00  # Blue

                    print(f"Red half brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    # green half brightness
                    # front left
                    cmd_actuators[13] = 0x80  # Green, red
                    cmd_actuators[14] = 0x00  # Blue
                    # front right
                    cmd_actuators[15] = 0x80  # Green, red
                    cmd_actuators[16] = 0x00  # Blue
                    # back left
                    cmd_actuators[17] = 0x80  # Green, red
                    cmd_actuators[18] = 0x00  # Blue
                    # back right
                    cmd_actuators[19] = 0x80  # Green, red
                    cmd_actuators[20] = 0x00  # Blue

                    print(f"Green half brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    # blue half brightness
                    # front left
                    cmd_actuators[13] = 0x00  # Green, red
                    cmd_actuators[14] = 0x08  # Blue
                    # front right
                    cmd_actuators[15] = 0x00  # Green, red
                    cmd_actuators[16] = 0x08  # Blue
                    # back left
                    cmd_actuators[17] = 0x00  # Green, red
                    cmd_actuators[18] = 0x08  # Blue
                    # back right
                    cmd_actuators[19] = 0x00  # Green, red
                    cmd_actuators[20] = 0x08  # Blue

                    print(f"Blue half brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    # red full brightness
                    # front left
                    cmd_actuators[13] = 0x0F  # Green, red
                    cmd_actuators[14] = 0x00  # Blue
                    # front right
                    cmd_actuators[15] = 0x0F  # Green, red
                    cmd_actuators[16] = 0x00  # Blue
                    # back left
                    cmd_actuators[17] = 0x0F  # Green, red
                    cmd_actuators[18] = 0x00  # Blue
                    # back right
                    cmd_actuators[19] = 0x0F  # Green, red
                    cmd_actuators[20] = 0x00  # Blue

                    print(f"Red full brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    # green full brightness
                    # front left
                    cmd_actuators[13] = 0xF0  # Green, red
                    cmd_actuators[14] = 0x00  # Blue
                    # front right
                    cmd_actuators[15] = 0xF0  # Green, red
                    cmd_actuators[16] = 0x00  # Blue
                    # back left
                    cmd_actuators[17] = 0xF0  # Green, red
                    cmd_actuators[18] = 0x00  # Blue
                    # back right
                    cmd_actuators[19] = 0xF0  # Green, red
                    cmd_actuators[20] = 0x00  # Blue

                    print(f"Green full brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    # blue full brightness
                    # front left
                    cmd_actuators[13] = 0x00  # Green, red
                    cmd_actuators[14] = 0x0F  # Blue
                    # front right
                    cmd_actuators[15] = 0x00  # Green, red
                    cmd_actuators[16] = 0x0F  # Blue
                    # back left
                    cmd_actuators[17] = 0x00  # Green, red
                    cmd_actuators[18] = 0x0F  # Blue
                    # back right
                    cmd_actuators[19] = 0x00  # Green, red
                    cmd_actuators[20] = 0x0F  # Blue

                    print(f"Blue full brightness")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    # All off
                    # front left
                    cmd_actuators[13] = 0x00  # Green, red
                    cmd_actuators[14] = 0x00  # Blue
                    # front right
                    cmd_actuators[15] = 0x00  # Green, red
                    cmd_actuators[16] = 0x00  # Blue
                    # back left
                    cmd_actuators[17] = 0x00  # Green, red
                    cmd_actuators[18] = 0x00  # Blue
                    # back right
                    cmd_actuators[19] = 0x00  # Green, red
                    cmd_actuators[20] = 0x00  # Blue

                    print(f"RGB off")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    test_actuators_state = 4

                elif test_actuators_state == 4: # motors
                    left_motor_speed = 200
                    right_motor_speed = -200
                    left_motor_bytes = struct.pack('<h', left_motor_speed)
                    right_motor_bytes = struct.pack('<h', right_motor_speed)
                    cmd_actuators[21:23] = left_motor_bytes
                    cmd_actuators[23:25] = right_motor_bytes
                
                    print(f"Motors 200, -200")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    left_motor_speed = -200
                    right_motor_speed = 200
                    left_motor_bytes = struct.pack('<h', left_motor_speed)
                    right_motor_bytes = struct.pack('<h', right_motor_speed)
                    cmd_actuators[21:23] = left_motor_bytes
                    cmd_actuators[23:25] = right_motor_bytes
                
                    print(f"Motors -200, 200")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    left_motor_speed = 0
                    right_motor_speed = 0
                    left_motor_bytes = struct.pack('<h', left_motor_speed)
                    right_motor_bytes = struct.pack('<h', right_motor_speed)
                    cmd_actuators[21:23] = left_motor_bytes
                    cmd_actuators[23:25] = right_motor_bytes
                
                    print(f"Motors off")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(0.5)

                    test_actuators_state = 5          
                
                elif test_actuators_state == 5: # sound

                    cmd_actuators[25] = 1
                    print(f"Play a3")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                    await asyncio.sleep(1.5)
                    cmd_actuators[25] = 0
                    print(f"Stop play")
                    await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)                    

                    test_actuators_state = 0


            cmd_notif[0] = 0x01 # setup notification command
            cmd_notif[1] = 0x00 # disable most sensors stream
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
