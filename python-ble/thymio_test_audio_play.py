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
AUDIO_EXEC_OK = 0
AUDIO_EXEC_ERROR = 1
AUDIO_EXEC_NOT_FOUND = 2
AUDIO_EXEC_NOT_SUPPORTED = 3

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

    if(audio_test_state == 1): # expect exec indication error
        if(data[0] == AUDIO_IND_EXEC_RES) and (data[1] == AUDIO_EXEC_NOT_FOUND):
            print("received exec not found indication")
            audio_test_state = 2
    elif(audio_test_state == 3): # expect load indication ok
        if(data[0] == AUDIO_IND_LOAD_RES) and (data[1] == FILE_LOAD_OK):
            print("received load ok indication")
            audio_test_state = 4
    elif(audio_test_state == 5): # expect exec indication ok
        if(data[0] == AUDIO_IND_EXEC_RES) and (data[1] == AUDIO_EXEC_OK):
            print("received exec ok indication")
            audio_test_state = 6
    elif(audio_test_state == 7): # expect load indication ok
        if(data[0] == AUDIO_IND_LOAD_RES) and (data[1] == FILE_LOAD_OK):
            print("received load ok indication")
            audio_test_state = 8
    elif(audio_test_state == 9): # expect exec indication error
        if(data[0] == AUDIO_IND_EXEC_RES) and (data[1] == AUDIO_EXEC_NOT_SUPPORTED):
            print("received exec error format not supported indication")
            audio_test_state = 10
    elif(audio_test_state == 11): # expect load indication ok
        if(data[0] == AUDIO_IND_LOAD_RES) and (data[1] == FILE_LOAD_OK):
            print("received load ok indication")
            audio_test_state = 12
    elif(audio_test_state == 13): # expect exec indication ok
        if(data[0] == AUDIO_IND_EXEC_RES) and (data[1] == AUDIO_EXEC_OK):
            print("received exec ok indication")
            audio_test_state = 14
    elif(audio_test_state == 15): # expect load indication ok
        if(data[0] == AUDIO_IND_LOAD_RES) and (data[1] == FILE_LOAD_OK):
            print("received load ok indication")
            audio_test_state = 16
    elif(audio_test_state == 18): # expect exec indication ok
        if(data[0] == AUDIO_IND_EXEC_RES) and (data[1] == AUDIO_EXEC_OK):
            print("received exec ok indication")
            audio_test_state = 19
    elif(audio_test_state == 20): # expect load indication ok
        if(data[0] == AUDIO_IND_LOAD_RES) and (data[1] == FILE_LOAD_TOO_BIG):
            print("received load error too big indication")
            audio_test_state = 21
    elif(audio_test_state == 23): # expect exec indication error
        if(data[0] == AUDIO_IND_EXEC_RES) and (data[1] == AUDIO_EXEC_ERROR):
            print("received exec error already playing indication")
            audio_test_state = 24
    elif(audio_test_state == 24): # expect exec indication ok
        if(data[0] == AUDIO_IND_EXEC_RES) and (data[1] == AUDIO_EXEC_OK):
            print("received exec ok indication")
            audio_test_state = 25            

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

async def load_file(file_path, client, chunk_size):
    # Header packet format:
    # 1 byte: Command (e.g., 0x01 for load audio)
    # 4 bytes: Total file length (high byte, low byte)
    # 4 bytes: CRC32
    global audio_char

    # Read the audio file
    print(f"Reading file: {file_path}")
    with open(file_path, "rb") as f:
        file_data = f.read()
    
    file_size = len(file_data)
    file_crc = crc32mpeg2(file_data)
    file_name = os.path.basename(file_path)

    if len(file_name.encode('utf-8')) > 20:
        print("Error: Filename is too long (max 20 bytes)")
        return
    
    print(f"File Size: {file_size} bytes")
    print(f"File CRC32: 0x{file_crc:08X}")

    # In the C code, the command is 'AUDIO_WRITE_LOAD' which is not explicitly defined.
    # We'll use a placeholder command byte 0x01.
    header_command = 0x01 
    header = bytearray([header_command])
    header.extend(file_size.to_bytes(4, byteorder='big')) # 4 bytes total len
    header.extend(file_crc.to_bytes(4, byteorder='big')) # 4 bytes crc

    sequence_id = 0
    sequence_id_bytes = bytearray(2)
    sequence_id_bytes[0] = (sequence_id >> 8) & 0xFF            
    sequence_id_bytes[1] = sequence_id & 0xFF

    cmd_packet = header + sequence_id_bytes + file_data
    print("Total packet size = " + str(len(cmd_packet)))

    start_time = time.perf_counter()

    total_bytes_sent = 0

    first_chunk = cmd_packet[:chunk_size]
    await client.write_gatt_char(AUDIO_CHAR_UUID, first_chunk, response=True)
    print(f"Sent first chunk of size: {len(first_chunk)}")
    total_bytes_sent += len(first_chunk)

    print("Sending file data in chunks...")
    for i in range(chunk_size, len(cmd_packet), (chunk_size-2)):
        sequence_id += 1
        chunk = cmd_packet[i:i + (chunk_size-2)]
        
        # Each packet starts with a 2-byte sequence ID
        sequence_id_bytes[0] = (sequence_id >> 8) & 0xFF            
        sequence_id_bytes[1] = sequence_id & 0xFF
        chunk_with_seq_id = sequence_id_bytes + chunk
        total_bytes_sent += len(chunk_with_seq_id)
        await client.write_gatt_char(audio_char, chunk_with_seq_id, response=True)
        print(f"Sent subsequent chunk with seq_id {sequence_id}, size: {len(chunk_with_seq_id)}")
        
    end_time = time.perf_counter()
    elapsed_time = end_time - start_time
    print("File transfer complete.")
    print("total bytes sent = " + str(total_bytes_sent) + " in " + str(elapsed_time) + " seconds")
    

async def send_audio_file():
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

    cmd_packet_stop = bytearray([0] * 1)
    cmd_packet_stop[0] = 0x03

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
                if(audio_test_state == 0): # Send exec command
                    print("[" + str(audio_test_state) + "] Send exec command")
                    audio_test_state = 1
                    await client.write_gatt_char(audio_char, cmd_packet_exec, response=True)
                elif(audio_test_state == 1): # Expect an indication error since no audio was loaded
                    print("[" + str(audio_test_state) + "] Expect indication exec error")
                elif(audio_test_state == 2): # Load correct wav file
                    print("[" + str(audio_test_state) + "] Loading correct wav audio")
                    audio_test_state = 3
                    await load_file("./audio/ok.wav", client, chunk_size)                    
                elif(audio_test_state == 3): # Expect indication load ok
                    print("[" + str(audio_test_state) + "] Expect indication load ok")
                elif(audio_test_state == 4): # Play wav audio
                    print("[" + str(audio_test_state) + "] Send exec command")
                    audio_test_state = 5
                    await client.write_gatt_char(audio_char, cmd_packet_exec, response=True)                    
                elif(audio_test_state == 5): # Expect an indication exec ok
                    print("[" + str(audio_test_state) + "] Expect indication exec ok")
                elif(audio_test_state == 6): # Load wrong format wav file
                    print("[" + str(audio_test_state) + "] Loading wrong wav audio")
                    audio_test_state = 7
                    await load_file("./audio/wrong.wav", client, chunk_size)
                elif(audio_test_state == 7): # Expect indication load ok
                    print("[" + str(audio_test_state) + "] Expect indication load ok")
                elif(audio_test_state == 8): # Play wav audio
                    print("[" + str(audio_test_state) + "] Send exec command")
                    audio_test_state = 9
                    await client.write_gatt_char(audio_char, cmd_packet_exec, response=True)  
                elif(audio_test_state == 9): # Expect an indication exec error (format not supported)
                    print("[" + str(audio_test_state) + "] Expect indication exec error")
                elif(audio_test_state == 10): # Load correct mp3 file
                    print("[" + str(audio_test_state) + "] Loading correct mp3 audio")
                    audio_test_state = 11
                    await load_file("./audio/small.mp3", client, chunk_size)                    
                elif(audio_test_state == 11): # Expect indication load ok
                    print("[" + str(audio_test_state) + "] Expect indication load ok")
                elif(audio_test_state == 12): # Play mp3 audio
                    print("[" + str(audio_test_state) + "] Send exec command")
                    audio_test_state = 13
                    await client.write_gatt_char(audio_char, cmd_packet_exec, response=True)                    
                elif(audio_test_state == 13): # Expect an indication exec ok
                    print("[" + str(audio_test_state) + "] Expect indication exec ok")
                elif(audio_test_state == 14): # Load big mp3 file
                    print("[" + str(audio_test_state) + "] Loading big mp3 audio")
                    audio_test_state = 15
                    await load_file("./audio/big.mp3", client, chunk_size)                    
                elif(audio_test_state == 15): # Expect indication load ok
                    print("[" + str(audio_test_state) + "] Expect indication load ok")
                elif(audio_test_state == 16): # Play mp3 audio
                    print("[" + str(audio_test_state) + "] Send exec command")
                    audio_test_state = 17
                    await client.write_gatt_char(audio_char, cmd_packet_exec, response=True) 
                elif(audio_test_state == 17): # Stop after some seconds
                    await asyncio.sleep(5)
                    audio_test_state = 18
                    print("[" + str(audio_test_state) + "] Send stop command")
                    await client.write_gatt_char(audio_char, cmd_packet_stop, response=True) 
                elif(audio_test_state == 18): # Expect an indication exec ok
                    print("[" + str(audio_test_state) + "] Expect indication exec ok")
                elif(audio_test_state == 19): # Load too big mp3 file
                    print("[" + str(audio_test_state) + "] Loading too big audio (fake)")                    
                    file_size = 240001
                    file_crc = 0
                    header_command = 0x01 
                    header = bytearray([header_command])
                    header.extend(file_size.to_bytes(4, byteorder='big')) # 4 bytes total len
                    header.extend(file_crc.to_bytes(4, byteorder='big')) # 4 bytes crc
                    sequence_id = 0
                    sequence_id_bytes = bytearray(2)
                    sequence_id_bytes[0] = (sequence_id >> 8) & 0xFF            
                    sequence_id_bytes[1] = sequence_id & 0xFF
                    cmd_packet = header + sequence_id_bytes
                    audio_test_state = 20
                    await client.write_gatt_char(AUDIO_CHAR_UUID, cmd_packet, response=True)
                elif(audio_test_state == 20): # Expect indication load error
                    print("[" + str(audio_test_state) + "] Expect indication load error too big")
                elif(audio_test_state == 21): # Execute again what was previously loaded in memory
                    print("[" + str(audio_test_state) + "] Send exec command (again)")
                    audio_test_state = 22
                    await client.write_gatt_char(audio_char, cmd_packet_exec, response=True)
                elif(audio_test_state == 22): # Execute once more before previous playing is terminated
                    await asyncio.sleep(5)
                    print("[" + str(audio_test_state) + "] Send exec command (again before termination)")
                    audio_test_state = 23
                    await client.write_gatt_char(audio_char, cmd_packet_exec, response=True)                     
                elif(audio_test_state == 23): # Expect an indication exec error
                    print("[" + str(audio_test_state) + "] Expect indication exec error already playing")
                elif(audio_test_state == 24): # Expect an indication exec ok
                    print("[" + str(audio_test_state) + "] Expect indication exec ok")                
                elif(audio_test_state == 25): # Test finished
                    print("TEST SUCCESSFULL!!!")
                    break
                await asyncio.sleep(1)
    
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    
    asyncio.run(send_audio_file())

