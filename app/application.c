#include <application.h>

#define SERVICE_INTERVAL_INTERVAL (60 * 60 * 1000)
#define BATTERY_UPDATE_INTERVAL   (60 * 60 * 1000)

#define UPDATE_SERVICE_INTERVAL   (5 * 1000)
#define UPDATE_NORMAL_INTERVAL    (10 * 1000)

#define TEMPERATURE_TAG_PUB_NO_CHANGE_INTEVAL (15 * 60 * 1000)
#define TEMPERATURE_TAG_PUB_VALUE_CHANGE 0.6f

#define LUX_METER_TAG_PUB_NO_CHANGE_INTEVAL (15 * 60 * 1000)
#define LUX_METER_TAG_PUB_VALUE_CHANGE 60.0f

// LED instance
bc_led_t led;

// Temp meter instance
bc_tmp112_t temp;

// Lux meter instance
bc_tag_lux_meter_t lux;

event_param_t temperature;
event_param_t iluminance;


void battery_event_handler(bc_module_battery_event_t event, void *event_param)
{
    (void) event_param;

    float voltage;

    if (event == BC_MODULE_BATTERY_EVENT_UPDATE)
    {
        if (bc_module_battery_get_voltage(&voltage))
        {
            bc_radio_pub_battery(&voltage);
        }
    }
}


void tmp112_event_handler(bc_tmp112_t *self, bc_tmp112_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    float tempValue;

    if (event == BC_TMP112_EVENT_UPDATE)
    {
        if (bc_tmp112_get_temperature_celsius(&temp, &tempValue))
        {
            bc_log_debug("%f", tempValue);
            if ((fabs(tempValue - temperature.value) >= TEMPERATURE_TAG_PUB_VALUE_CHANGE) || (temperature.next_pub < bc_scheduler_get_spin_tick()))
            {
                bc_radio_pub_temperature(BC_I2C_I2C0, &tempValue);
                temperature.value = tempValue;
                temperature.next_pub = bc_scheduler_get_spin_tick() + TEMPERATURE_TAG_PUB_NO_CHANGE_INTEVAL;
            }
        }
    }
}

void lux_meter_event_handler(bc_tag_lux_meter_t *self, bc_tag_lux_meter_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    float iluminanceValue;

    if (event == BC_TAG_LUX_METER_EVENT_UPDATE)
    {
        if (bc_tag_lux_meter_get_illuminance_lux(&lux, &iluminanceValue))
        {
            bc_log_debug("%f", iluminanceValue);
            if ((fabs(iluminanceValue - iluminance.value) >= LUX_METER_TAG_PUB_VALUE_CHANGE) || (iluminance.next_pub < bc_scheduler_get_spin_tick()))
            {
                bc_radio_pub_temperature(BC_TAG_LUX_METER_I2C_ADDRESS_DEFAULT, &iluminanceValue);
                iluminance.value = iluminanceValue;
                iluminance.next_pub = bc_scheduler_get_spin_tick() + LUX_METER_TAG_PUB_NO_CHANGE_INTEVAL;
            }
        }
    }
}


void switch_to_normal_mode_task(void *param)
{
    bc_tmp112_set_update_interval(&temp, UPDATE_NORMAL_INTERVAL);
    bc_tag_lux_meter_set_update_interval(&lux, UPDATE_NORMAL_INTERVAL);

    bc_scheduler_unregister(bc_scheduler_get_current_task_id());
}


void application_init(void)
{
    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);

    bc_log_init(BC_LOG_LEVEL_DUMP, BC_LOG_TIMESTAMP_ABS);

    bc_tmp112_init(&temp, BC_I2C_I2C0, 0x49);
    bc_tmp112_set_update_interval(&temp, UPDATE_SERVICE_INTERVAL);

    // set measurement handler (call "tmp112_event_handler()" after measurement)
    bc_tmp112_set_event_handler(&temp, tmp112_event_handler, NULL);

    bc_tag_lux_meter_init(&lux, BC_I2C_I2C0, BC_TAG_LUX_METER_I2C_ADDRESS_DEFAULT);
    bc_tag_lux_meter_set_update_interval(&lux, UPDATE_SERVICE_INTERVAL);
    bc_tag_lux_meter_set_event_handler(&lux, lux_meter_event_handler, NULL);

    // Send pairing request
    bc_radio_init(BC_RADIO_MODE_NODE_SLEEPING);
    bc_radio_pairing_request("fridge-monitor", VERSION);

    bc_module_battery_init();
    bc_module_battery_set_event_handler(battery_event_handler, NULL);
    bc_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);


    bc_scheduler_register(switch_to_normal_mode_task, NULL, SERVICE_INTERVAL_INTERVAL);

    bc_led_pulse(&led, 2000);

}
