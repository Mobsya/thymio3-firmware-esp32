set(COMPONENT_SRCS
    accelerometer.c
    angle_controller.c
    ann.c
    aseba_esp32.c
    aseba.c
    attentive.c
    audio_tone_uri.c
    behavior.c
    bh1745nuc.c
    ble_spp.c
    buttons.c
    codec.c
    color_sensor.c
    comm.c
    common.c
    drawer.c
    dsp.c
    efuse_id.c
    explorer.c
    fearful.c
    fifo.c
    file_server.c
    file_system.c
    friendly.c
    gpio.c
    groove.c
    gyroscope.c
    i2c.c
    leds.c
    line_tracker.c
    lsm6ds3us.c
    lsm6ds3tr.c
    lsm6ds0.c
    main.c
    mode.c
    musician.c
    natives.c
    obedient.c
    #painter.c
    power.c
    python_handler.c
    rc5.c
    sensors.c
    sequence.c
    serial_protocol.c
    settings.c
    shift_registers.c
    sn74hc595.c
    spi.c
    stm32_spi.c
    #tcp_server.c
    tcs3701.c
    test.c
    thymio-buffer.c
    thymio_natives.c
    timer_hw.c
    timer_sw.c
    uart.c
    utility.c
    vm-buffer.c
    vm.c
    ../components/thymio-nn/nn/nn.c
    ../components/thymio-nn/nn/nn-alloc-stdlib.c
    ../components/thymio-nn/thymio/nn-descriptions.c
    ../components/thymio-nn/thymio/nn-natives.c
    wifi_handler.c
)

#set(COMPONENT_EMBED_FILES server_certs/ca_cert.pem favicon.ico upload_script.html a3_44100.mp3 alarm_44100.mp3 b3_44100.mp3 bad_44100.mp3 blop_44100.mp3 bye_44100.mp3 c3_44100.mp3 chicken.mp3 d3_44100.mp3 e3_44100.mp3 f3_44100.mp3 fall_44100.mp3 g3_44100.mp3 good_44100.mp3 detect_44100.mp3 magic_44100.mp3 tick_44100.mp3)
set(COMPONENT_EMBED_FILES server_certs/ca_cert.pem favicon.ico upload_script.html)

set(COMPONENT_ADD_INCLUDEDIRS ". ./aseba ../components")

register_component()

