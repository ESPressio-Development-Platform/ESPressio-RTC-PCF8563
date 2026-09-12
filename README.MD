# ESPressio-RTC-PCF8563

Platform-independent PCF8563 implementation for `ESPressio-RTC`.

Implemented facilities include calendar read/write, explicit user-selected century policy, voltage-low validity reporting/clearing, STOP control, programmable CLKOUT (32.768 kHz / 1.024 kHz / 32 Hz / 1 Hz), and fixed-frequency ISR-driven `ESPressio-Platform::Clock` aliases.

No Arduino, ESP-IDF, or FreeRTOS API is used by the driver.

## Century and leap-year note

The PCF8563 century bit is application-assignable and toggles when the two-digit year rolls from 99 to 00. The driver therefore makes the base century a template policy. The hardware leap-year rule is purely two-digit divisibility by four (including year `00`), so applications spanning a Gregorian non-leap century such as 2100 must account for that hardware limitation.

`Clock1Hz`, `Clock32Hz`, `Clock1024Hz`, and `Clock32k` correspond directly to the chip's selectable CLKOUT frequencies.
