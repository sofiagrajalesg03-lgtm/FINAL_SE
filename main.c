

#include <stdint.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#define SPI_HOST_ID            SPI2_HOST
#define PIN_SPI_MISO           19
#define PIN_SPI_MOSI           23
#define PIN_SPI_SCLK           18
#define PIN_SPI_CS             5
#define SPI_CLOCK_HZ           1000000

#define MCP4132_CMD_WRITE      0x00
#define MCP4132_CMD_READ       0x0C
#define MCP4132_ADDR_MASK      0x0F
#define MCP4132_DATA_MASK      0xFF
#define MCP4132_WIPER0_ADDR    0x00
#define MCP4132_WIPER_MAX      128

//punto 1
static const char *TAG = "spi_init";

esp_err_t spi_bus_init(spi_device_handle_t *out_handle)
{
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_SPI_MISO,
        .mosi_io_num = PIN_SPI_MOSI,
        .sclk_io_num = PIN_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };

    esp_err_t ret = spi_bus_initialize(SPI_HOST_ID, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return ret;
    }

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_CLOCK_HZ,
        .mode = 0,
        .spics_io_num = PIN_SPI_CS,
        .queue_size = 1,
        .flags = 0,
    };

    ret = spi_bus_add_device(SPI_HOST_ID, &devcfg, out_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(ret));
        spi_bus_free(SPI_HOST_ID);
    }

    return ret;
}

esp_err_t mcp4132_write_register(spi_device_handle_t spi, uint8_t address, uint8_t value)
{
    uint8_t packet[2] = {
        (uint8_t)((MCP4132_CMD_WRITE << 4) | (address & MCP4132_ADDR_MASK)),
        value
    };

    spi_transaction_t trans = {
        .length = 16,
        .tx_buffer = packet,
        .rx_buffer = NULL,
    };

    return spi_device_transmit(spi, &trans);
}

uint8_t mcp4132_read_register(spi_device_handle_t spi, uint8_t address, esp_err_t *out_error)
{
    uint8_t tx_data[2] = {
        (uint8_t)((MCP4132_CMD_READ << 4) | (address & MCP4132_ADDR_MASK)),
        0x00
    };
    uint8_t rx_data[2] = {0};

    spi_transaction_t trans = {
        .length = 16,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };

    esp_err_t err = spi_device_transmit(spi, &trans);
    if (out_error != NULL) {
        *out_error = err;
    }

    if (err != ESP_OK) {
        return 0;
    }

    return (uint8_t)(rx_data[1] & MCP4132_DATA_MASK);
}

esp_err_t mcp4132_set_wiper(spi_device_handle_t spi, uint8_t value)
{
    if (value > MCP4132_WIPER_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    return mcp4132_write_register(spi, MCP4132_WIPER0_ADDR, value);
}


