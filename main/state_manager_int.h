
// On-board addressable RGB LED.  On this clone board it is driven from
// GPIO48 (per the board documentation; the official DevKitC-1 v1.1 uses
// GPIO38 instead).
#define STATE_LED_GPIO     GPIO_NUM_48
#define STATE_LED_CHANNEL  0

typedef struct
{
    uint8_t state;
    led_strip_t* led_strip;
    portMUX_TYPE mux;
}
state_manager_driver_t;