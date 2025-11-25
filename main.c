#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>

static const struct device *adc_dev;

#define ADC_RESOLUTION    12
#define ADC_CHANNEL       0
#define ADC_REFERENCE     ADC_REF_INTERNAL
#define ADC_GAIN          ADC_GAIN_1_6

#define SLEEP_TIME_MS     1000

static const struct adc_channel_cfg chl0_cfg = {
    .gain = ADC_GAIN,
    .reference = ADC_REFERENCE,
    .acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
    .channel_id = ADC_CHANNEL,
#ifdef CONFIG_ADC_NRFX_SAADC
    .input_positive = SAADC_CH_PSELP_PSELP_AnalogInput0,
#endif
};

static int16_t sample_buffer[1];

static const struct adc_sequence sequence = {
    .channels = BIT(ADC_CHANNEL),
    .buffer = sample_buffer,
    .buffer_size = sizeof(sample_buffer),
    .resolution = ADC_RESOLUTION,
    .calibrate = true,
};

int main(void)
{
    int err;
    int32_t adc_vref;

    if (adc_dev == NULL || !device_is_ready(adc_dev)) {
        adc_dev = device_get_binding("ADC_0");
        if (adc_dev == NULL) {
            printk("ERROR: ADC device not found\n");
            return -1;
        }
    }

    k_msleep(1000);
    
    printk("=== ADC Test nRF52840 ===\n");
    printk("ADC device: %s\n", adc_dev->name);

    err = adc_channel_setup(adc_dev, &chl0_cfg);
    if (err != 0) {
        printk("ERROR: ADC setup failed: %d\n", err);
        return -1;
    }
    printk("ADC channel setup successful\n");

    adc_vref = adc_ref_internal(adc_dev);
    printk("ADC reference: %d mV\n", adc_vref);
    printk("Starting ADC readings...\n\n");

    while (1) {
        err = adc_read(adc_dev, &sequence);
        if (err != 0) {
            printk("ADC read error: %d\n", err);
            k_msleep(SLEEP_TIME_MS);
            continue;
        }

        int32_t mv_value = sample_buffer[0];
        adc_raw_to_millivolts(adc_vref, ADC_GAIN, ADC_RESOLUTION, &mv_value);
        
        printk("ADC RAW: %d, Voltage: %d mV\n", sample_buffer[0], mv_value);

        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}