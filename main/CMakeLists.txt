idf_component_register(
	SRCS
		"main.c"
		"my_httpd.c"
		"my_gpio.c"
		"drv8833_pwm.c"
		"my_wifi.c"
		"sound.c"
		"led_strip.c"
		"led_strip_encoder.c"
		"my_adc.c"
		"my_udp.c"
		"handle_message.c"
		"my_play_mp3.c"
	INCLUDE_DIRS "."
	EMBED_FILES
		"mmfile/didi_1c_16khz.mp3"
		"mmfile/gun_1c_16khz.mp3"
		"mmfile/aiyaya_1c_16khz_23s.mp3"
		"mmfile/canon_1c_16khz_21s.mp3"
		"mmfile/Croatian_1c_16khz_12s.mp3"
		"mmfile/Galway_1c_16khz_18s.mp3"
		"mmfile/Vivacity_1c_16khz_20s.mp3")

set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../web")
if(EXISTS ${WEB_SRC_DIR}/dist)
	spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
else()
	message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit. Please run 'npm run build' in ${WEB_SRC_DIR}")
endif()
