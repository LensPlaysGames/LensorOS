#include "io.h"
#include "memory/paging.h"
#include "memory/virtual_memory_manager.h"
#include <e1000.h>

#include <pci.h>

#include <format>

#define REG_CTRL          0x0000
#define REG_STATUS        0x0008
#define REG_EEPROM        0x0014
#define REG_CTRL_EXT      0x0018
#define REG_IMASK         0x00d0
#define REG_RCTRL         0x0100
#define REG_RXDESCLO      0x2800
#define REG_RXDESCHI      0x2804
#define REG_RXDESCLEN     0x2808
#define REG_RXDESCHEAD    0x2810
#define REG_RXDESCTAIL    0x2818

/// RX Delay Timer Register
#define REG_RDTR    0x2820
/// RX Descriptor Control
#define REG_RXDCTL  0x3828
/// RX Interrupt Absolute Delay Timer
#define REG_RADV    0x282c
/// RX Small Packet Detect Interrupt
#define REG_RSRPD   0x2c00

/// Transmit Inter Packet Gap
#define REG_TIPG  0x0410
/// SLU == Set Link Up
#define ECTRL_SLU 0x40

/// Reciever Enable
#define RCTL_EN              (1 << 1)
/// Store Bad Packets
#define RCTL_SBP             (1 << 2)
/// Unicast Promiscuous Enabled
#define RCTL_UPE             (1 << 3)
/// Multicast Promiscuous Enabled
#define RCTL_MPE             (1 << 4)
/// Long Packet Reception Enable
#define RCTL_LPE             (1 << 5)
/// Set Loop Back Mode to None
#define RCTL_LBM_NONE        (0 << 6)
/// Set Loop Back Mode to Physical
#define RCTL_LBM_PHY         (3 << 6)

/// FIXME: Possibly this should be RTCL
/// Free Buffer Threshold is 1/2 of RDLEN
#define RCTL_RDMTS_HALF      (0 << 8)
/// Free Buffer Threshold is 1/4 of RDLEN
#define RCTL_RDMTS_QUARTER   (1 << 8)
/// Free Buffer Threshold is 1/8 of RDLEN
#define RCTL_RDMTS_EIGHTH    (2 << 8)

/// Multicast offset - bits 47:36
#define RCTL_MO_36           (0 << 12)
/// Multicast offset - bits 46:35
#define RCTL_MO_35           (1 << 12)
/// Multicast offset - bits 45:34
#define RCTL_MO_34           (2 << 12)
/// Multicast offset - bits 43:32
#define RCTL_MO_32           (3 << 12)

/// Broadcast Accept Mode
#define RCTL_BAM             (1 << 15)
/// VLAN Filter Enable
#define RCTL_VFE             (1 << 18)
/// Canonical Form Indicator Enable
#define RCTL_CFIEN           (1 << 19)
/// Canonical Form Indicator Bit Value
#define RCTL_CFI             (1 << 20)
/// Discard Pause Frames
#define RCTL_DPF             (1 << 22)
/// Pass MAC Control Frames
#define RCTL_PMCF            (1 << 23)
/// Strip Ethernet CRC
#define RCTL_SECRC           (1 << 26)

// Buffer Sizes
#define RCTL_BSIZE_256    (3 << 16)
#define RCTL_BSIZE_512    (2 << 16)
#define RCTL_BSIZE_1024   (1 << 16)
#define RCTL_BSIZE_2048   (0 << 16)
#define RCTL_BSIZE_4096   ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192   ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384  ((1 << 16) | (1 << 25))

/// End Of Packet
#define CMD_EOP   (1 << 0)
/// Insert FCS
#define CMD_IFCS  (1 << 1)
/// Insert Checksum
#define CMD_IC    (1 << 2)
/// Report Status
#define CMD_RS    (1 << 3)
/// Report Packet Sent
#define CMD_RPS   (1 << 4)
/// VLAN Packet Enable
#define CMD_VLE   (1 << 6)
/// Interrupt Delay Enable
#define CMD_IDE   (1 << 7)

/// Transmit Enable
#define TCTL_EN          (1 << 1)
/// Pad Short Packets
#define TCTL_PSP         (1 << 3)
/// Collision Threshold
#define TCTL_CT_SHIFT    4
/// Collision Distance
#define TCTL_COLD_SHIFT  12
/// Software XOFF transmission
#define TCTL_SWXOFF      (1 << 22)
/// Re-transmit on Late Collision
#define TCTL_RTLC        (1 << 24)

/// Descriptor Done
#define TSTA_EN          (1 << 0)
/// Excess Collisions
#define TSTA_EC          (1 << 1)
/// Late Collision
#define TSTA_LC          (1 << 2)
/// Transmit Underrun
#define LSTA_TU          (1 << 3)

#define E1000_NUM_RX_DESC 32
#define E1000_NUM_TX_DESC 8

struct E1000RXDesc {
    volatile u64 Address;
    volatile u16 Length;
    volatile u16 Checksum;
    volatile u8 Status;
    volatile u8 Errors;
    volatile u16 Special;
} __attribute__((packed));

struct E1000TXDesc {
    volatile u64 Address;
    volatile u16 Length;
    volatile u8 CSO;
    volatile u8 Command;
    volatile u8 Status;
    volatile u8 CSS;
    volatile u16 Special;
} __attribute__((packed));

E1000 gE1000 = {};

void E1000::write_command(u16 address, u32 value) {
    if (BARType == PCI::BarType::Memory)
        volatile_write((volatile u32*)(BARMemoryAddress + address), value);
    else {
        out32(BARIOAddress, address);
        io_wait();
        out32(BARIOAddress + 4, value);
    }
}
u32 E1000::read_command(u16 address) {
    if (BARType == PCI::BarType::Memory)
        return volatile_read<u32>((volatile u32*)(BARMemoryAddress + address));
    else {
        out32(BARIOAddress, address);
        io_wait();
        return in32(BARIOAddress + 4);
    }
}

bool E1000::detect_eeprom() {
    write_command(REG_EEPROM, 1);
    io_wait();
    // Read value 1000 times in a row, in case it is just taking a
    // while to be set.
    for (usz i = 0; i < 1000; ++i) {
        if (read_command(REG_EEPROM) & 0x10) return true;
    }
    return false;
}

u16 E1000::read_eeprom(u8 address) {
    u32 out = 0;
    u32 calculatedAddress = 0;
    u32 successMask = 0;
    if (EEPROMExists) {
        calculatedAddress = u32(address) << 8;
        successMask = 1 << 4;
    } else {
        calculatedAddress = u32(address) << 2;
        successMask = 1 << 1;
    }
    // Write address to EEPROM register.
    write_command(REG_EEPROM, 1 | calculatedAddress);
    // TODO: Maybe put a max spin so we don't hang forever in case of
    // something going wrong!
    // Read from EEPROM register until specific bit is set.
    do out = read_command(REG_EEPROM);
    while (!(out & successMask));

    return (out >> 16) & 0xffff;
}

void E1000::get_mac_address() {
    if (EEPROMExists) {
        u16 value = 0;
        // Byte order of each two-byte pair is flipped.
        value = read_eeprom(0);
        MACAddress[0] = value & 0xff;
        MACAddress[1] = value >> 8;
        value = read_eeprom(1);
        MACAddress[2] = value & 0xff;
        MACAddress[3] = value >> 8;
        value = read_eeprom(2);
        MACAddress[4] = value & 0xff;
        MACAddress[5] = value >> 8;
    } else {
        // TODO: What if BARType is IO?
        // TODO: Figure out what 0x5400 is
        u8* base = (u8*)(BARMemoryAddress + 0x5400);
        for (uint i = 0; i < 6; ++i, ++base)
            MACAddress[i] = *base;
        if (!MACAddress[0] && !MACAddress[1] && !MACAddress[2] && !MACAddress[3])
            std::print("[E1000]:\033[31mERROR:\033[m First four bytes of MACAddress are zero!\n");
    }
}

E1000::E1000(PCI::PCIHeader0* header) : PCIHeader(header) {
    BARType = PCI::get_bar_type(PCIHeader->BAR0);

    if (BARType == PCI::BarType::Memory) {
        BARMemoryAddress = PCIHeader->BAR0 & ~usz(3);
        Memory::map((void*)BARMemoryAddress, (void*)BARMemoryAddress,
                    (u64)Memory::PageTableFlag::Present
                    | (u64)Memory::PageTableFlag::ReadWrite
                    );
        std::print("[E1000]: BAR0 is memory! addr={}\n", (void*)BARMemoryAddress);
    }
    else {
        BARIOAddress = PCIHeader->BAR0 & ~usz(1);
        Memory::map((void*)BARIOAddress, (void*)BARIOAddress,
                    (u64)Memory::PageTableFlag::Present
                    | (u64)Memory::PageTableFlag::ReadWrite
                    );
        std::print("[E1000]: BAR0 is IO! addr={}\n", (void*)BARIOAddress);
    }



    EEPROMExists = detect_eeprom();
    if (EEPROMExists)
        std::print("[E1000]: EEPROM EXISTS!!\n");
    else std::print("[E1000]: EEPROM DOES NOT EXIST!!\n");

    get_mac_address();
    std::print("[E1000]:MACAddress: ");
    for (u8 c : MACAddress)
        std::print("{:x}-", c);
    std::print("\n");
}
