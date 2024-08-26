idf_component_register(
	SRCS
		"main.c"
		"my_gpio.c"
		"drv8833_pwm.c"
		"my_wifi.c"
		"sound.c"
		"led_strip.c"
		"led_strip_encoder.c"
		"my_adc.c"
		"handle_message.c"
		"my_play_mp3.c"
		"audio_convert.c"
		"cnv_lib/cnv_audio/cnv_audio.c"
		"esp_color.c"
		"bt_gatts.c"
		"utils.c"
		"my_ucp.c"
	INCLUDE_DIRS "."
		"cnv_lib/cnv_audio"
		"cnv_lib/cnv_basic"
	EMBED_FILES
		"mmfile/didi_1c_16khz.mp3"
		"mmfile/gun_1c_16khz.mp3"
		"mmfile/aiyaya_1c_16khz_23s.mp3"
		"mmfile/canon_1c_16khz_21s.mp3"
		"mmfile/Croatian_1c_16khz_12s.mp3"
		"mmfile/Galway_1c_16khz_18s.mp3"
		"mmfile/Vivacity_1c_16khz_20s.mp3")

add_prebuilt_library(convert "${CMAKE_CURRENT_SOURCE_DIR}/cnv_lib/lib/${CONFIG_IDF_TARGET}/libconvert.a")

target_link_libraries(${COMPONENT_TARGET} INTERFACE convert)