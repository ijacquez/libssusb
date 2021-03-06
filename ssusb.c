/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <ssusb/ssusb.h>

#include "debug.h"
#include "drivers/driver.h"

extern const ssusb_device_driver_t device_datalink_red;
extern const ssusb_device_driver_t device_datalink_green;
extern const ssusb_device_driver_t device_datalink_bluetooth;
extern const ssusb_device_driver_t device_usb_cartridge;

static const ssusb_device_driver_t *_device_drivers[] = {
        &device_usb_cartridge,
        &device_datalink_red,
        &device_datalink_green,
        &device_datalink_bluetooth,
        NULL
};

static bool _initialized = false;
static ssusb_driver_t *_driver_list = NULL;
static const ssusb_device_driver_t *_device_driver = NULL;

static const ssusb_device_driver_t *_drivers_get(const char *driver_name);

ssusb_ret_t
ssusb_init(void)
{
        ssusb_deinit();

        ssusb_driver_t **driver_list = &_driver_list;

        const ssusb_device_driver_t **device_driver =
            _device_drivers;

        while (*device_driver != NULL) {
                *driver_list = malloc(sizeof(ssusb_driver_t));

                ssusb_driver_t * const driver = *driver_list;

                if (driver == NULL) {
                        return SSUSB_INIT_ERROR;
                }

                (void)memset(driver, 0, sizeof(ssusb_driver_t));

                driver->name = (*device_driver)->name;
                driver->description = (*device_driver)->description;

                driver_list = &driver->next;

                device_driver++;
        }

        _initialized = true;

        return SSUSB_OK;
}

void
ssusb_deinit(void)
{
        if (!_initialized) {
                return;
        }

        while (_driver_list != NULL) {
                ssusb_driver_t * const next_driver = _driver_list->next;

                free(_driver_list);

                _driver_list = next_driver;
        }

        ssusb_drivers_deselect();

        _initialized = false;
}

ssusb_ret_t
ssusb_drivers_list_get(const ssusb_driver_t **driver_list)
{
        assert(_initialized);
        assert(driver_list != NULL);

        *driver_list = _driver_list;

        return SSUSB_OK;
}

ssusb_ret_t
ssusb_drivers_select(const char *driver_name)
{
        assert(_initialized);

        if ((driver_name == NULL) || (*driver_name == '\0')) {
                return SSUSB_SELECT_INVALID_NAME;
        }
        ssusb_ret_t ret;

        ret = ssusb_drivers_deselect();
        if (ret != SSUSB_OK) {
                return ret;
        }

        const ssusb_device_driver_t * const select_driver =
            _drivers_get(driver_name);

        if (select_driver == NULL) {
                return SSUSB_SELECT_NOT_FOUND;
        }

        if ((select_driver->init()) != 0) {
                return SSUSB_SELECT_INIT_ERROR;
        }

        _device_driver = select_driver;

        return SSUSB_OK;
}

ssusb_ret_t
ssusb_drivers_detect_select(void)
{
        const ssusb_driver_t *driver;
        ssusb_drivers_list_get(&driver);

        while (driver != NULL) {
                DEBUG_PRINTF("Detecting %s...\n", driver->name);

                if ((ssusb_drivers_select(driver->name)) == SSUSB_OK) {
                        DEBUG_PRINTF("Found\n");
                        return SSUSB_OK;
                }

                driver = driver->next;
        }

        DEBUG_PRINTF("No device driver found\n");

        return SSUSB_SELECT_NOT_FOUND;
}

ssusb_ret_t
ssusb_drivers_deselect(void)
{
        assert(_initialized);

        if (_device_driver != NULL) {
                if ((_device_driver->deinit()) != 0) {
                        _device_driver = NULL;

                        return SSUSB_DESELECT_DEINIT_ERROR;
                }
        }

        _device_driver = NULL;

        return SSUSB_OK;
}

ssusb_ret_t
ssusb_drivers_selected_get(const ssusb_device_driver_t **device)
{
        assert(_initialized);
        assert(device != NULL);

        *device = _device_driver;

        return SSUSB_OK;
}

static const ssusb_device_driver_t *
_drivers_get(const char *driver_name)
{
        const ssusb_device_driver_t **device_driver =
            _device_drivers;

        while (*device_driver != NULL) {
                if ((strncmp(driver_name, (*device_driver)->name, SSUSB_DRIVER_NAME_SIZE)) == 0) {
                        return *device_driver;
                }

                device_driver++;
        }

        return NULL;
}
