# send_audio.py
import asyncio
import os
import sys
import argparse
from bleak import BleakClient, BleakScanner
import time

# UUIDs from the C code
# Service UUID: BLE_SVC_THYMIO_UUID16 -> 0xABF0
SERVICE_UUID = "0000abf0-0000-1000-8000-00805f9b34fb"
AUDIO_CHAR_UUID = "0000abf4-0000-1000-8000-00805f9b34fb"

FILE_LOAD_OK = 0
FILE_LOAD_CRC_ERR = 1
FILE_LOAD_NOT_COMPLETE = 2
FILE_LOAD_WRONG_SEQ = 3
FILE_LOAD_TOO_BIG = 4

AUDIO_IND_LOAD_RES = 0x01
AUDIO_IND_EXEC_RES = 0x02
AUDIO_IND_REC_RES = 0x03

AUDIO_EXEC_OK = 0
AUDIO_EXEC_ERROR = 1
AUDIO_EXEC_NOT_FOUND = 2
AUDIO_EXEC_NOT_SUPPORTED = 3

AUDIO_REC_OK = 0
AUDIO_REC_ERROR = 1
AUDIO_REC_TOO_LONG = 2

audio_test_state = 0 
audio_char = None

# Callback for receiving python notifications
def audio_notification_handler(sender, data):
    """
    Handles incoming data notifications from the BLE device.
    It unpacks the data to extract the proximity sensor values and calculates the refresh rate.
    """
    global audio_test_state

    print("audio notification = " + str(data))

    if(audio_test_state == 1): # expect exec indication ok
        if(data[0] == AUDIO_IND_REC_RES) and (data[1] == AUDIO_REC_OK):
            print("received rec ok indication")
            audio_test_state = 2
    elif(audio_test_state == 3): # expect load indication ok
        if(data[0] == AUDIO_IND_REC_RES) and (data[1] == AUDIO_REC_TOO_LONG):
            print("received rec error too long indication")
            audio_test_state = 4
    elif(audio_test_state == 5): # expect load indication ok
        if(data[0] == AUDIO_IND_REC_RES) and (data[1] == AUDIO_REC_OK):
            print("received rec ok indication")
            audio_test_state = 6            
    elif(audio_test_state == 7): # expect exec indication ok
        if(data[0] == AUDIO_IND_EXEC_RES) and (data[1] == AUDIO_EXEC_OK):
            print("received exec ok indication")
            audio_test_state = 8

def crc32mpeg2(buf, crc=0xFFFFFFFF):
    for val in buf:
        crc ^= val << 24
        for _ in range(8):
            if (crc & 0x80000000) == 0:
                crc = crc << 1
            else:
                crc = (crc << 1) ^ 0x04C11DB7
    return crc & 0xFFFFFFFF

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
  

async def test_rec():
    """
    Connects to the Thymio BLE device and sends the audio file.
    
    :param file_path: Path to the WAV or MP3 file to send.
    """

    global audio_test_state, audio_char

    # Chunk and send the file data
    chunk_size = 500  # The C code's rx_buff_temp is 500

    # packet for "execute script" command
    cmd_packet_exec = bytearray([0] * 21)
    cmd_packet_exec[0] = 0x02

    cmd_packet_start_rec = bytearray([0] * 2)
    cmd_packet_start_rec[0] = 0x05    

    ble_address = await find_thymio_device()
    if not ble_address:
        print("Could not find a Thymio device. Exiting.")
        return

    print(f"Connecting to {ble_address}...")
    try:
        async with BleakClient(ble_address) as client:
            print("Connected.")
            
            # Subscribe to the audio characteristics for indications
            await client.start_notify(AUDIO_CHAR_UUID, audio_notification_handler)
            print("Subscribed to notifications from audio characteristic.")

            # Wait for the characteristic handle to be ready before writing.
            audio_char = client.services.get_characteristic(AUDIO_CHAR_UUID)
            if not audio_char:
                print(f"Error: Characteristic with UUID {AUDIO_CHAR_UUID} not found.")
                return

            audio_test_state = 0            
            while 1:
                if(audio_test_state == 0): # Send start rec command
                    cmd_packet_start_rec[1] = 5 # 5 seconds
                    print("[" + str(audio_test_state) + "] Send start rec command")
                    audio_test_state = 1
                    await client.write_gatt_char(audio_char, cmd_packet_start_rec, response=True)
                elif(audio_test_state == 1): # Expect an indication ok
                    print("[" + str(audio_test_state) + "] Expect indication ok")
                elif(audio_test_state == 2): # Send start rec too long
                    cmd_packet_start_rec[1] = 11 # 11 seconds
                    print("[" + str(audio_test_state) + "] Send start rec too long")
                    audio_test_state = 3
                    await client.write_gatt_char(audio_char, cmd_packet_start_rec, response=True)
                elif(audio_test_state == 3): # Expect an indication error
                    print("[" + str(audio_test_state) + "] Expect indication error")
                elif(audio_test_state == 4): # Send start rec max
                    cmd_packet_start_rec[1] = 10 # 10 seconds
                    print("[" + str(audio_test_state) + "] Send start rec max")
                    audio_test_state = 5
                    await client.write_gatt_char(audio_char, cmd_packet_start_rec, response=True)
                elif(audio_test_state == 5): # Expect an indication ok
                    print("[" + str(audio_test_state) + "] Expect indication ok")
                elif(audio_test_state == 6): # Send exec command
                    print("[" + str(audio_test_state) + "] Send exec command")
                    audio_test_state = 7
                    await client.write_gatt_char(audio_char, cmd_packet_exec, response=True)      
                elif(audio_test_state == 7): # Expect an indication exec ok
                    print("[" + str(audio_test_state) + "] Expect indication exec ok")                                                               
                elif(audio_test_state == 8): # Test finished
                    print("TEST SUCCESSFULL!!!")
                    break
                await asyncio.sleep(1)
    
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    
    asyncio.run(test_rec())

