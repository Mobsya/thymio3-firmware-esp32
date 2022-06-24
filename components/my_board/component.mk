#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

ifdef CONFIG_AUDIO_BOARD_CUSTOM
COMPONENT_ADD_INCLUDEDIRS += ./es8374_codec
COMPONENT_SRCDIRS += ./es8374_codec

COMPONENT_ADD_INCLUDEDIRS += ./thymio_board
COMPONENT_SRCDIRS += ./thymio_board
endif