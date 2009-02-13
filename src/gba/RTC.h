#ifndef RTC_H
#define RTC_H

#include "../common/Util.h"

namespace Cartridge
{

u16 rtcRead(u32 address);
bool rtcWrite(u32 address, u16 value);
void rtcEnable(bool);
bool rtcIsEnabled();
void rtcReset();

void rtcReadGame(gzFile gzFile);
void rtcSaveGame(gzFile gzFile);

} // namespace Cartridge

#endif // RTC_H
