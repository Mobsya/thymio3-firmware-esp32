#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the
# src/ directory, compile them and link them into lib(subdirectory_name).a
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#

#COMPONENT_EMBED_TXTFILES := magic_44100.mp3 tick_44100.mp3 blop_44100.mp3 fall_44100.mp3 detect_44100.mp3 bye_44100.mp3 c3_44100.mp3 d3_44100.mp3 e3_44100.mp3 f3_44100.mp3 g3_44100.mp3 a3_44100.mp3 b3_44100.mp3
#COMPONENT_EMBED_TXTFILES := magic_44100.mp3 tick_44100.mp3 blop_44100.mp3 fall_44100.mp3 detect_44100.mp3 bye_44100.mp3 tick_8000.mp3 tick_11025.mp3 tick_16000.mp3 tick_22050.mp3 tick_32000.mp3 tick_48000.mp3
#COMPONENT_EMBED_TXTFILES := magic_44100.mp3 tick_44100.mp3 blop_44100.mp3 fall_44100.mp3 detect_44100.mp3 bye_44100.mp3 bell.mp3 blopblop.mp3 tuck.mp3 blop2.mp3 tick_8000.mp3 tick_11025.mp3 tick_16000.mp3 tick_22050.mp3 tick_32000.mp3 tick_44100.mp3 tick_48000.mp3

COMPONENT_EMBED_FILES := magic_44100.mp3
COMPONENT_EMBED_FILES += tick_44100.mp3
COMPONENT_EMBED_FILES += blop_44100.mp3
COMPONENT_EMBED_FILES += fall_44100.mp3
COMPONENT_EMBED_FILES += detect_44100.mp3
COMPONENT_EMBED_FILES += alarm_44100.mp3
COMPONENT_EMBED_FILES += bye_44100.mp3
COMPONENT_EMBED_FILES += c3_44100.mp3
COMPONENT_EMBED_FILES += d3_44100.mp3
COMPONENT_EMBED_FILES += e3_44100.mp3
COMPONENT_EMBED_FILES += f3_44100.mp3
COMPONENT_EMBED_FILES += g3_44100.mp3
COMPONENT_EMBED_FILES += a3_44100.mp3
COMPONENT_EMBED_FILES += b3_44100.mp3
COMPONENT_EMBED_FILES += favicon.ico
COMPONENT_EMBED_FILES += upload_script.html

COMPONENT_ADD_INCLUDEDIRS := ${PROJECT_PATH}/main/aseba

COMPONENT_EXTRA_INCLUDES := ${PROJECT_PATH}/main/aseba


