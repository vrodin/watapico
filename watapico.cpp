#include <string.h>

#include <pico/time.h>

#include <hardware/gpio.h>
#include <hardware/clocks.h>
#include <hardware/structs/vreg_and_chip_reset.h>
#include <hardware/structs/rosc.h>

#include "./roms/roms.h"

// Address Bus (A0 - A16)
constexpr uint32_t ADDR_MASK = 0x1FFFF;  // Bits 0-16

// Data Bus (D0 - D7)
constexpr uint32_t DATA_MASK = (0xFF << 17);

// /RD
constexpr uint8_t RD_PIN = 29;
constexpr uint32_t READ_MASK = (1u << RD_PIN);

constexpr uint8_t PWR_ON_PIN = 25;
constexpr uint32_t PWR_ON_MASK = (1u << PWR_ON_PIN);

uint8_t __aligned(4) rom[65536];
uint16_t ROM_MASK = 0xFFFF;

[[noreturn]] void __time_critical_func(handle_bus)() {
    while (true) {
        while (gpio_get_all() & READ_MASK);
        const uint32_t data = rom[gpio_get_all() & ROM_MASK] << 17 | PWR_ON_MASK;

        gpio_set_dir_out_masked(DATA_MASK);
        gpio_put_all(data);
        gpio_set_dir_in_masked(DATA_MASK);
    }
}

inline static uint8_t random_byte() {
    uint32_t random = 0;
    for (int k = 0; k < 8; k++) {
        random = random << 1 | rosc_hw->randombit;
    }
    return (uint8_t) random;
}

int main() {
    // Set the system clock speed.
    hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
    sleep_us(35);
    set_sys_clock_hz(400 * MHZ, true); // 100x of Watara Supervision clock speed

    // Initialize all input pins at once
    gpio_init_mask(ADDR_MASK | DATA_MASK | READ_MASK | PWR_ON_MASK);
    gpio_set_dir_in_masked(ADDR_MASK | DATA_MASK | READ_MASK);

    const RomEntry *rom_entry = get_rom_by_index(random_byte());
    memcpy(rom, rom_entry->data, rom_entry->size);
    ROM_MASK = rom_entry->mask;

    gpio_set_dir(PWR_ON_PIN, GPIO_OUT);
    gpio_put(PWR_ON_PIN, 0);
    sleep_us(5000);
    gpio_put(PWR_ON_PIN, 1);

    handle_bus();
    return 0;
}
