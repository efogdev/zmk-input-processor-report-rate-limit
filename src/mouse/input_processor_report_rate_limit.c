/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_report_rate_limit

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/keymap.h>

struct zip_rrl_config {
    uint32_t report_ms;
};

struct zip_rrl_data {
    const struct device *dev;
    int64_t last_rpt_time_x;
    int64_t last_rpt_time_y;
    int16_t x;
    int16_t y;
    bool sync_x;
    bool sync_y;
};

static int zip_rrl_handle_event(const struct device *dev, struct input_event *event, 
                                uint32_t param1, uint32_t param2, 
                                struct zmk_input_processor_state *state) {

    const struct zip_rrl_config *config = dev->config;
    struct zip_rrl_data *data = dev->data;

    int64_t now = k_uptime_get();
    int16_t val = event->value;
    bool sync = event->sync;
    event->value = 0;
    event->sync = false;

    if (event->code == INPUT_REL_X) {
        data->x += val;
        data->sync_x = data->sync_x || sync;
        if (now - data->last_rpt_time_x > config->report_ms) {
            if (data->x != 0 || sync) {
                data->last_rpt_time_x = now;
                event->value = data->x;
                event->sync = data->sync_x;
                data->x = 0;
                data->sync_x = false;
                return ZMK_INPUT_PROC_CONTINUE;
            }
        }
    }

    if (event->code == INPUT_REL_Y) {
        data->y += val;
        data->sync_y = data->sync_y || sync;
        if (now - data->last_rpt_time_y > config->report_ms) {
            if (data->y != 0 || sync) {
                data->last_rpt_time_y = now;
                event->value = data->y;
                event->sync = data->sync_y;
                data->y = 0;
                data->sync_y = false;
                return ZMK_INPUT_PROC_CONTINUE;
            }
        }
    }

    return ZMK_INPUT_PROC_STOP;
}

static struct zmk_input_processor_driver_api sy_driver_api = {
    .handle_event = zip_rrl_handle_event,
};

static int zip_rrl_init(const struct device *dev) {
    // const struct zip_rrl_config *config = dev->config;
    struct zip_rrl_data *data = dev->data;
    data->dev = dev;
    return 0;
}

#define TL_INST(n)                                                                        \
    static struct zip_rrl_data data_##n = {};                                             \
    static struct zip_rrl_config config_##n = {                                           \
        .report_ms = DT_INST_PROP(n, report_ms),                                          \
    };                                                                                    \
    DEVICE_DT_INST_DEFINE(n, &zip_rrl_init, NULL, &data_##n, &config_##n, POST_KERNEL,    \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &sy_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TL_INST)
