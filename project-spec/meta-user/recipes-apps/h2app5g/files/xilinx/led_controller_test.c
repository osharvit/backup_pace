// url=https://logictronix.com/wp-content/uploads/2019/07/Petalinux_LED_Controller_Tutorial_LogicTronix.pdf
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/mman.h>

// Define maximum LED value (2^8)-1 = 255
#define LED_LIMIT 255
// Define delay length
#define DELAY 10000000
/* Define the base memory address of the led_controller IP core */
static volatile uint32_t *leds;
static const uint32_t leds_addr = 0x43C00000;
void map_gpios(void)
{
    int fd;
    if ((fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0)
        if (fd < 0) {
            perror("/dev/mem");
            exit(-1);
        }
    if ((leds = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd,
                     leds_addr))
        == MAP_FAILED) {
        perror("leds");
        exit(-1);
    }
}
static void run_leds_mode(void)
{
    /* unsigned 32-bit variables for storing current LED value */
    uint32_t led_val = 0;
    int i=0;

    printf("led_controller IP test begin.\r\n");
    printf("--------------------------------------------\r\n\n");
    /* Loop forever */
    while(1){
        while(led_val<=LED_LIMIT){
            /* Print value to terminal */
            printf("LED value: %d\r\n", led_val);

            /* Write value to led_controller IP core */
            *leds = led_val;
            /* increment LED value */
            led_val++;
            /* run a simple delay to allow changes on LEDs to be
               visible */
            for(i=0;i<DELAY;i++);
        }
        /* Reset LED value to zero */
        led_val = 0;
    }
}
/* main function */
int main(void)
{
    map_gpios();
    run_leds_mode();
    return 0;
}
