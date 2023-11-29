/*This is tone file*/

const char* tone_uri[] = {
   "flash://tone/0_a3.mp3",
   "flash://tone/1_alarm.mp3",
   "flash://tone/2_b3.mp3",
   "flash://tone/3_bad.mp3",
   "flash://tone/4_blop.mp3",
   "flash://tone/5_bye.mp3",
   "flash://tone/6_c3.mp3",
   "flash://tone/7_d3.mp3",
   "flash://tone/8_detect.mp3",
   "flash://tone/9_e3.mp3",
   "flash://tone/10_f3.mp3",
   "flash://tone/11_fall.mp3",
   "flash://tone/12_g3.mp3",
   "flash://tone/13_good.mp3",
   "flash://tone/14_magic.mp3",
   "flash://tone/15_tick.mp3",
};

int get_tone_uri_num()
{
    return sizeof(tone_uri) / sizeof(char *) - 1;
}
