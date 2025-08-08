const CHUNK_SIZE = 500;

let writeChar, readChar, ackChar;
let currentSeq = 1;
let pendingAck = false;
let reconnecting = false;
let device;

/**
 * Calculate a CRC code for a string.
 * @returns CRC32 code for the string
 */
function crc32(str) {
  let crc = 0xFFFFFFFF;
  for (let i = 0; i < str.length; i++) {
    let byte = str.charCodeAt(i);
    crc ^= byte;
    for (let j = 0; j < 8; j++) {
      let mask = -(crc & 1);
      crc = (crc >>> 1) ^ (0xEDB88320 & mask);
    }
  }
  return (crc ^ 0xFFFFFFFF) >>> 0;
}

/**
 * Request a bluetooth device and connect to it.
 */
async function requestAndConnect() {
  device = await navigator.bluetooth.requestDevice({
    filters: [{ namePrefix: 'ESP32' }],
    optionalServices: ['12345678-1234-1234-1234-1234567890ab']
  });

  // To handle the reconnects
  device.addEventListener('gattserverdisconnected', onDisconnected);

  await connect();
}

/**
 * Connect to the device and to all of the exposed services and characteristics.
 */
async function connect() {
  const server = await device.gatt.connect();
  const service = await server.getPrimaryService('12345678-1234-1234-1234-1234567890ab');

  writeChar = await service.getCharacteristic('12345678-1234-1234-1234-1234567890ac');
  readChar  = await service.getCharacteristic('12345678-1234-1234-1234-1234567890ad');
  ackChar   = await service.getCharacteristic('12345678-1234-1234-1234-1234567890ae');

  await ackChar.startNotifications();
  ackChar.addEventListener('characteristicvaluechanged', handleAck);

  console.log("✅ Connected to ESP32");
}

function onDisconnected() {
  console.log('⚠️ Disconnected. Attempting to reconnect...');
  if (!reconnecting) {
    reconnecting = true;
    retryConnection();
  }
}

async function retryConnection() {
  let attempts = 0;
  const maxAttempts = 10;

  while (attempts < maxAttempts) {
    try {
      await delay(2000);  // Wait 2 seconds before retry
      if (!device.gatt.connected) {
        await connect();
        reconnecting = false;
        return;
      }
    } catch (e) {
      console.warn(`Retry ${attempts + 1} failed:`, e);
    }
    attempts++;
  }

  console.log(`❌ Failed to reconnect after ${attempts} attempts`);
}

function handleAck(event) {
  const value = new TextDecoder().decode(event.target.value);
  console.log("ACK:", value);
  if (value.startsWith("ACK:")) {
    const ackSeq = parseInt(value.split(":")[1]);
    if (ackSeq === currentSeq) {
      pendingAck = true;
    }
  } else if (value.startsWith("MSG_RECEIVED:")) {
    const crc = value.split(":")[1];
    alert("✅ ESP32 received full message. CRC: " + crc);
  } else if (value === "MSG_CORRUPTED") {
    alert("❌ Message corrupted. Please retry.");
  }
}

/**
 * Chunk a string into chunks of a predetermined size.
 */
function chunkString(str, size) {
  const chunks = [];
  for (let i = 0; i < str.length; i += size) {
    chunks.push(str.slice(i, i + size));
  }
  return chunks;
}

function delay(timeout) {
  return new Promise(resolve => {
    setTimeout(() => resolve(), timeout);
  });
}

async function sendData(text) {
  const crc = crc32(text);
  const chunks = chunkString(text, CHUNK_SIZE);

  console.log(`Chunks to send: ${chunks.length}`);

  for (let i = 0; i < chunks.length; i++) {
    pendingAck = false;
    currentSeq = i + 1;

    let data = chunks[i];
    if (i === chunks.length - 1) {
      data += "<END>:" + crc.toString(16).padStart(8, '0');
    }

    const chunkPayload = `${currentSeq}|${data}`;
    const encoded = new TextEncoder().encode(chunkPayload);

    let retry = 0;
    while (!pendingAck && retry < 3) {
      console.log(`Sending chunk ${currentSeq}, attempt ${retry + 1}`);
      await writeChar.writeValue(encoded);
      await delay(1);
      retry++;
    }

   if (!pendingAck) {
      alert(`❌ Failed to send chunk ${currentSeq}.`);
      return;
    }
  }
}

async function sendFileData() {
  const file = document.getElementById('fileInput').files[0];
  if (!file) return alert("Select a file");

  const arrayBuffer = await file.arrayBuffer();
  const base64String = btoa(
    new Uint8Array(arrayBuffer).reduce((data, byte) => data + String.fromCharCode(byte), '')
  );

  await sendData(base64String);
  console.log('✅ File sent');
}

async function readFakeData() {
  while(true) {
    const rawValue = await readChar.readValue();
    const value = new TextDecoder().decode(rawValue);
    console.log(value);
    setTimeout(() => {}, 10);
  }
}
