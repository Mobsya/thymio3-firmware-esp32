import asyncio
from bleak import BleakScanner, BleakClient
import struct
import time

# UUIDs from the C code
# Service UUID: BLE_SVC_SPP_UUID16 -> 0xABF0
SERVICE_UUID = "0000abf0-0000-1000-8000-00805f9b34fb"
# Commands Characteristic UUID -> 0xABF1
CMD_CHARACTERISTIC_UUID = "0000abf1-0000-1000-8000-00805f9b34fb"
# Sensors stream Characteristic UUID -> 0xABF2
SENSORS_STREAM_CHARACTERISTIC_UUID = "0000abf2-0000-1000-8000-00805f9b34fb"
# Python Characteristic UUID -> 0xABF3
PYTHON_CHARACTERISTIC_UUID = "0000abf3-0000-1000-8000-00805f9b34fb"


# Callback for receiving notifications
def notification_handler(sender, data):
    """
    Handles incoming data notifications from the BLE device.
    It unpacks the data to extract the proximity sensor values and calculates the refresh rate.
    """
    print("notification = " + str(data))


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

async def main():

    packet_size = 251
    cmd_packet_exec = bytearray([0] * 1)
    cmd_packet_exec[0] = 0x05
    cmd_packet_stop = bytearray([0] * 1)
    cmd_packet_stop[0] = 0x06

    script_thymio = """
import thymio
import time
mot = thymio.MOTORS()
mot.set_speed(200, -200)
rgb_fl = thymio.LEDS_RGB(0)
while 1:
\trgb_fl.set_intensity(1, 0, 0)
\ttime.sleep(0.2)
\trgb_fl.set_intensity(0, 1, 0)
\ttime.sleep(0.2)
\trgb_fl.set_intensity(0, 0, 1)
\ttime.sleep(0.2)
"""

    script_hello = "import time\n\nwhile 1:\n\tprint(\"Hello world\")\n\ttime.sleep(1)"
    script_excpetion = "import time\n\ntime.sleep(2)\nraise ValueError('Blabla.')"
    script_loop = "import time\n\na = 0\nwhile 1:\n\ta = a + 1\nprint(\"a=\" + str(a))"
    script_malformed = "import time\n\n   a= \n3"

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
    try:
        async with BleakClient(ble_address) as client:
            print(f"Connected: {client.is_connected}")
            
            # Subscribe to the Python characteristic for notifications
            await client.start_notify(PYTHON_CHARACTERISTIC_UUID, notification_handler)
            print("Subscribed to notifications from Python characteristic.")


            # # script_hello test
            # print("script_hello test")
            # byte_script = script_hello.encode('utf-8')
            # size_msb = (len(byte_script) >> 8) & 0xFF
            # size_lsb = len(byte_script) & 0xFF
            # print("script size = " + str(len(byte_script)))
            # header = bytes([0x04, size_msb, size_lsb])
            # cmd_packet = header + byte_script

            # print(f"Loading script...")
            # for i in range(0, len(cmd_packet), packet_size):
            #     chunk = cmd_packet[i:i + packet_size]              
            #     await client.write_gatt_char(PYTHON_CHARACTERISTIC_UUID, chunk, response=True)

            # await asyncio.sleep(1)
            # print(f"Executing script...")
            # await client.write_gatt_char(PYTHON_CHARACTERISTIC_UUID, cmd_packet_exec, response=True)

            # await asyncio.sleep(10)


            # # script_hello test, send it and run a second time to test running a second script while another is already running...
            # print("script_hello test")
            # byte_script = script_hello.encode('utf-8')
            # size_msb = (len(byte_script) >> 8) & 0xFF
            # size_lsb = len(byte_script) & 0xFF
            # print("script size = " + str(len(byte_script)))
            # header = bytes([0x04, size_msb, size_lsb])
            # cmd_packet = header + byte_script

            # print(f"Loading script...")
            # for i in range(0, len(cmd_packet), packet_size):
            #     chunk = cmd_packet[i:i + packet_size]              
            #     await client.write_gatt_char(PYTHON_CHARACTERISTIC_UUID, chunk, response=True)

            # await asyncio.sleep(1)
            # print(f"Executing script...")
            # await client.write_gatt_char(PYTHON_CHARACTERISTIC_UUID, cmd_packet_exec, response=True)

            # await asyncio.sleep(10)
            # print(f"Stop script...") # It stop the first instance of "script_hello"
            # await client.write_gatt_char(PYTHON_CHARACTERISTIC_UUID, cmd_packet_stop, response=True)

            # await asyncio.sleep(10)


            # # script_excpetion test
            # print("script_excpetion test")
            # byte_script = script_excpetion.encode('utf-8')
            # size_msb = (len(byte_script) >> 8) & 0xFF
            # size_lsb = len(byte_script) & 0xFF
            # print("script size = " + str(len(byte_script)))
            # header = bytes([0x04, size_msb, size_lsb])
            # cmd_packet = header + byte_script

            # print(f"Loading script...")
            # for i in range(0, len(cmd_packet), packet_size):
            #     chunk = cmd_packet[i:i + packet_size]              
            #     await client.write_gatt_char(PYTHON_CHARACTERISTIC_UUID, chunk, response=True)

            # await asyncio.sleep(1)
            # print(f"Executing script...")
            # await client.write_gatt_char(PYTHON_CHARACTERISTIC_UUID, cmd_packet_exec, response=True)

            # await asyncio.sleep(10)


            # # script_loop test
            # print("script_loop test")
            # byte_script = script_loop.encode('utf-8')
            # size_msb = (len(byte_script) >> 8) & 0xFF
            # size_lsb = len(byte_script) & 0xFF
            # print("script size = " + str(len(byte_script)))
            # header = bytes([0x04, size_msb, size_lsb])
            # cmd_packet = header + byte_script

            # print(f"Loading script...")
            # #if(len(cmd_packet) > 256):
            # for i in range(0, len(cmd_packet), packet_size):
            #     chunk = cmd_packet[i:i + packet_size]              
            #     await client.write_gatt_char(PYTHON_CHARACTERISTIC_UUID, chunk, response=True)

            # await asyncio.sleep(1)
            # print(f"Executing script...")
            # await client.write_gatt_char(PYTHON_CHARACTERISTIC_UUID, cmd_packet_exec, response=True)

            # await asyncio.sleep(10)
            # print(f"Stop script...")
            # await client.write_gatt_char(PYTHON_CHARACTERISTIC_UUID, cmd_packet_stop, response=True)


            # # script_malformed test
            # print("script_malformed test")
            # byte_script = script_malformed.encode('utf-8')
            # size_msb = (len(byte_script) >> 8) & 0xFF
            # size_lsb = len(byte_script) & 0xFF
            # print("script size = " + str(len(byte_script)))
            # header = bytes([0x04, size_msb, size_lsb])
            # cmd_packet = header + byte_script

            # print(f"Loading script...")
            # for i in range(0, len(cmd_packet), packet_size):
            #     chunk = cmd_packet[i:i + packet_size]              
            #     await client.write_gatt_char(PYTHON_CHARACTERISTIC_UUID, chunk, response=True)

            # await asyncio.sleep(1)
            # print(f"Executing script...")
            # await client.write_gatt_char(PYTHON_CHARACTERISTIC_UUID, cmd_packet_exec, response=True)

            # await asyncio.sleep(10)


            # script_thymio test
            print("script_thymio test")
            byte_script = script_thymio.encode('utf-8')
            size_msb = (len(byte_script) >> 8) & 0xFF
            size_lsb = len(byte_script) & 0xFF
            print("script size = " + str(len(byte_script)))
            header = bytes([0x04, size_msb, size_lsb])
            cmd_packet = header + byte_script

            print(f"Loading script...")
            #if(len(cmd_packet) > 256):
            for i in range(0, len(cmd_packet), packet_size):
                chunk = cmd_packet[i:i + packet_size]              
                await client.write_gatt_char(PYTHON_CHARACTERISTIC_UUID, chunk, response=True)

            await asyncio.sleep(5)
            print(f"Executing script...")
            await client.write_gatt_char(PYTHON_CHARACTERISTIC_UUID, cmd_packet_exec, response=True)

            await asyncio.sleep(10)
            print(f"Stop script...")
            await client.write_gatt_char(PYTHON_CHARACTERISTIC_UUID, cmd_packet_stop, response=True)


    except Exception as e:
        print(f"An error occurred: {e}")
    finally:
        # Ensure notifications are stopped and client is disconnected if an error occurs or the script exits
        if 'client' in locals() and client.is_connected:
            await client.stop_notify(PYTHON_CHARACTERISTIC_UUID)
            print("Stopped receiving notifications.")
            # BleakClient's async with statement handles disconnection automatically upon exiting the block
            # No explicit client.disconnect() is needed here if using 'async with'
        print("Script finished.")

if __name__ == "__main__":
    asyncio.run(main())
