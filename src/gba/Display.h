#ifndef DISPLAY_H
#define DISPLAY_H

#include "../Util.h"

namespace Display
{

bool init();
void initColorMap(int redShift, int greenShift, int blueShift);
void uninit();

void readState(gzFile gzFile);
void saveState(gzFile gzFile);

void drawLine(int line, u32* src);
void drawScreen();
void clear();

} // namespace Display

#endif // DISPLAY_H
