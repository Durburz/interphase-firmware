
//#define COMPILE_RIGHT
#define COMPILE_LEFT

#include "interphase.h"
#include "nrf_drv_config.h"
#include "nrf_gzll.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_rtc.h"

// Define payload length
#define TX_PAYLOAD_LENGTH ROWS ///< 5 byte payload length when transmitting

// Data and acknowledgement payloads
static uint8_t data_payload[TX_PAYLOAD_LENGTH];                ///< Payload to send to Host.
static uint8_t ack_payload[NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH]; ///< Placeholder for received ACK payloads from Host.

static uint8_t keys[ROWS];

// Setup switch pins with pullups
static void gpio_config(void)
{
    nrf_gpio_cfg_sense_input(C01, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
    nrf_gpio_cfg_sense_input(C02, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
    nrf_gpio_cfg_sense_input(C03, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
    nrf_gpio_cfg_sense_input(C04, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
    nrf_gpio_cfg_sense_input(C05, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
    nrf_gpio_cfg_sense_input(C06, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
    nrf_gpio_cfg_sense_input(C07, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);

    nrf_gpio_cfg_output(R01);
    nrf_gpio_cfg_output(R02);
    nrf_gpio_cfg_output(R03);
    nrf_gpio_cfg_output(R04);
    nrf_gpio_cfg_output(R05);
}

// Return the key states of one row
static uint8_t read_row(uint32_t row)
{
    uint8_t buff = 0;
    nrf_gpio_pin_set(row);
    buff = buff << 1 | nrf_gpio_pin_read(C01);
    buff = buff << 1 | nrf_gpio_pin_read(C02);
    buff = buff << 1 | nrf_gpio_pin_read(C03);
    buff = buff << 1 | nrf_gpio_pin_read(C04);
    buff = buff << 1 | nrf_gpio_pin_read(C05);
    buff = buff << 1 | nrf_gpio_pin_read(C06);
    buff = buff << 1 | nrf_gpio_pin_read(C07);
    nrf_gpio_pin_clear(row);
    return buff;
}

// Return the key states, masked with valid key pins
static void read_keys(void)
{
    keys[0] = read_row(R01);
    keys[1] = read_row(R02);
    keys[2] = read_row(R03);
    keys[3] = read_row(R04);
    keys[4] = read_row(R05);
    return;
}

// Assemble packet and send to receiver
static void send_data(void)
{
    nrf_gzll_add_packet_to_tx_fifo(PIPE_NUMBER, data_payload, TX_PAYLOAD_LENGTH);
}

int main()
{
    // Initialize Gazell
    nrf_gzll_init(NRF_GZLL_MODE_DEVICE);

    // Attempt sending every packet up to 100 times
    nrf_gzll_set_max_tx_attempts(100);

    // Addressing
    nrf_gzll_set_base_address_0(0x01020304);
    nrf_gzll_set_base_address_1(0x05060708);

    // Enable Gazell to start sending over the air
    nrf_gzll_enable();

    // Configure all keys as inputs with pullups
    gpio_config();

    // Main loop, constantly sleep, waiting for RTC and gpio IRQs
    while(1)
    {
        read_keys();
        for(int i=0; i < ROWS; i++)
        {
            data_payload[i] = keys[i];
        }
        send_data();
    }
}

/*****************************************************************************/
/** Gazell callback function definitions  */
/*****************************************************************************/

void  nrf_gzll_device_tx_success(uint32_t pipe, nrf_gzll_device_tx_info_t tx_info)
{
    uint32_t ack_payload_length = NRF_GZLL_CONST_MAX_PAYLOAD_LENGTH;

    if (tx_info.payload_received_in_ack)
    {
        // Pop packet and write first byte of the payload to the GPIO port.
        nrf_gzll_fetch_packet_from_rx_fifo(pipe, ack_payload, &ack_payload_length);
    }
}

// no action is taken when a packet fails to send, this might need to change
void nrf_gzll_device_tx_failed(uint32_t pipe, nrf_gzll_device_tx_info_t tx_info)
{

}

// Callbacks not needed
void nrf_gzll_host_rx_data_ready(uint32_t pipe, nrf_gzll_host_rx_info_t rx_info)
{}
void nrf_gzll_disabled()
{}
