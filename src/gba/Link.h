#ifndef GBALINK_H

#define LINK_PARENTLOST 0x80
#define UNSUPPORTED -1
#define MULTIPLAYER 0
#define NORMAL8 1
#define NORMAL32 2
#define UART 3
#define JOYBUS 4
#define GP 5
#define RFU_INIT 0
#define RFU_COMM 1
#define RFU_SEND 2
#define RFU_RECV 3

void linkUpdateSIOCNT(u16);
void StartJOYLink(u16);
void linkUpdateRCNT(u16);
void LinkSSend(u16);
void LinkUpdate(int);

extern bool linkenable;

#endif // GBALINK_H
