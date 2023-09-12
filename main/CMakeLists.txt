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
	INCLUDE_DIRS "."
	EMBED_FILES
		"gun_s8_16k_mono.pcm"
		"didi_s8_16k_mono.pcm")

set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../web")
if(EXISTS ${WEB_SRC_DIR}/dist)
	spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
else()
	message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit. Please run 'npm run build' in ${WEB_SRC_DIR}")
endif()
