// Commands definition: from the device (e.g. phone) to the Thymio
#define CMD_SET_MOST_ACTUATORS 0x01
// Circle LEDs => 4 bytes, brightness from 0..15
// Front lego LEDs => 4 bytes, brightness from 0..15
// Rear lego LEDs => 4 bytes, brightness from 0..15
// RGB front left => 2 bytes (r bit0..3, g bit4..7, b bit8..11)
// RGB front right => 2 bytes (r bit0..3, g bit4..7, b bit8..11)
// RGB back left => 2 bytes (r bit0..3, g bit4..7, b bit8..11)
// RGB back right => 2 bytes (r bit0..3, g bit4..7, b bit8..11)
// Motor left => 2 bytes -1000..1000
// Motor right => 2 bytes -1000..1000
// Sound => 1 byte (
//    0 = magic,
//    1 = Tick,
//    2 = Blop,
//    3 = Fall,
//    4 = Detection,
//    5 = Bye,
//    6 = C3,
//    7 = D3,
//    8 = E3,
//    9 = F3,
//    10 = G3,
//    11 = A3,
//    12 = B3,
//    13 = Alarm,
//    14 = Good,
//    15 = Bad)
#define CMD_SET_MOST_ACTUATORS_LEN 25

#define CMD_SET_OTHERS_ACTUATORS 0x02
// Rotate => 4 bytes => 2 bytes (angle -360..360), 2 bytes (speed -1000..1000)
// IMU flags => 1 byte => bit0: reset angle, bit1: clear tap event, bit2: clear freefall event
// RGB small bottom => 2 bytes (r bit0..3, g bit4..7, b bit8..11)
// RGB small back => 2 bytes (r bit0..3, g bit4..7, b bit8..11)
// Buttons LEDs => 4 bytes, brightness from 0..15
// Receiver LED + microphone LED => 1 byte (receiver bit0..3, microphone bit4)
// Set volume => 1 byte => vol = 0..10 (bit0..3), save volume in flash bit4
// Behaviors enabling/disabling => 2 bytes
#define CMD_SET_OTHERS_ACTUATORS_LEN 17

// Responses definition: from the Thymio to the device (e.g. phone)
#define RSP_MOST_SENSORS 0x01
// color sensor => 4 bytes => H (2), S (1), V (1)
// ground sensors => 4 bytes => left (2), right (2)
// acceleration raw => 6 bytes => x (2), y (2), z (2)
// gyro raw => 6 bytes => x (2), y (2), z (2)
// buttons => 1 byte
// microphone volume => 2 bytes
// proximity sensors => 14 bytes => left (2), front left (2), center (2), front right (2), right (2), back left (2), back right (2)
// tv remote => 1 byte
#define RSP_MOST_SENSORS_LEN 38

#define RSP_OTHERS_SENSORS 0x02
// color raw values => 8 bytes (red, green, blue, clear)
// color detected => 1 byte
// ground ambient => 4 bytes => left (2), right (2)
// ground reflected => 4 bytes => left (2), right (2)
// angle degrees => 2 bytes
// events flags => 1 byte => bit0 tap detected, bit1 freefall detected, bit2 clap detected
// motor left speed => 2 bytes
// motor right speed => 2 bytes
// motor left pwm duty => 2 bytes
// motor right pwm duty => 2 bytes
// battery voltage => 2 bytes
#define RSP_OTHERS_SENSORS_LEN 30

void ble_spp_init(void);