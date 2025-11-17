  
#include "i2c.h"
#include "esp_log.h"

void groove_write_dummy(void)
{
    static uint8_t data = 0x00u;
    I2C_WriteToAddress(0x42, 0, &data, 1u);
    ESP_LOGI("groove", "write %d", data);
    if(data == 0xFF)
    {
        data = 0;
    }
    else
    {
        data++;
    }

}
