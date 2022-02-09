#ifndef LENSOR_OS_TSS_H
#define LENSOR_OS_TSS_H

struct TSSEntry {
    u32 Reserved0;
    u32 l_RSP0;
    u32 h_RSP0;
    u32 l_RSP1;
    u32 h_RSP1;
    u32 l_RSP2;
    u32 h_RSP2;
    u64 Reserved1;
    u32 l_IST1;
    u32 h_IST1;
    u32 l_IST2;
    u32 h_IST2;
    u32 l_IST3;
    u32 h_IST3;
    u32 l_IST4;
    u32 h_IST4;
    u32 l_IST5;
    u32 h_IST5;
    u32 l_IST6;
    u32 h_IST6;
    u32 l_IST7;
    u32 h_IST7;
    u64 Reserved2;
    u16 Reserved3;
    u16 IOMapBaseAddress;
} __attribute__((packed));

#endif
