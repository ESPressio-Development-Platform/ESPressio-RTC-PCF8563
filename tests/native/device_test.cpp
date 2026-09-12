#include <array>
#include <cassert>
#include <cstdint>
#include "ESPressio_RTC_PCF8563.hpp"

using namespace ESPressio;
struct Bus {
    std::array<std::uint8_t,256> r{};
    bool Read(std::uint8_t, std::uint8_t reg, std::uint8_t* d, std::size_t n) noexcept {
        for(std::size_t i=0;i<n;++i) { d[i]=r[reg+i]; }
        return true;
    }
    bool Write(std::uint8_t, std::uint8_t reg, const std::uint8_t* d, std::size_t n) noexcept {
        for(std::size_t i=0;i<n;++i) { r[reg+i]=d[i]; }
        return true;
    }
};
using Device = RTC::PCF8563::Device<Bus,2000>;
static_assert(RTC::IsRealTimeClockV<Device>);
static_assert(Platform::Clock::IsClockSourceV<RTC::PCF8563::Clock32k>);
static_assert(Platform::Clock::FrequencyHz<RTC::PCF8563::Clock32Hz> == 32U);
static_assert(Platform::Clock::FrequencyHz<RTC::PCF8563::Clock1024Hz> == 1024U);
int main(){
    Bus b; Device rtc(b);
    RTC::DateTime in{2026,9,12,16,45,30,6};
    assert(rtc.Write(in)==RTC::Result::Ok);
    RTC::Reading out{}; assert(rtc.Read(out)==RTC::Result::Ok);
    assert(out.Value.Year==2026 && out.Validity==RTC::TimeValidity::Valid);
    b.r[0x02] |= 0x80; assert(rtc.Read(out)==RTC::Result::Ok); assert(out.Validity==RTC::TimeValidity::VoltageLow);
    assert(rtc.ClearVoltageLowFlag()==RTC::Result::Ok); assert((b.r[0x02]&0x80)==0);
    assert(rtc.ConfigureClockOutput(RTC::PCF8563::ClockOutputFrequency::Hz32768)==RTC::Result::Ok);
    assert(b.r[0x0D]==0x80);
}
