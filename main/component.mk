#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the
# src/ directory, compile them and link them into lib(subdirectory_name).a
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#

COMPONENT_EMBED_TXTFILES := adf_music.mp3 chicken.mp3 dixie_horn.mp3

COMPONENT_ADD_INCLUDEDIRS := ${PROJECT_PATH}/main/aseba

COMPONENT_EXTRA_INCLUDES := ${PROJECT_PATH}/main/aseba
