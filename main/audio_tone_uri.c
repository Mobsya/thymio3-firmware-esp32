/*This is tone file*/

const char* tone_uri[] = {
   "flash://tone/0_a3_44100.mp3",
   "flash://tone/1_alarm_44100.mp3",
   "flash://tone/2_b3_44100.mp3",
   "flash://tone/3_bad_44100.mp3",
   "flash://tone/4_blop_44100.mp3",
   "flash://tone/5_bye_44100.mp3",
   "flash://tone/6_c3_44100.mp3",
   "flash://tone/7_d3_44100.mp3",
   "flash://tone/8_detect_44100.mp3",
   "flash://tone/9_e3_44100.mp3",
   "flash://tone/10_f3_44100.mp3",
   "flash://tone/11_fall_44100.mp3",
   "flash://tone/12_g3_44100.mp3",
   "flash://tone/13_good_44100.mp3",
   "flash://tone/14_magic_44100.mp3",
   "flash://tone/15_tick_44100.mp3",
};

int get_tone_uri_num()
{
    return sizeof(tone_uri) / sizeof(char *) - 1;
}
