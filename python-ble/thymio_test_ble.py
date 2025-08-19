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

    if len(data) == STREAM_NOTIFY_MOST_SENSORS_LEN:
        # The data format is defined in the bt_tx_data array in the C code
        # Proximity sensor values start at index 24 and are 7x uint16_t
        # bt_tx_data[24-25]: prox_temp = GetProximityValue(0)
        # bt_tx_data[26-27]: prox_temp = GetProximityValue(1)
        # ... and so on up to GetProximityValue(6) at bt_tx_data[36-37]
        # The '<' means little-endian, 'H' means unsigned short (2 bytes)
        proximity_values = struct.unpack('<HHHHHHH', data[24:38])
        print(f"[{current_time:.2f}] Proximity Values: {proximity_values} | Refresh Rate: {refresh_rate_hz:.2f} Hz")
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

            # Define colors to cycle through (Red, Green, Blue)
            colors = [(15, 0, 0), (0, 15, 0), (0, 0, 15)]
            color_index = 0

            # Define motor speeds for pivoting
            left_motor_speed = -200
            right_motor_speed = 200

            print(f"Setting motors to pivot (Left: {left_motor_speed}, Right: {right_motor_speed})")
            print("Continuously changing front-left LED color every second...")

            # Start time for the 15-second duration
            start_time = time.time()
            duration = 15 # seconds

            while True:
                # Check if 15 seconds have passed
                if time.time() - start_time > duration:
                    print(f"Time limit of {duration} seconds reached. Disconnecting...")
                    break # Exit the loop

                # Prepare the command packet
                cmd_actuators[0] = 0x01

                # Set RGB values for the front-left LED
                current_color = colors[color_index]
                cmd_actuators[13] = (current_color[0]&0xFF) | ((current_color[1]&0xFF)<<4)  # Red, green
                cmd_actuators[14] = current_color[2]&0xFF  # Blue
                
                # Set motor speeds
                # Use struct.pack('<h', value) to convert signed 16-bit integer to 2 bytes (little-endian)
                left_motor_bytes = struct.pack('<h', left_motor_speed)
                right_motor_bytes = struct.pack('<h', right_motor_speed)
                
                cmd_actuators[21:23] = left_motor_bytes
                cmd_actuators[23:25] = right_motor_bytes

                # Write the command packet to the CMD characteristic
                print(f"Writing color {current_color} and motor speeds to CMD characteristic...")
                await client.write_gatt_char(CMD_CHARACTERISTIC_UUID, cmd_actuators, response=True)
                
                # Move to the next color in the cycle
                color_index = (color_index + 1) % len(colors)
                
                # Wait for 1 second before the next update
                await asyncio.sleep(1)

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
