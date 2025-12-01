import asyncio
import time
import struct
import random
import os
import sys
from typing import Dict, Any, List
from bleak import BleakScanner, uuids, BleakClient

# -----------------------------------------------------------------------------
# CONFIGURATION
# -----------------------------------------------------------------------------

# IMPORTANT: REPLACE THIS PLACEHOLDER WITH YOUR ROBOT'S ACTUAL BLE ADDRESS
ROBOT_BLE_ADDRESS = "00:00:00:00:00:00"

# BLE UUIDs and Protocol Constants (Inferred from ble_spp_server.h and ble_spp.h)
FS_CHAR_UUID = "ABF6" # File System Characteristic UUID

# Commands (Write to FS_CHAR_UUID)
CMD_LOAD_DATA = 0x01
CMD_SAVE_DATA = 0x02
CMD_DELETE_DATA = 0x03
CMD_DOWNLOAD = 0x06
CMD_DOWNLOAD_ACK = 0x07

# Indications (Received via Notifications/Indications on FS_CHAR_UUID)
IND_LOAD_RES = 0x01
IND_SAVE_RES = 0x02
IND_DELETE_RES = 0x03
IND_DOWNLOAD_DATA = 0x07
IND_DOWNLOAD_RES = 0x08

# Response Code
RESP_OK = 0x00

# Test Parameters
MAX_CHUNK_SIZE = 500 # Safe chunk size for transfers (assuming MTU >= 247)
FILE_NAME = "testfile.bin" + '\x00'
# Sizes: 25, 50, 75, ..., 250 KB (10 sizes)
FILE_SIZES_KB = list(range(300, 301, 1)) #list(range(200, 251, 25)) #list(range(25, 251, 25))
# Total 20 cycles: repeating the 10 sizes twice
TEST_CYCLES = sorted(list(set(FILE_SIZES_KB))) * 2

# -----------------------------------------------------------------------------
# GLOBAL STATE AND SYNCHRONIZATION
# -----------------------------------------------------------------------------

# Synchronization tools for the asynchronous operations
indication_event = asyncio.Event()
indication_data: Dict[str, Any] = {}

# Download data buffer
downloaded_data = bytearray()
expected_download_len = 0
current_sequence_num = 0
download_in_progress = False
download_crc = 0
download_seq_num = 0

# Statistics list
stats: List[Dict[str, Any]] = []

# -----------------------------------------------------------------------------
# HELPER FUNCTIONS
# -----------------------------------------------------------------------------
def crc32mpeg2(buf, crc=0xFFFFFFFF):
    for val in buf:
        crc ^= val << 24
        for _ in range(8):
            if (crc & 0x80000000) == 0:
                crc = crc << 1
            else:
                crc = (crc << 1) ^ 0x04C11DB7
    return crc & 0xFFFFFFFF

def create_dummy_file(size_bytes: int) -> bytes:
    """Creates a dummy file content of the specified size with random bytes."""
    # Use os.urandom for cryptographically strong randomness (good for testing data integrity)
    print(f"-> Generating dummy file of {size_bytes / 1024:.2f} KB...")
    return os.urandom(size_bytes)

def notification_handler(sender: int, data: bytearray):
    """Handles incoming BLE indications/notifications on the FS characteristic."""
    global indication_data, downloaded_data, current_sequence_num, expected_download_len, download_in_progress, download_crc, download_seq_num
    
    if not data:
        print("<- WARNING: Received empty indication.")
        return

    ind_type = data[0]
    ind_payload = data[1:]

    # Store indication details
    indication_data['type'] = ind_type
    indication_data['time'] = time.time()

    if download_in_progress:
        download_seq_num = struct.unpack('<H', data[:2])[0]
        data_chunk = data[2:]
        #print(f"<- seq {download_seq_num}=={current_sequence_num}.")

        # Simple sequence check
        if download_seq_num == current_sequence_num:
            downloaded_data.extend(data_chunk)
            current_sequence_num += 1
            
            # Signal that an ACK write is needed for flow control
            indication_data['ack_needed'] = True
            indication_event.set() # Wake up the main thread to send ACK
        else:
            print(f"<- ERROR: Download sequence mismatch. Expected {current_sequence_num}, got {download_seq_num}. Data skipped.")
            download_in_progress = False        
    else:
        if ind_type in [IND_LOAD_RES, IND_SAVE_RES, IND_DELETE_RES, IND_DOWNLOAD_RES]:
            print(f"<- Indication received: Type 0x{ind_type:02X}. Payload size: {len(ind_payload)}. Payload: {ind_payload}")

            # Handle simple result indications
            indication_data['result'] = ind_payload[0]
            indication_event.set()
            
        elif ind_type == IND_DOWNLOAD_DATA:
            print(f"<- Indication received: Type 0x{ind_type:02X}. Payload size: {len(ind_payload)}")
            download_in_progress = True
            expected_download_len = struct.unpack('<I', ind_payload[:4])[0]
            download_crc = struct.unpack('<I', ind_payload[4:8])[0]
            download_seq_num = struct.unpack('<H', ind_payload[8:10])[0]
            data_chunk = ind_payload[10:]
            #print(f"<- Exp size {expected_download_len}, crc {download_crc}, seq {download_seq_num}.")
            
            # Simple sequence check
            if download_seq_num == current_sequence_num:
                downloaded_data.extend(data_chunk)
                current_sequence_num += 1
                
                # Signal that an ACK write is needed for flow control
                indication_data['ack_needed'] = True
                indication_event.set() # Wake up the main thread to send ACK
            else:
                print(f"<- ERROR: Download sequence mismatch. Expected {current_sequence_num}, got {download_seq_num}. Data skipped.")
                download_in_progress = False

async def wait_for_indication(expected_type: int, timeout: float = 10.0) -> Dict[str, Any]:
    """Waits for a specific indication type, handling download chunks in between."""
    global indication_data
    
    start_time = time.time()
    
    while True:
        # Reset event data before waiting
        #indication_data = {'type': None, 'time': None}
        #indication_event.clear()
        
        try:
            # Wait for any indication to arrive
            await asyncio.wait_for(indication_event.wait(), timeout=timeout - (time.time() - start_time))
            indication_event.clear()
        except asyncio.TimeoutError:
            raise TimeoutError(f"Timeout waiting for indication type 0x{expected_type:02X}")

        # The event was set, check the received type
        received_type = indication_data.get('type')

        if received_type == expected_type:
            # Found the expected result, return it
            return indication_data
        elif received_type == IND_DOWNLOAD_DATA and received_type != expected_type:
            # If we are waiting for a final result (not IND_DOWNLOAD_DATA) but get data,
            # this is a protocol violation or a race condition. Treat as error/continue.
            print(f"<- WARNING: Received 0x{received_type:02X} while waiting for 0x{expected_type:02X}. Ignoring and waiting.")
            continue
        
        # If it's a transient message (like IND_DOWNLOAD_DATA when we expect it),
        # the caller will process it and clear the event to wait for the next one.
        # This function should only return when the *expected* type is found.

# -----------------------------------------------------------------------------
# FS OPERATIONS
# -----------------------------------------------------------------------------

async def upload_file(client, char_uuid, file_data: bytes, filename: str) -> float:
    """Implements FS_WRITE_LOAD."""
    file_size = len(file_data)
    crc32_checksum = crc32mpeg2(file_data)
    #print(f"Upload CRC32: {hex(crc32_checksum)}")
    crc32_bytes = bytearray(4)
    crc32_bytes[3] = crc32_checksum & 0xFF
    crc32_bytes[2] = (crc32_checksum >> 8) & 0xFF
    crc32_bytes[1] = (crc32_checksum >> 16) & 0xFF
    crc32_bytes[0] = (crc32_checksum >> 24) & 0xFF
    size_file = bytearray(4)
    size_file[3] = file_size & 0xFF
    size_file[2] = (file_size >> 8) & 0xFF
    size_file[1] = (file_size >> 16) & 0xFF
    size_file[0] = (file_size >> 24) & 0xFF
    sequence_id = 0
    sequence_id_bytes = bytearray(2)
    sequence_id_bytes[0] = (sequence_id >> 8) & 0xFF            
    sequence_id_bytes[1] = sequence_id & 0xFF

    #print("Upload file size = " + str(file_size))
    header = bytes([0x01]) + size_file + crc32_bytes + sequence_id_bytes
    #print("header = " + str(header))
    cmd_packet = header + file_data

    print(f"-> Upload: Sending {file_size} bytes in chunks (max {MAX_CHUNK_SIZE} bytes/chunk)...")

    start_time = time.time()

    # Handle the first chunk separately
    first_chunk = cmd_packet[:MAX_CHUNK_SIZE]
    await client.write_gatt_char(FS_CHAR_UUID, first_chunk, response=True)
    #print(f"Sent first chunk of size: {len(first_chunk)}")

    for i in range(MAX_CHUNK_SIZE, len(cmd_packet), (MAX_CHUNK_SIZE-2)):
        sequence_id += 1            
        chunk = cmd_packet[i:i + (MAX_CHUNK_SIZE-2)] # 2 bytes used for sequence id
        sequence_id_bytes[0] = (sequence_id >> 8) & 0xFF            
        sequence_id_bytes[1] = sequence_id & 0xFF
        chunk_with_seq_id = sequence_id_bytes + chunk    
        await client.write_gatt_char(FS_CHAR_UUID, chunk_with_seq_id, response=True)
        #print(f"Sent subsequent chunk with seq_id {sequence_id}, size: {len(chunk_with_seq_id)}")
    
    # 3. Wait for Load Result Indication
    res = await wait_for_indication(IND_LOAD_RES)
    end_time = time.time()
    
    if res.get('result') == RESP_OK:
        print(f"<- Upload: SUCCESS.")
        return end_time - start_time
    else:
        raise Exception(f"Upload failed with response code: 0x{res.get('result'):02X}")

async def save_file(client, char_uuid, filename: str) -> float:
    """Implements FS_WRITE_SAVE."""
    # Command: [CMD_ID: 1 byte, Filename_Len: 1 byte, Filename: N bytes]
    print(f"-> Save: Sending SAVE command (0x{CMD_SAVE_DATA:02X})...")
    filename_bytes = filename.encode('ascii')

    command_packet = bytes([CMD_SAVE_DATA]) + filename_bytes
    
    start_time = time.time()
    await client.write_gatt_char(char_uuid, command_packet, response=True)
    
    # Wait for Save Result Indication
    res = await wait_for_indication(IND_SAVE_RES)
    end_time = time.time()
    
    if res.get('result') == RESP_OK:
        print(f"<- Save: SUCCESS.")
        return end_time - start_time
    else:
        raise Exception(f"Save failed with response code: 0x{res.get('result'):02X}")

async def download_file(client, char_uuid, filename: str, expected_data: bytes) -> tuple[float, bool]:
    """Implements FS_WRITE_DOWNLOAD and data check."""
    global downloaded_data, expected_download_len, current_sequence_num, indication_data, download_in_progress, download_seq_num
    
    # Reset state for download
    downloaded_data = bytearray()
    current_sequence_num = 0
    indication_data.clear()
    
    # 1. Initial Command: [CMD_ID: 1 byte, Filename_Len: 1 byte, Filename: N bytes]
    print(f"-> Download: Sending DOWNLOAD command (0x{CMD_DOWNLOAD:02X})...")
    filename_bytes = filename.encode('ascii')
    command_packet = bytes([CMD_DOWNLOAD]) + filename_bytes

    start_time = time.time()
    await client.write_gatt_char(char_uuid, command_packet, response=True)
    
    # Wait for the next data chunk indication (IND_DOWNLOAD_DATA)
    await wait_for_indication(IND_DOWNLOAD_DATA) 

    # 2. Receive Data Chunks and ACK
    while len(downloaded_data) < expected_download_len:
        try:
            # The notification_handler sets ack_needed and ack_seq upon receiving a chunk
            if indication_data.get('ack_needed'):
                ack_packet = bytes([CMD_DOWNLOAD_ACK])
                
                # Send ACK (FS_WRITE_DOWNLOAD_ACK)
                await client.write_gatt_char(char_uuid, ack_packet, response=True)
                
                # Clear the ACK flags so the handler can set them again
                del indication_data['ack_needed']
                indication_event.clear() # Clear event to allow handler to set it again on next chunk
                
                #print(f"-> Download: ACK'd chunk {download_seq_num}. Total: {len(downloaded_data)}/{expected_download_len} bytes.")
            else:
                 # Should not happen if protocol is followed, but handles spurious wakes
                pass
    
            # Wait for the next data chunk indication (IND_DOWNLOAD_DATA)
            await asyncio.wait_for(indication_event.wait(), timeout=10.0)
            indication_event.clear()

        except TimeoutError:
            print("-> Download: Timeout waiting for next data chunk.")            
            break
            
    download_in_progress = False
    # 3. Wait for final Download Result Indication
    #try:
    #    res = await wait_for_indication(IND_DOWNLOAD_RES)
    #except TimeoutError:
    #    print("-> Download: Timeout waiting for final DOWNLOAD_RES. Assuming failure.")
    #    res = {'result': 0xFF, 'type': IND_DOWNLOAD_RES} # Force a non-OK check

    end_time = time.time()
    time_taken = end_time - start_time
    
    #if res.get('result') != RESP_OK:
    #     print(f"<- Download: FAILED with final response code 0x{res.get('result'):02X}.")
    #     return time_taken, False
    
    if len(downloaded_data) != expected_download_len:
         print(f"<- Download: FAILED.")
         return time_taken, False
         
    # 4. Data integrity check
    data_match = downloaded_data == expected_data
    
    if data_match:
        print(f"<- Download: SUCCESS. Data integrity CHECK OK. Time: {time_taken:.3f}s")
    else:
        print(f"<- Download: FAILED. Data integrity CHECK FAILED. Downloaded {len(downloaded_data)} bytes.")
    
    return time_taken, data_match

async def delete_file(client, char_uuid, filename: str) -> float:
    """Implements FS_WRITE_DELETE."""
    # Command: [CMD_ID: 1 byte, Filename_Len: 1 byte, Filename: N bytes]
    print(f"-> Delete: Sending DELETE command (0x{CMD_DELETE_DATA:02X})...")
    filename_bytes = filename.encode('ascii')
    command_packet = bytes([CMD_DELETE_DATA]) + filename_bytes
    
    start_time = time.time()
    await client.write_gatt_char(char_uuid, command_packet, response=True)
    
    # Wait for Delete Result Indication
    res = await wait_for_indication(IND_DELETE_RES)
    end_time = time.time()
    
    if res.get('result') == RESP_OK:
        print(f"<- Delete: SUCCESS.")
        return end_time - start_time
    else:
        raise Exception(f"Delete failed with response code: 0x{res.get('result'):02X}")

# -----------------------------------------------------------------------------
# TEST ORCHESTRATION AND REPORTING
# -----------------------------------------------------------------------------

async def run_test_cycle(client, char_uuid: str, file_size_kb: int, cycle_num: int):
    """Runs a single full test sequence (Upload, Save, Download, Delete)."""
    
    file_size_bytes = file_size_kb * 1024
    
    cycle_stats: Dict[str, Any] = {
        'cycle': cycle_num,
        'size_kb': file_size_kb,
        'size_bytes': file_size_bytes,
        'upload_time': None,
        'save_time': None,
        'download_time': None,
        'delete_time': None,
        'data_match': False,
        'success': False,
        'error': None
    }
    
    try:
        print("\n" + "="*50)
        print(f"STARTING CYCLE {cycle_num:02d}/{len(TEST_CYCLES)} - Size: {file_size_kb} KB")
        print("="*50)

        # 0) Create dummy file
        original_data = create_dummy_file(file_size_bytes)
        
        # 1) Upload the dummy file (FS_WRITE_LOAD)
        cycle_stats['upload_time'] = await upload_file(client, char_uuid, original_data, FILE_NAME)

        # 2) Save the dummy file (FS_WRITE_SAVE)
        cycle_stats['save_time'] = await save_file(client, char_uuid, FILE_NAME)

        # 3) Download and check (FS_WRITE_DOWNLOAD, FS_WRITE_DOWNLOAD_ACK)
        download_time, data_match = await download_file(client, char_uuid, FILE_NAME, original_data)
        cycle_stats['download_time'] = download_time
        cycle_stats['data_match'] = data_match
        
        if not data_match:
            raise Exception("Data integrity check failed during download.")

        # 4) Delete the file (FS_WRITE_DELETE)
        cycle_stats['delete_time'] = await delete_file(client, char_uuid, FILE_NAME)
        
        cycle_stats['success'] = True

    except Exception as e:
        print(f"\n!!! CYCLE {cycle_num} FAILED: {e} !!!")
        cycle_stats['error'] = str(e)
    
    finally:
        stats.append(cycle_stats)
        print("="*50)
        print(f"CYCLE {cycle_num} FINISHED. Success: {cycle_stats['success']}")
        print("="*50)


def print_statistics():
    """Prints the final summary statistics."""
    print("\n" + "#"*60)
    print("                 BLE FILE SYSTEM TEST REPORT")
    print("#"*60)
    
    total_cycles = len(stats)
    success_cycles = sum(1 for s in stats if s['success'])
    failure_cycles = total_cycles - success_cycles
    
    print(f"Total Cycles Run: {total_cycles}")
    print(f"Successful Cycles: {success_cycles}")
    print(f"Failed Cycles: {failure_cycles}")
    
    if not stats:
        return

    # --- Summary Timing Statistics ---
    times = {
        'upload': [s['upload_time'] for s in stats if s['upload_time'] is not None],
        'save': [s['save_time'] for s in stats if s['save_time'] is not None],
        'download': [s['download_time'] for s in stats if s['download_time'] is not None],
        'delete': [s['delete_time'] for s in stats if s['delete_time'] is not None],
    }
    
    #print("\n--- Summary Timing Statistics (Seconds) ---")
    #header = f"{'Operation':<10} | {'Avg Time':<10} | {'Min Time':<10} | {'Max Time':<10} | {'Count':<5}"
    #print("-" * len(header))
    #print(header)
    #print("-" * len(header))
    #
    #for op, t_list in times.items():
    #    if t_list:
    #        avg_t = sum(t_list) / len(t_list)
    #        min_t = min(t_list)
    #        max_t = max(t_list)
    #        print(f"{op.capitalize():<10} | {avg_t:<10.3f} | {min_t:<10.3f} | {max_t:<10.3f} | {len(t_list):<5}")
    #    else:
    #         print(f"{op.capitalize():<10} | {'N/A':<10} | {'N/A':<10} | {'N/A':<10} | {0:<5}")

    # --- Statistics by file size (Throughput estimation) ---
    print("\n--- Performance by File Size (Throughput and Success) ---")
    size_groups: Dict[int, Dict[str, List[float]]] = {}
    
    for s in stats:
        size = s['size_kb']
        if size not in size_groups:
            size_groups[size] = {'upload': [], 'download': [], 'success': []}
        
        if s['upload_time'] is not None: size_groups[size]['upload'].append(s['upload_time'])
        if s['download_time'] is not None: size_groups[size]['download'].append(s['download_time'])
        size_groups[size]['success'].append(s['success'])
        
    header_size = f"{'Size (KB)':<10} | {'Avg Upload (s)':<15} | {'Upload Rate (KB/s)':<18} | {'Avg Download (s)':<17} | {'Download Rate (KB/s)':<20} | {'Success Rate (%)':<18}"
    print("-" * len(header_size))
    print(header_size)
    print("-" * len(header_size))
    
    for size in sorted(size_groups.keys()):
        group = size_groups[size]
        
        # Calculate Averages and Rates
        upload_times = group['upload']
        avg_upload = sum(upload_times) / len(upload_times) if upload_times else 0.0
        upload_rate = (size / avg_upload) if avg_upload > 0 else 0.0
        
        download_times = group['download']
        avg_download = sum(download_times) / len(download_times) if download_times else 0.0
        download_rate = (size / avg_download) if avg_download > 0 else 0.0
        
        success_count = sum(1 for s in group['success'] if s)
        rate = (success_count / len(group['success'])) * 100 if group['success'] else 0.0

        print(f"{size:<10} | {avg_upload:<15.3f} | {upload_rate:<18.1f} | {avg_download:<17.3f} | {download_rate:<20.1f} | {rate:<18.1f}")
        
    print("\n--- Indications Received (Success/Failure) ---")
    print(f"Total cycles completed: {total_cycles}")
    print(f"Total cycles with successful Load (0x01/0x00): {sum(1 for s in stats if s['upload_time'] is not None)}")
    print(f"Total cycles with successful Save (0x03/0x00): {sum(1 for s in stats if s['save_time'] is not None)}")
    print(f"Total cycles with successful Delete (0x05/0x00): {sum(1 for s in stats if s['delete_time'] is not None)}")
    print(f"Total data match failures: {sum(1 for s in stats if s.get('data_match') is False and s['download_time'] is not None)}")

    print("#"*60)

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

async def main(address: str):
    """Main function to connect to the BLE device and run tests."""
    ROBOT_BLE_ADDRESS = await find_thymio_device()
    if not ROBOT_BLE_ADDRESS:
        print("Could not find a Thymio device. Exiting.")
        sys.exit(1)
    print(f"Target BLE Address: {ROBOT_BLE_ADDRESS}")
    address = ROBOT_BLE_ADDRESS

    try:
        async with BleakClient(address) as client:
            print(f"Connection established: {client.is_connected}")
            
            # Start notifications/indications for the file system characteristic
            print(f"Starting notification for FS Characteristic (0x{FS_CHAR_UUID})...")
            await client.start_notify(FS_CHAR_UUID, notification_handler)

            for i, size_kb in enumerate(TEST_CYCLES):
                cycle_num = i + 1
                await run_test_cycle(client, FS_CHAR_UUID, size_kb, cycle_num)
                
            # Stop notifications
            await client.stop_notify(FS_CHAR_UUID)

    except Exception as e:
        print(f"\n!!! FATAL ERROR DURING BLE CONNECTION/SETUP !!!\n{type(e).__name__}: {e}")
        
    finally:
        print_statistics()

if __name__ == "__main__":
    
    print(f"File System Char UUID: 0x{FS_CHAR_UUID}")
    print(f"Test Sizes (KB): {sorted(list(set(FILE_SIZES_KB)))}")
    print(f"Total Cycles: {len(TEST_CYCLES)}")

    try:
        # Windows requires a specific event loop policy for Bleak
        if sys.platform == "win32":
            asyncio.set_event_loop_policy(asyncio.WindowsProactorEventLoopPolicy())
        asyncio.run(main(ROBOT_BLE_ADDRESS))
    except KeyboardInterrupt:
        print("\nTest interrupted by user. Generating partial report.")
        print_statistics()
    except Exception as e:
        print(f"\nAn unexpected error occurred in the main execution: {e}")
        sys.exit(1)