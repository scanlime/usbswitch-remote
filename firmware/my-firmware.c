#include <stdint.h>
#include <stdbool.h>

struct gpio {
    uint8_t odr, idr, ddr, cr1, cr2;
};

typedef volatile struct gpio* gpio_t;

#define GPIO_A  ((gpio_t)0x5000)
#define GPIO_B  ((gpio_t)0x5005)
#define GPIO_C  ((gpio_t)0x500a)
#define GPIO_D  ((gpio_t)0x500f)

#define PIN_LED1        GPIO_C, (1U<<7)
#define PIN_LED2        GPIO_C, (1U<<6)
#define PIN_LED3        GPIO_C, (1U<<5)
#define PIN_LED4        GPIO_C, (1U<<4)

#define PIN_SEL_01      GPIO_B, (1U<<4)     // Used for 0 and 1
#define PIN_SEL_23      GPIO_B, (1U<<5)     // Used for 2 and 3
#define PIN_SEL_MSB     GPIO_D, (1U<<2)     // Select between 01 / 23

#define PIN_TUCOPLEX_S  GPIO_D, (1U<<5)     // Sleeve, via 270 ohm
#define PIN_TUCOPLEX_R  GPIO_C, (1U<<3)     // Ring, via 270 ohm
#define PIN_TUCOPLEX_T  GPIO_D, (1U<<3)     // Tip, via 270 ohm

#define PIN_SENSE1      GPIO_A, (1U<<1)
#define PIN_SENSE2      GPIO_A, (1U<<2)
#define PIN_SENSE3      GPIO_D, (1U<<4)
#define PIN_SENSE4      GPIO_A, (1U<<3)

#define PIN_BUTTON      GPIO_D, (1U<<6)

static void led_delay()
{
    // Arbitrary time to display LED, about 15ms
    for (uint8_t j = 20; j; j--)
        for (uint8_t k = 255; k; k--)
            __asm__ ("nop");
}

static void charge_delay()
{
    // About 8ms, plenty of time to charge the RC circuit in a button
    for (uint8_t j = 10; j; j--)
        for (uint8_t k = 255; k; k--)
            __asm__ ("nop");
}

static void input_pullup_delay()
{
    // Small delay for charging weak pull-up
    for (uint8_t k = 5; k; k--)
            __asm__ ("nop");
}

static inline void gpio_mode_push_pull(gpio_t port, uint8_t pin)
{
    port->ddr |= pin;
    port->cr1 |= pin;
    port->cr2 &= ~pin;
}

static inline void gpio_mode_float(gpio_t port, uint8_t pin)
{
    port->ddr &= ~pin;
    port->cr1 &= ~pin;
    port->cr2 &= ~pin;
}

static inline void gpio_mode_pull_up(gpio_t port, uint8_t pin)
{
    port->ddr &= ~pin;
    port->cr1 |= pin;
    port->cr2 &= ~pin;    
}

static inline void gpio_output(gpio_t port, uint8_t pin, bool value)
{
    if (value)
        port->odr |= pin;
    else
        port->odr &= ~pin;
}

static inline bool gpio_input(gpio_t port, uint8_t pin)
{
    return (port->idr & pin) != 0;
}

void gpio_init()
{
    gpio_mode_push_pull(PIN_LED1);
    gpio_mode_push_pull(PIN_LED2);
    gpio_mode_push_pull(PIN_LED3);
    gpio_mode_push_pull(PIN_LED4);
    gpio_mode_push_pull(PIN_SEL_01);
    gpio_mode_push_pull(PIN_SEL_23);
    gpio_mode_push_pull(PIN_SEL_MSB);
    gpio_mode_pull_up(PIN_BUTTON);
    gpio_mode_float(PIN_TUCOPLEX_S);
    gpio_mode_float(PIN_TUCOPLEX_R);
    gpio_mode_float(PIN_TUCOPLEX_T);
}

void led_set_channel(uint8_t ch)
{
    gpio_output(PIN_LED1, ch != 0);
    gpio_output(PIN_LED2, ch != 1);
    gpio_output(PIN_LED3, ch != 2);
    gpio_output(PIN_LED4, ch != 3);
}

void usb_set_channel(uint8_t ch)
{
    gpio_output(PIN_SEL_01, ch & 1);
    gpio_output(PIN_SEL_23, ch & 1);
    gpio_output(PIN_SEL_MSB, ch >> 1);
}

void set_channel(uint8_t ch)
{
    led_set_channel(ch);
    usb_set_channel(ch);
}

void scan_one_led(gpio_t cathode_port, uint8_t cathode_pin,
                  gpio_t anode_port, uint8_t anode_pin)
{
    gpio_output(cathode_port, cathode_pin, false);
    gpio_output(anode_port, anode_pin, true);
    gpio_mode_push_pull(cathode_port, cathode_pin);
    gpio_mode_push_pull(anode_port, anode_pin);

    led_delay();

    gpio_mode_float(cathode_port, cathode_pin);
    gpio_mode_float(anode_port, anode_pin);
}

void scan_led_for_channel(uint8_t ch)
{
    switch (ch) {
        case 0:
            scan_one_led(PIN_TUCOPLEX_R, PIN_TUCOPLEX_S);
            break;
        case 1:
            scan_one_led(PIN_TUCOPLEX_S, PIN_TUCOPLEX_R);
            break;
        case 2:
            scan_one_led(PIN_TUCOPLEX_S, PIN_TUCOPLEX_T);
            break;
        case 3:
            scan_one_led(PIN_TUCOPLEX_T, PIN_TUCOPLEX_S);
            break;
    }
}

uint8_t scan_button_path(gpio_t cathode_port, uint8_t cathode_pin,
                         gpio_t anode_port, uint8_t anode_pin)
{
    uint8_t result = 0;

    gpio_output(cathode_port, cathode_pin, false);
    gpio_output(anode_port, anode_pin, true);
    gpio_mode_push_pull(cathode_port, cathode_pin);

    gpio_mode_pull_up(anode_port, anode_pin);
    input_pullup_delay();

    // Sample state without any pre-charge time
    if (!gpio_input(anode_port, anode_pin)) {

        // Seems like at least one of the two buttons on this
        // path are pressed; try charging the capacitor and sampling
        // again, to tell the two states apart.

        // Charge capacitor and sample again
        gpio_mode_push_pull(anode_port, anode_pin);
        charge_delay();

        gpio_mode_pull_up(anode_port, anode_pin);
        input_pullup_delay();

        // If shorted button, this is still low and we return 1.
        // If it's the one with an RC circuit, it's charged by now and
        // we return 2.
        result = 1 + gpio_input(anode_port, anode_pin);
    }

    gpio_mode_float(cathode_port, cathode_pin);
    gpio_mode_float(anode_port, anode_pin);

    return result;
}

uint8_t scan_buttons_once()
{
    uint8_t result;

    result = scan_button_path(PIN_TUCOPLEX_T, PIN_TUCOPLEX_R);
    if (result)
        return result;

    result = scan_button_path(PIN_TUCOPLEX_R, PIN_TUCOPLEX_T);
    if (result)
        return result + 2;

    if (!gpio_input(PIN_BUTTON)) {
        return 5;
    }

    return 0;
}

void main()
{
    uint8_t channel = 0;
    uint8_t button_now = 0;
    uint8_t button_debounce = 0;
    uint8_t button_prev = 0;

    gpio_init();

    while (1) {
        set_channel(channel);
        scan_led_for_channel(channel);
        button_now = scan_buttons_once();

        if (button_now == button_debounce) {

            if (button_now && button_now <= 4) {
                // Set channel
                channel = button_now - 1;
            }
            else if (button_now == 5 && button_prev == 0) {
                // Next channel
                channel = (channel + 1) & 3;
            }

            button_prev = button_now;
        }
        button_debounce = button_now;
    }
}
