/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "io_assignment.h"


const static char *TAG = "ADC";

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/


//static int adc_raw[2][10];
//static int voltage[2][10];
static int g_adc_raw;
static int g_voltage;
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);


static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_chan_handle = NULL;

static bool do_calibration1;

/* @return: the voltage, mV */
int adc_get_voltage()
{
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &g_adc_raw));
    if (do_calibration1) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan_handle, g_adc_raw, &g_voltage));
        //ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, g_voltage);
        return g_voltage;
    }

    return 0;
}

void adc_deinit()
{
    //Tear Down
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    if (do_calibration1) {
        example_adc_calibration_deinit(adc1_cali_chan_handle);
    }
}

/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}

/***********************adc voltage check task************************/
#if 0
typedef struct {
    volatile bool                    task_run;
    EventGroupHandle_t               state_event;
    void                            *args;
} voltage_handle_t;

static voltage_handle_t g_voltage_handler;

static void voltage_task(void *args)
{
    voltage_handle_t *handle = (voltage_handle_t *)args;

    while (handle->task_run) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        adc_get_voltage();


    }
    xEventGroupSetBits(handle->state_event, BIT0);
    vTaskDelete(NULL);
}
static esp_err_t voltage_task_wait_for_stop(void)
{
    EventBits_t uxBits = xEventGroupWaitBits(g_voltage_handler.state_event, BIT0, true, true, 5000 / portTICK_RATE_MS);
    esp_err_t ret = ESP_ERR_TIMEOUT;
    if (uxBits & BIT0) {
        ret = ESP_OK;
    } else {
        ESP_LOGE(TAG, "wait for stop timeout.");
    }
    return ret;
}
esp_err_t voltage_task_stop(void)
{
    voltage_handle_t *handle = &g_voltage_handler;
    if (false == handle->task_run) {
        return ESP_OK;
    }

    xEventGroupClearBits(handle->state_event, BIT0);
    handle->task_run = false;
    esp_err_t ret = voltage_task_wait_for_stop();

    return ret;
}
esp_err_t voltage_task_run(void)
{
    g_voltage_handler.task_run = true;
    xTaskCreate(voltage_task, "voltage_task", 4096, &g_voltage_handler, 5, NULL);
    return ESP_OK;
}
void voltage_task_init_and_run()
{
    g_voltage_handler.state_event = xEventGroupCreate();
    voltage_task_run();
}
#endif
/***********************adc voltage check task************************/


void adc_init()
{
    //-------------ADC1 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN0, &config));

    //-------------ADC1 Calibration Init---------------//
    do_calibration1 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN0, ADC_ATTEN_DB_11, &adc1_cali_chan_handle);

    //voltage_task_init_and_run();
}



