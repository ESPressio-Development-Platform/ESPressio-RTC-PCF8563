#pragma once

#include <cstdint>

#include <ESPressio_RTC.hpp>

namespace ESPressio::RTC::PCF8563 {

inline constexpr std::uint8_t I2CAddress = 0x51U;
struct Origin final : ESPressio::Platform::Backend {};

enum class ClockOutputFrequency : std::uint8_t {
    Hz32768 = 0,
    Hz1024 = 1,
    Hz32 = 2,
    Hz1 = 3
};

/**
 * PCF8563 civil-time/calendar driver.
 *
 * TBaseCentury defines the century represented when the device C bit is zero;
 * the C bit set represents TBaseCentury + 100. This makes the device's
 * user-assignable century bit an explicit application policy.
 */
template <typename TBus, std::uint16_t TBaseCentury = 2000U>
class Device final
    : public RTC::DeviceProviderDeclaration<
          Origin,
          ESPressio::Platform::CapabilitySet<
              RTC::Capability::ClockOutput>> {
    static_assert(TBaseCentury % 100U == 0U,
                  "PCF8563 base century must be a multiple of 100");
    static_assert(TBaseCentury >= 1900U && TBaseCentury + 199U <= 2199U,
                  "PCF8563 base century must fit ESPressio-RTC DateTime range");
public:
    explicit constexpr Device(TBus& bus) noexcept : registers_(bus) {}

    Result Read(Reading& reading) noexcept {
        std::uint8_t data[7]{};
        auto result = registers_.Read(0x02U, data, sizeof(data));
        if (result != Result::Ok) return result;

        DateTime value{};
        value.Second = FromBcd(static_cast<std::uint8_t>(data[0] & 0x7FU));
        value.Minute = FromBcd(static_cast<std::uint8_t>(data[1] & 0x7FU));
        value.Hour = FromBcd(static_cast<std::uint8_t>(data[2] & 0x3FU));
        value.Day = FromBcd(static_cast<std::uint8_t>(data[3] & 0x3FU));
        value.Weekday = static_cast<std::uint8_t>(data[4] & 0x07U);
        value.Month = FromBcd(static_cast<std::uint8_t>(data[5] & 0x1FU));
        value.Year = static_cast<std::uint16_t>(
            TBaseCentury + ((data[5] & 0x80U) != 0U ? 100U : 0U) + FromBcd(data[6]));
        if (!RTC::IsValid(value)) return Result::InvalidDateTime;

        reading.Value = value;
        reading.Validity = (data[0] & 0x80U) != 0U
                               ? TimeValidity::VoltageLow
                               : TimeValidity::Valid;
        return Result::Ok;
    }

    Result Write(const DateTime& value) noexcept {
        if (!RTC::IsValid(value) || value.Year < TBaseCentury ||
            value.Year > static_cast<std::uint16_t>(TBaseCentury + 199U))
            return Result::InvalidDateTime;

        const bool nextCentury = value.Year >= static_cast<std::uint16_t>(TBaseCentury + 100U);
        const auto year = static_cast<std::uint8_t>(value.Year % 100U);
        std::uint8_t data[7]{
            ToBcd(value.Second),
            ToBcd(value.Minute),
            ToBcd(value.Hour),
            ToBcd(value.Day),
            static_cast<std::uint8_t>(value.Weekday & 0x07U),
            static_cast<std::uint8_t>(ToBcd(value.Month) | (nextCentury ? 0x80U : 0U)),
            ToBcd(year)
        };
        return registers_.Write(0x02U, data, sizeof(data));
    }

    /** Clears the latched voltage-low flag while preserving the current seconds value. */
    Result ClearVoltageLowFlag() noexcept {
        std::uint8_t seconds{};
        auto result = registers_.ReadByte(0x02U, seconds);
        if (result != Result::Ok) return result;
        seconds = static_cast<std::uint8_t>(seconds & 0x7FU);
        return registers_.WriteByte(0x02U, seconds);
    }

    Result ConfigureClockOutput(ClockOutputFrequency frequency, bool enabled = true) noexcept {
        const auto value = static_cast<std::uint8_t>(
            (enabled ? 0x80U : 0x00U) | static_cast<std::uint8_t>(frequency));
        return registers_.WriteByte(0x0DU, value);
    }

    Result SetStopped(bool stopped) noexcept {
        // Reserved N bits in Control_status_1 must always be written as zero.
        return registers_.WriteByte(0x00U, stopped ? 0x20U : 0x00U);
    }

private:
    RTC::RegisterDevice<TBus, I2CAddress> registers_;
};

/** ISR-counted Platform Clock aliases for the selectable PCF8563 CLKOUT rates.
 * The physical CLKOUT configuration must match the alias used by the composition.
 */
using Clock1Hz = RTC::ExternalTickClock<Origin, 1U>;
using Clock32Hz = RTC::ExternalTickClock<Origin, 32U>;
using Clock1024Hz = RTC::ExternalTickClock<Origin, 1024U>;
using Clock32k = RTC::ExternalTickClock<
    Origin,
    32768U,
    32U,
    ESPressio::Platform::CapabilitySet<
        ESPressio::Platform::Capability::HighResolutionClock>>;

} // namespace ESPressio::RTC::PCF8563
