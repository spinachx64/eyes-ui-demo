// M5GFX 0.2.28 fills spi_bus_config_t with 0xff before assigning known fields.
// ESP-IDF 6.1 added dma_burst_size, so legacy configs pass UINT32_MAX to GDMA.
// Keep this workaround in the board example; don't modify managed dependencies.
#include <stdint.h>
#include "esp_idf_version.h"
#include "driver/spi_common.h"
#include "esp_log.h"

esp_err_t __real_spi_bus_initialize(spi_host_device_t host,
                                   const spi_bus_config_t *config,
                                   spi_dma_chan_t dma_chan);

esp_err_t __wrap_spi_bus_initialize(spi_host_device_t host,
                                   const spi_bus_config_t *config,
                                   spi_dma_chan_t dma_chan) {
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 1, 0)
    if (config && config->dma_burst_size == UINT32_MAX) {
        spi_bus_config_t fixed = *config;
        fixed.dma_burst_size = 0; // ESP-IDF documented driver default
        ESP_LOGI("eyes_spi", "Using default DMA burst size for legacy SPI config");
        return __real_spi_bus_initialize(host, &fixed, dma_chan);
    }
#endif
    return __real_spi_bus_initialize(host, config, dma_chan);
}
