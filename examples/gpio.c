/*
 * MIT License
 *
 * Copyright (c) 2024 hanyazou
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <mcu_peripheral/mcu_peripheral.h>

void usage(void)
{
    printf("Usage:\n");
    printf("    gpio [gpio number]          (input)\n");
    printf("    gpio [gpio number] [value]  (output)\n");
}

int main(int argc, char *argv[])
{
    mcupr_result_t result;
    mcupr_gpio_chip_t *gpio_chip;
    mcupr_gpio_device_t gpio_dev;
    mcupr_gpio_chip_params_t gpio_chip_params;
    int pin;
    int value;
    int output;
    char *tail;

    mcupr_initialize();

    mcupr_gpio_init_params(&gpio_chip_params);
    result = mcupr_gpio_chip_create(&gpio_chip, &gpio_chip_params);
    if (result != MCUPR_RES_OK) {
        exit(1);
    }

    if (argc < 2 || 3 < argc) {
        usage();
        exit(1);
    }

    pin = strtol(argv[1], &tail, 0);
    if (*tail != '\0') {
        fprintf(stderr, "Invalid GPIO %s\n", argv[1]);
        exit(1);
    }

    output = 0;
    if (argc == 3) {
        value = strtol(argv[2], &tail, 0);
        if (*tail != '\0') {
            fprintf(stderr, "Invalid value %s\n", argv[1]);
            exit(1);
        }
        output = 1;
    }

    result = mcupr_gpio_open(gpio_chip, &gpio_dev, pin,
                             output ? MCUPR_GPIO_MODE_OUTPUT : MCUPR_GPIO_MODE_INPUT);
    if (result != MCUPR_RES_OK) {
        exit(1);
    }

    if (output) {
        mcupr_gpio_write(gpio_chip, gpio_dev, value);
    } else {
        printf("%d\n", mcupr_gpio_read(gpio_chip, gpio_dev));
    }

    mcupr_gpio_close(gpio_chip, gpio_dev);
    mcupr_gpio_chip_release(gpio_chip);

    exit(0);
}
