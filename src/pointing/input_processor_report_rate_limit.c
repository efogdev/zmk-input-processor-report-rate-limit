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
    uint8_t type;
    size_t codes_len;
    uint16_t codes[];
};

struct zip_rrl_data {
    int16_t rmds[CONFIG_ZMK_INPUT_PROCESSOR_REPORT_RATE_LIMIT_CODES_MAX_LEN];
    bool syncs[CONFIG_ZMK_INPUT_PROCESSOR_REPORT_RATE_LIMIT_CODES_MAX_LEN];
    int64_t last_rpt[CONFIG_ZMK_INPUT_PROCESSOR_REPORT_RATE_LIMIT_CODES_MAX_LEN];
};

static int limit_val(const struct device *dev, struct input_event *event, 
                     int code_idx, uint32_t delay_ms,
                     struct zmk_input_processor_state *state) {

    // const struct zip_rrl_config *cfg = dev->config;
    struct zip_rrl_data *data = dev->data;
    int64_t now = k_uptime_get();

    // purge leftover delta, if last reported had not been left too long
    if (now - data->last_rpt[code_idx] >= delay_ms * CONFIG_ZMK_INPUT_PROCESSOR_REPORT_RATE_LIMIT_CODES_MAX_LEN) {
        data->rmds[code_idx] = 0;
        data->syncs[code_idx] = false;
    }

    // accumulate delta, stop provessing
    if (now - data->last_rpt[code_idx] < delay_ms) {
        data->rmds[code_idx] += event->value;
        data->syncs[code_idx] |= event->sync;
        event->value = 0;
        event->sync = false;
        // LOG_DBG("rate limited");
        return ZMK_INPUT_PROC_STOP;
    }

    // flush delta, continue provessing
    event->value += data->rmds[code_idx];
    event->sync |= data->syncs[code_idx];
    // LOG_DBG("c: %d v: %d r: %d", event->code, event->value, data->rmds[code_idx]);
    data->rmds[code_idx] = 0;
    data->syncs[code_idx] = false;
    data->last_rpt[code_idx] = now;

    return ZMK_INPUT_PROC_CONTINUE;
}

static int zip_rrl_handle_event(const struct device *dev, struct input_event *event, 
                                uint32_t param1, uint32_t param2, 
                                struct zmk_input_processor_state *state) {

    const struct zip_rrl_config *cfg = dev->config;
    if (event->type != cfg->type) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    for (int i = 0; i < cfg->codes_len; i++) {
        if (cfg->codes[i] == event->code) {
            return limit_val(dev, event, i, param1, state);
        }
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static struct zmk_input_processor_driver_api sy_driver_api = {
    .handle_event = zip_rrl_handle_event,
};

static int zip_rrl_init(const struct device *dev) {
    // const struct zip_rrl_config *cfg = dev->config;
    struct zip_rrl_data *data = dev->data;

    int64_t now = k_uptime_get();
    for (int i = 0; i < CONFIG_ZMK_INPUT_PROCESSOR_REPORT_RATE_LIMIT_CODES_MAX_LEN; i++) {
        data->rmds[i] = 0;
        data->syncs[i] = false;
        data->last_rpt[i] = now;
    }

    return 0;
}

#define RRL_INST(n)                                                                            \
    BUILD_ASSERT(DT_INST_PROP_LEN(n, codes)                                                    \
                 <= CONFIG_ZMK_INPUT_PROCESSOR_REPORT_RATE_LIMIT_CODES_MAX_LEN,                \
                 "Codes length > CONFIG_ZMK_INPUT_PROCESSOR_REPORT_RATE_LIMIT_CODES_MAX_LEN"); \
    static struct zip_rrl_data data_##n = {};                                                  \
    static struct zip_rrl_config config_##n = {                                                \
        .type = DT_INST_PROP_OR(n, type, INPUT_EV_REL),                                        \
        .codes_len = DT_INST_PROP_LEN(n, codes),                                               \
        .codes = DT_INST_PROP(n, codes),                                                       \
    };                                                                                         \
    DEVICE_DT_INST_DEFINE(n, &zip_rrl_init, NULL, &data_##n, &config_##n, POST_KERNEL,         \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &sy_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RRL_INST)
