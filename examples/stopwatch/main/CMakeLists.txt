idf_component_register(SRCS "main.cpp" "spi_compat.c" INCLUDE_DIRS "."
                       REQUIRES eyes m5unified esp_timer esp_hw_support esp_driver_spi)
# Normalize M5GFX 0.2.28's unset dma_burst_size on ESP-IDF 6.1.
# Valid SPI configs pass through untouched; see spi_compat.c.
target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=spi_bus_initialize"
                      "-Wl,--undefined=__wrap_spi_bus_initialize")
