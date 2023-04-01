#include <e1000.h>

#include <io.h>
#include <memory/common.h>
#include <memory/paging.h>
#include <memory/virtual_memory_manager.h>
#include <pci.h>
#include <stdint.h>

#include <format>

/* TODO: Move all this into documentation of some sort, somewhere else.
 *
 * At power up, the Ethernet controller is not automatically configured
 * by the hardware for normal operation. Software initialization is
 * required before normal operation can continue. In general, the
 * Ethernet controller is considered non-functional until the software
 * driver successfully loads and sets up the hardware. However, Auto-
 * Negotiation can start at power up or upon receipt of an assertion of
 * PCI reset if configured to do so by the EEPROM.
 *
 * General Configuration:
 * Several values in the Device Control Register (CTRL) need to be set
 * upon power up or after an Ethernet controller reset for normal
 * operation.
 * - Speed and duplex are determined via Auto-Negotiation by the PHY,
 *   Auto-Negotiation by the MAC for internal SerDes1 mode, or forced
 *   by software if the link is forced. In internal PHY mode, the
 *   Ethernet controller can be configured automatically by hardware or
 *   forced by software to the same configuration as the PHY.
 * - In internal PHY mode, the Auto-Speed Detection Enable (CTRL.ASDE)
 *   bit, when set to 1b, detects the resolved speed and duplex of the
 *   link and self-configure the MAC appropriately. This bit should be
 *   set in conjunction with the Set Link Up (CTRL.SLU) bit.
 * - The MAC can also be forced to a specific Speed/Duplex combination.
 *   This is accomplished by setting the Set Link Up (CTRL.SLU), Force
 *   Speed (CTRL. FRCSPD) and Force Duplex (CTRL.FRCDPLX) bits. Once
 *   speed and duplex are determined (either via Auto-Negotiation or
 *   forced by software), speed is forced by setting the appropriate
 *   Speed Selection (CTRL.SPEED) bits and duplex is forced by updating
 *   the Full Duplex (CTRL.FD) bit.
 * - For the 82541xx and 82547GI/EI, configure the LED behavior through
 *   LEDCTRL.
 * - Link Reset (CTRL.LRST) should be set to 0b (normal). The
 *   Ethernet controller defaults to LRST = 1b which disables Auto-
 *   Negotiation. A transition to 0b initiates the Auto-Negotiation
 *   function. LRST can be defined in the EEPROM. This bit is only valid
 *   in internal SerDes mode and has no effect in internal PHY mode. 1.
 *   The 82540EP/EM, 82541xx, and 82547GI/EI do not support any SerDes
 *   functionality.
 * - PHY Reset (CTRL.PHY_RST) should be set to 0b. Setting this bit to 1b
 *   resets the PHY without accessing the PHY registers. This bit is
 *   ignored in internal SerDes mode.
 * - CTRL.ILOS should be set to 0b (not applicable to the 82541xx and
 *   82547GI/EI).
 * - If Flow Control is desired, program the FCAH, FCAL, FCT and FCTTV
 *   registers. If not, they should be written with 0b. To enable XON
 *   frame transmission, the XON Enable (FCTRL.XONE) bit must be set.
 *   Advertising Flow Control capabilities during the AutoNegotiation
 *   process is dependent on whether the Ethernet controller is operating
 *   in internal SerDes or internal PHY mode. In internal SerDes mode,
 *   the TXCW register must be set up prior to starting the Auto-
 *   Negotiation process. In internal PHY mode, the appropriate PHY
 *   registers must be set up properly to advertise desired capabilities
 *   prior to starting or re-starting the Auto-Negotiation process. The
 *   Receive Flow Control Enable (CTRL.RFCE) and Transmit Flow Control
 *   Enable (CTRL.TFCE) bits need to be explicitly set by software in
 *   internal PHY mode (because Auto-Negotiation is managed by PHY rather
 *   than the MAC), or when a fiber connection is desired but link was
 *   forced rather than Auto-Negotiated.
 * - If VLANs are not used, software should clear VLAN Mode Enable
 *   (CTRL.VME) bit. In this instance, there is no need then to
 *   initialize the VLAN Filter Table Array (VFTA). If VLANs are
 *   desired, the VFTA should be both initialized and loaded with the
 *   desired information.
 * - For the 82541xx and 82547GI/EI, clear all statistical counters.
 *
 * Receive Initialization:
 * - Program the Receive Address Register(s) (RAL/RAH) with the desired
 *   Ethernet addresses. RAL[0]/RAH[0] should always be used to store the
 *   Individual Ethernet MAC address of the Ethernet controller. This can
 *   come from the EEPROM or from any other means (for example, on some
 *   machines, this comes from the system PROM not the EEPROM on the
 *   adapter port).
 * - Initialize the MTA (Multicast Table Array) to 0b. Per
 *   software, entries can be added to this table as desired.
 * - Program the Interrupt Mask Set/Read (IMS) register to enable any
 *   interrupt the software driver wants to be notified of when the event
 *   occurs. Suggested bits include RXT, RXO, RXDMT, RXSEQ, and LSC.
 *   There is no immediate reason to enable the transmit interrupts.
 * - If software uses the Receive Descriptor Minimum Threshold Interrupt,
 *   the Receive Delay Timer (RDTR) register should be initialized with
 *   the desired delay time.
 * - Allocate a region of memory for the receive descriptor list.
 *   Software should ensure this memory is aligned on a paragraph
 *   (16-byte) boundary. Program the Receive Descriptor Base Address
 *   (RDBAL/RDBAH) register(s) with the address of the region. RDBAL is
 *   used for 32-bit addresses and both RDBAL and RDBAH are used for
 *   64-bit addresses.
 * - Set the Receive Descriptor Length (RDLEN) register to the size
 *   (in bytes) of the descriptor ring. This register must be 128-byte
 *   aligned. The Receive Descriptor Head and Tail registers are
 *   initialized (by hardware) to 0b after a power-on or a software-
 *   initiated Ethernet controller reset. Receive buffers of appropriate
 *   size should be allocated and pointers to these buffers should be
 *   stored in the receive descriptor ring. Software initializes the
 *   Receive Descriptor Head (RDH) register and Receive Descriptor Tail
 *   (RDT) with the appropriate head and tail addresses. Head should point
 *   to the first valid receive descriptor in the descriptor ring and
 *   tail should point to one descriptor beyond the last valid descriptor
 *   in the descriptor ring.
 * Program the Receive Control (RCTL) register with appropriate values
 * for desired operation to include the following:
 * - Set the receiver Enable (RCTL.EN) bit to 1b for normal operation.
 *   However, it is best to leave the Ethernet controller receive logic
 *   disabled (RCTL.EN = 0b) until after the receive descriptor ring has
 *   been initialized and software is ready to process received packets.
 * - Set the Long Packet Enable (RCTL.LPE) bit to 1b when processing
 *   packets greater than the standard Ethernet packet size. For example,
 *   this bit would be set to 1b when processing Jumbo Frames.
 * - Loopback Mode (RCTL.LBM) should be set to 00b for normal operation.
 * - Configure the Receive Descriptor Minimum Threshold Size (RCTL.RDMTS)
 *   bits to the desired value.
 * - Configure the Multicast Offset (RCTL.MO) bits to the desired value.
 * - Set the Broadcast Accept Mode (RCTL.BAM) bit to 1b allowing the
 *   hardware to accept broadcast packets.
 * - Configure the Receive Buffer Size (RCTL.BSIZE) bits to reflect the
 *   size of the receive buffers software provides to hardware. Also
 *   configure the Buffer Extension Size (RCTL.BSEX) bits if receive
 *   buffer needs to be larger than 2048 bytes.
 * - Set the Strip Ethernet CRC (RCTL.SECRC) bit if the desire is for
 *   hardware to strip the CRC prior to DMA-ing the receive packet to
 *   host memory.
 * - For the 82541xx and 82547GI/EI, program the Interrupt Mask Set/Read
 *   (IMS) register to enable any interrupt the driver wants to be
 *   notified of when the even occurs. Suggested bits include RXT, RXO,
 *   RXDMT, RXSEQ, and LSC. There is no immediate reason to enable the
 *   transmit interrupts. Plan to optimize interrupts later, including
 *   programming the interrupt moderation registers TIDV, TADV, RADV and
 *   IDTR.
 * - For the 82541xx and 82547GI/EI, if software uses the Receive
 *   Descriptor Minimum Threshold Interrupt, the Receive Delay Timer
 *   (RDTR) register should be initialized with the desired delay time.
 *
 *
 * Transmit Initialisation:
 * - Allocate a region of memory for the transmit descriptor list.
 *   Software should insure this memory is aligned on a paragraph (16-
 *   byte) boundary. Program the Transmit Descriptor Base Address (TDBAL/
 *   TDBAH) register(s) with the address of the region. TDBAL is used for
 *   32-bit addresses and both TDBAL and TDBAH are used for 64-bit
 *   addresses.
 * - Set the Transmit Descriptor Length (TDLEN) register to the size (in
 *   bytes) of the descriptor ring. This register must be 128-byte
 *   aligned.
 * - The Transmit Descriptor Head and Tail (TDH/TDT) registers are
 *   initialized (by hardware) to 0b after a power-on or a software
 *   initiated Ethernet controller reset. Software should write 0b to
 *   both these registers to ensure this.
 * Initialize the Transmit Control Register (TCTL) for desired
 * operation to include the following:
 * - Set the Enable (TCTL.EN) bit to 1b for normal operation.
 * - Set the Pad Short Packets (TCTL.PSP) bit to 1b.
 * - Configure the Collision Threshold (TCTL.CT) to the desired value.
 *   Ethernet standard is 10h. This setting only has meaning in half
 *   duplex mode.
 * - Configure the Collision Distance (TCTL.COLD) to its expected value.
 *   For full duplex operation, this value should be set to 40h. For
 *   gigabit half duplex, this value should be set to 200h. For 10/100
 *   half duplex, this value should be set to 40h. Program the Transmit
 *   IPG (TIPG) register with the following decimal values to get the
 *   minimum legal Inter Packet Gap:
 *          FIBER   COPPER   FIBER (82544GC/EI)   COPPER(82544GC/EI)
 *   IPGT      10       10                    6                    8
 *   IPGR1     10       10                   8a                   8a
 *   IPGR2     10       10                   6a                   6a
 *   Where a == applicable to the 82541xx and 82547GI/EI.
 *
 *   Note: IPGR1 and IPGR2 are not needed in full duplex, but are
 *   easier to always program to the values shown.
 *
 *
 * The card may either be in 32 bit or 64 bit mode. This is indicated
 * by the BAR32 bit (bit 13) being set in the Initialisation Control
 * Word 1 (word 0x0a in the EEPROM). NOTE: bit is set for 82540EP and
 * 82540EM, the latter of which is the card QEMU emulates.
 * * FIXME: One thing I'm confused about here: how can we read a bit
 * from the EEPROM (which requires writing a command to the EEPROM
 * register, aka an offset from the base address) in order to know if
 * we need to use the 32-bit vs 64-bit base address to offset from?
 * Seems a bit chicken-and-eggy if you ask me.
 *
 *
 * Software can also directly access the EEPROM’s 4-wire interface
 * through the EEPROM/FLASH Control Register (EEC). It can use this for
 * reads, writes, or other EEPROM operations. To directly access the
 * EEPROM, software should follow these steps:
 *   1. Write a 1b to the EEPROM Request bit (EEC.EE_REQ).
 *   2. Read the EEPROM Grant bit (EEC.EE_GNT) until it becomes 1b. It
 *      remains 0b as long as the hardware is accessing the EEPROM.
 *   3. Write or read the EEPROM using the direct access to the 4-wire
 *   interface as defined in the EEPROM/FLASH Control & Data Register
 *   (EEC). The exact protocol used depends on the EEPROM placed on the
 *   board and can be found in the appropriate data sheet.
 *   4. Write a 0b to the EEPROM Request bit (EEC.EE_REQ).
 *
 * Software can cause the Ethernet controller to re-read the hardware
 * accessed fields of the EEPROM (setting hardware’s internal registers
 * appropriately) by writing a 1b to the EEPROM Reset bit of the
 * Extended Device Control Register (CTRL_EXT.EE_RST). This action will
 * also cause a reset.
 *
 */

/// VendorID for all: 8086
/// Stepping: 82547EI-A0  DeviceID:1019  Desc: Copper
/// Stepping: 82547EI-A1  DeviceID:1019  Desc: Copper
/// Stepping: 82547EI-B0  DeviceID:1019  Desc: Copper
/// Stepping: 82547EI-B0  DeviceID:101a  Desc: Mobile
/// Stepping: 82547GI-B0  DeviceID:1019  Desc: Copper
/// Stepping: 82546EB-A1  DeviceID:1010  Desc: Copper; Dual Port; MAC Default
/// Stepping: 82546EB-A1  DeviceID:1012  Desc: Fiber; Dual Port
/// Stepping: 82546EB-A1  DeviceID:101d  Desc: Fiber; Quad Port
/// Stepping: 82546GB-B0  DeviceID:1079  Desc: Copper; Dual Port
/// Stepping: 82546GB-B0  DeviceID:107a  Desc: Fiber; Dual Port
/// Stepping: 82546GB-B0  DeviceID:107b  Desc: SerDes; Dual Port
/// Stepping: 82545EM-A   DeviceID:100f  Desc: Copper
/// Stepping: 82545EM-A   DeviceID:1011  Desc: Fiber
/// Stepping: 82545GM-B   DeviceID:1026  Desc: Copper; MAC Default
/// Stepping: 82545GM-B   DeviceID:1027  Desc: Fiber
/// Stepping: 82545GM-B   DeviceID:1028  Desc: SerDes
/// Stepping: 82544EI-A4  DeviceID:1107  Desc: Copper; MAC Default
/// Stepping: 82544GC-A4  DeviceID:1112  Desc: Copper; MAC Default
/// Stepping: 82541EI-A0  DeviceID:1013  Desc: Copper; MAC Default
/// Stepping: 82541EI-B0  DeviceID:1013  Desc: Copper; MAC Default
/// Stepping: 82541EI-B0  DeviceID:1018  Desc: Mobile
/// Stepping: 82541GI-B1  DeviceID:1076  Desc: Copper
/// Stepping: 82541GI-B1  DeviceID:1077  Desc: Mobile
/// Stepping: 82541PI-C0  DeviceID:1076  Desc: Copper
/// Stepping: 82541ER-C0  DeviceID:1078  Desc: Copper
/// Stepping: 82540EP-A   DeviceID:1017  Desc: Desktop
/// Stepping: 82540EP-A   DeviceID:1016  Desc: Mobile
/// Stepping: 82540EM-A   DeviceID:100e  Desc: Desktop
/// Stepping: 82540EM-A   DeviceID:1015  Desc: Mobile

static constexpr bool is_82541xx(u16 deviceID) {
    return deviceID == 0x1013
        || deviceID == 0x1018
        || deviceID == 0x1076
        || deviceID == 0x1077
        || deviceID == 0x1078;
};

static constexpr bool is_82547_GI_EI(u16 deviceID) {
    return deviceID == 0x101a
        || deviceID == 0x1019;
}

/// REGISTERS ACCESSIBLE FROM BAR0 or BAR1:BAR0 DEPENDING ON BAR32 BIT

/// Category:    General
/// Permissions: R/W
/// CTRL  Device Control
/// Bits:
///   0      FD  Full Duplex (default 1, default 0 for 82541xx, 82547GI, and 82547EI only)
///            Enables software to override the hardware Auto-
///            Negotiation function. The FD sets the duplex mode only
///            if CTRL.FRCDPLX is set. When cleared, the Ethernet
///            controller operates in half-duplex; when set, the
///            Ethernet controller operates in full duplex. When the
///            Ethernet controller operates in TBI mode/internal SerDes
///            mode, and the AN Hardware is enabled, this bit is
///            ignored. When the Ethernet controller operates in TBI
///            mode/ internal SerDes, and the AN Hardware is disabled,
///            or the link is forced, this bit should be set by
///            software. When the Ethernet controller operates in
///            internal PHY mode, the FD bit is set by software based
///            on AN and data rate resolution.
///            Configurable through the EEPROM.
///   2:1    RESERVED  (clear these bits)
///   3      LRST  Link Reset (default 1)
///            Not applicable to the 82540EP, 82540EM, 82541xx, 82547GI, or 82547EI.
///            0b = Normal; 1b = Link Reset
///            Applicable only in TBI mode/internal SerDes of
///            operation. Used to reset the link control logic and
///            restart the Auto-Negotiation process, when TXCW.ANE is
///            set and TBI mode/internal SerDes is enabled.
///            When set, transmission and reception are halted
///            regardless of TBI mode/internal SerDes setting. A
///            transition to 0b initiates the Auto-Negotiation
///            function. Configurable from the EEPROM, allowing
///            initiation of Auto-Negotiation function at power up.
///   4      RESERVED  (clear this bit)
///   5      ASDE  Auto-Speed Detection Enable. (default 0)
///            When set, the Ethernet controller automatically detects
///            the resolved speed of the link by sampling the link in
///            internal PHY mode and self-configures the appropriate
///            status and control bits. Software must also set the SLU
///            bit for this operation. This function is ignored in TBI
///            mode/internal Serdes. The ASD feature provides a method
///            of determining the link speed without the need for
///            software accesses to the MII management registers.
///   6      SLU  Set Link Up (default 0)
///            In TBI mode/internal SerDes, provides manual link
///            configuration. When set, the Link Up signal is forced
///            high once receiver synchronization is achieved (LOS not
///            asserted) using CTRL.FD to determine the duplex mode.
///            This operation bypasses the link configuration process.
///            If Auto-Negotiation is enabled (TXCW.ANE equals 1b),
///            then Set Link Up is ignored. In internal PHY mode, this
///            bit must be set to 1b to permit the Ethernet controller
///            to recognize the I_LOS/I_LIND link signal from the PHY.
///            The "Set Link Up" is normally initialized to 0b.
///            However, if either the APM Enable or SMBus Enable bits
///            are set in the EEPROM then it is initialized to 1b,
///            ensuring MAC/PHY communication during preboot states
///            (for example, the 82547EI and 82541EI). Driver software
///            sets this bit when the driver software initializes,
///            therefore LED indications (link, activity, speed) are
///            not active until the software driver loads even though
///            the PHY has autonegotiated and established link with a
///            partner on the Ethernet.
///            Configurable through the EEPROM.
///   7      ILOS  Invert Loss-of-Signal (LOS) (default 0)
///            0b = do not invert (active high input signal); 1b = invert signal (active low input signal).
///            If using the internal PHY, this bit should be set to 0b
///            to ensure proper communication with the MAC. If using an
///            external TBI device, this bit can be set if the Ethernet
///            controller provides a link loss indication with negative
///            polarity.
///            NOTE: This is a reserved bit for the 82541xx and 82547GI/EI.
///   9:8    SPEED  Speed selection (default 0b10)
///            These bits determine the speed configuration and are
///            written by software after reading the PHY configuration
///            through the MDI/O interface. These signals are ignored
///            in TBI mode/internal Serdes or when Auto-Speed Detection
///            (CTRL.ASDE) is enabled
///            00b 10 Mb/s
///            01b 100 Mb/s
///            10b 1000 Mb/s
///            11b not used
///   10     RESERVED  (clear this bit)
///   11     FRCSPD  Force Speed (default 1)
///            When set, the Ethernet controller speed is configured by
///            CTRL.SPEED bits. The PHY device must resolve to the same
///            speed configuration or software must manually set it to
///            the same speed as the Ethernet controller. When cleared,
///            this allows the PHY device or ASD function (CTRL.ASDE is
///            set) to set the Ethernet controller speed. This bit is
///            superseded by the CTRL_EXT.SPD_BYPS bit, which has a
///            similar function. Applicable only in internal PHY mode
///            of operation and is configurable through EEPROM.
///   12     FRCDPLX  Force Duplex (default 0)
///            When set, software can override the duplex indication
///            from the PHY which is in internal PHY mode. When set the
///            CTRL.FD bit sets duplex. When cleared, the CTRL.FD is
///            ignored.
///   17:13  RESERVED  (clear these bits)
///   18     SDP0_DATA  SDP0 Data Value (default 0)
///            Used to read (write) value of software-controllable IO
///            pin SDP0. If SDP0 is configured as an output (SDP0_IODIR=1b),
///            this bit controls the value driven on the pin (initial
///            value EEPROM-configurable). If SDP0 is configured as an
///            input, reads return the current value of the pin.
///   19     SDP1_DATA  SDP1 Data Value (default 0)
///            Used to read (write) value of softwarecontrollable IO
///            pin SDP1. If SDP1 is configured as an output (SDP1_IODIR=1b),
///            this bit controls the value driven on the pin (initial
///            value EEPROM-configurable). If SDP1 is configured as an
///            input, reads return the current value of the pin.
///   20     ADVD3WUC  D3Cold Wakeup Capability Advertisement Enable (default 0)
///            When set, D3Cold wakeup capability is advertised based on
///            whether the AUX_PWR pin advertises presence of auxiliary
///            power (yes if AUX_PWR is indicated, no otherwise). When
///            0b, however, D3Cold wakeup capability is not advertised
///            even if AUX_PWR presence is indicated. Formerly used as
///            SDP2 pin data value, initial value is EEPROM-
///            configurable.
///            NOTE: Not applicable to the 82541ER
///   21     EN_PHY_PWR_MGMT  PHY Power-Management Enable (default 0, default 1 for 82541xx, 82547GI, and 82547EI only)
///            When set, the PHY is informed of power-state transitions
///            and attempts to autonegotiate advertising lower line
///            speeds only (10 or 100 Mb/sec) when entering D3 or D0u
///            power states with wakeup or manageability enabled. It
///            again re-negotiates, advertising full speed capabilities
///            (10/100/1000 Mbps) when transitioning back to full D0
///            operational state. If this bit is clear, the PHY
///            automatic speed/power management capability is disabled,
///            and the PHY remains operational at its current line speed
///            through powerstate transitions. Formerly used as SDP3 pin
///            data value, initial value is EEPROM-configurable.
///   22     SDP0_IODIR  SDP0 Pin Directionality (default 0)
///            Controls whether software-controllable pin SDP0 is
///            configured as an input or output (0b = input, 1b = output).
///            Initial value is EEPROM-configurable. This bit is not
///            affected by software or system reset, only by initial
///            power-on or direct software writes.
///   23     SDP1_IODIR  SDP1 Pin Directionality (default 0)
///            Controls whether software-controllable pin SDP1 is
///            configured as an input or output (0b = input, 1b = output).
///            Initial value is EEPROM-configurable. This bit is not
///            affected by software or system reset, only by initial
///            power-on or direct software writes.
///   25:24  RESERVED  (clear these bits)
///   26     RST  Device Reset (default 0)
///            0b = normal; 1b = reset. Self clearing.
///            When set, it globally resets the entire Ethernet
///            controller with the exception of the PCI configuration
///            registers. All registers (receive, transmit, interrupt,
///            statistics, etc.), and state machines are set to their
///            power-on reset values. This reset is equivalent to a PCI
///            reset, with the one exception being that the PCI
///            configuration registers are not reset.
///            To ensure that global device reset has fully completed
///            and that the Ethernet controller responds to subsequent
///            access, wait approximately 1 microsecond after setting
///            and before attempting to check to see if the bit has
///            cleared or to access any other device register.
///   27     RFCE  Receive Flow Control Enable (default 0)
///            When set, indicates that the Ethernet controller
///            responds to the reception of flow control packets.
///            Reception and responding to flow control packets
///            requires matching the content of the Ethernet
///            controller’s FCAL/H and FCT registers. If
///            AutoNegotiation is enabled, this bit is set to the
///            negotiated flow control value.
///   28     TFCE  Transmit Flow Control Enable (default 0)
///            When set, indicates that the Ethernet controller
///            transmits flow control packets (XON and XOFF frames)
///            based on the receive FIFO fullness, or when triggered to
///            do so based on external control pins (XOFF XON pins when
///            FCTRH.XFCE is set). If Auto-Negotiation is enabled, this
///            bit is set to the negotiated flow control value
///   29     RESERVED  (clear this bit)
///   30     VME  VLAN Mode Enable (default 0)
///            When set to 1b, all packets transmitted from the
///            Ethernet controller that have VLE bit set in their
///            descriptor is sent with an 802.1Q header added to the
///            packet. The contents of the header come from the
///            transmit descriptor and from the VLAN type register. On
///            receive, VLAN information is stripped from 802.1Q
///            packets and is loaded to the packet’s descriptor.
///            Reserved. Should be written with 0b to ensure future
///            compatibility.
///            NOTE: Not applicable to the 82541ER.
///   31     PHY_RST  PHY Reset (default 0)
///            0b = Normal.
///            1b = Assert hardware reset to the internal PHY.
///            The technique is to set the bit, wait approximately 3
///            microseconds, then clear the bit. For the 82547GI/
///            82541GI (B1 stepping), this register must be used
///            instead of a PHY register.
///            NOTE: For the 82546GB, when resetting the PHY through
///            the MAC, the PHY should be held in reset for a minimum
///            of 10 ms before releasing the reset signal.
#define REG_CTRL 0x0000
#define CTRL_FULL_DUPLEX (1 << 0)
#define CTRL_LINK_RESET (1 << 3)
#define CTRL_AUTO_SPEED_DETECTION_ENABLE (1 << 5)
#define CTRL_SET_LINK_UP (1 << 6)
#define CTRL_INVERT_LOSS_OF_SIGNAL (1 << 7)
#define CTRL_FORCE_SPEED_10MBS (0b00 << 8)
#define CTRL_FORCE_SPEED_100MBS (0b11 << 8)
#define CTRL_FORCE_SPEED_1000MBS (0b10 << 8)
#define CTRL_FORCE_SPEED (1 << 11)
#define CTRL_FORCE_DUPLEX (1 << 12)
#define CTRL_SDP0_DATA (1 << 18)
#define CTRL_SDP1_DATA (1 << 19)
#define CTRL_D3COLD_WAKEUP_CAPABILITY_ADVERTISEMENT_ENABLE (1 << 20)
#define CTRL_PHY_POWER_MANAGEMENT_ENABLE (1 << 21)
#define CTRL_SDP0_PIN_DIRECTIONALITY (1 << 22)
#define CTRL_SDP1_PIN_DIRECTIONALITY (1 << 23)
#define CTRL_DEVICE_RESET (1 << 26)
#define CTRL_RECEIVE_FLOW_CONTROL_ENABLE (1 << 27)
#define CTRL_TRANSMIT_FLOW_CONTROL_ENABLE (1 << 28)
#define CTRL_VLAN_MODE_ENABLE (1 << 30)
#define CTRL_PHY_RESET (1 << 31)

/// Category:    General
/// Permissions: R
/// STATUS  Device Status
/// Bits:
///   0      FD  Link Full Duplex configuration Indication
///            When cleared, the Ethernet controller operates in half-
///            duplex; when set, the Ethernet controller operates in
///            Full duplex. The FD provides the duplex setting status
///            of the Ethernet controller as set by either Hardware
///            Auto-Negotiation function, or by software.
///   1      LU  Link Up Indication
///            0b = no link config; 1b = link config.
///            For TBI mode/internal SerDes operation: If Auto-
///            Negotiation is enabled, this bit is set if a valid link
///            is negotiated. If link is forced through CTRL.SLU, it
///            reflects the status of this control bit.
///            For internal PHY mode of operation: Reflects the status
///            of the internal link signal indicating a transition to a
///            Link Up.
///   3:2    Function ID (default 0)
///            Provides software a mechanism to determine the Ethernet
///            controller function number (LAN identifier) for this
///            MAC. Read as: [0b,0b] LAN A, [0b,1b] LAN B.
///            NOTE: These settings are only applicable to the 82546GB
///            and 82546EB. For all other Ethernet controllers, set
///            these bits to 0b.
///   4      TXOFF  Transmission Paused
///            When set, Indicates the transmit function is in Pause
///            state due to reception of an XOFF pause frame when
///            symmetrical flow control is enabled. It is cleared upon
///            expiration of the pause timer, or receipt of an XON
///            frame. Applicable only while working in full-duplex
///            flow-control mode of operation.
///   5      TBIMODE  TBI Mode/internal SerDes Indication
///            When set, the Ethernet controller is configured to work
///            in TBI mode/internal SerDes of operation. When clear,
///            the Ethernet controller is configured to work in
///            internal PHY mode.
///            NOTE: For the 82544GC and 82544EI, reflects the status
///            of the TBI_MODE input pin. For all other Ethernet
///            controllers, set this bit to 0b.
///   7:6    SPEED  Link Speed Setting
///            Indicates the configured speed of the link. These bits
///            are either forced by software when forcing the link
///            speed through the CTRL.SPEED control bits, automatically
///            set by hardware when Auto-Speed Detection is enabled or
///            reflect the internal indication inputs from the PHY.
///            When Auto-Speed Detection is enabled, the Ethernet
///            controller’s speed is configured only once after the
///            internal link is asserted.
///            Speed indication is mapped as follows:
///            00b = 10 Mb/s
///            01b = 100 Mb/s
///            10b = 1000 Mb/s
///            11b = 1000 Mb/s
///            These bits are not valid in TBI mode/internal SerDes.
///   9:8    ASDV  Auto Speed Detection Value
///            Indicates the speed sensed by the Ethernet controller
///            from the internal PHY. The ASDV status bits are provided
///            for diagnostics purposes. The ASD function can be
///            initiated by software writing a logic 1b to the
///            CTRL_EXT.ASDCHK bit. The resultant speed detection is
///            reflected in these bits.
///   10     RESERVED  (reads as 0)
///   11     PCI66  PCI Bus speed indication
///            When set, indicates that the PCI bus is running at
///            66 MHz. Reflects the M66EN input pin.
///            NOTE: Not applicable to the 82547GI or 82547EI.
///   12     BUS64  PCI Bus Width indication
///            When set, indicates that the Ethernet controller is
///            sitting on a 64-bit PCI/PCI-X bus. BUS64 is determined
///            by REQ64# assertion
///   13     PCIX_MODE  PCI-X Mode indication
///            When set to 1b, the Ethernet controller is operating in
///            PCI-X Mode; otherwise, the Ethernet controller is
///            operating in conventional PCI Mode.
///   15:14  PCIXSPD  PCI-X Bus Speed Indication
///            Attempts to indicate the speed of the bus when operating
///            in a PCI-X bus. Only valid when STATUS.PCIX_MODE = 1b.
///            00b = 50-66 MHz
///            01b = 66-100 MHz
///            10b = 100-133 MHz
///            11b = Reserved
///   31:16  RESERVED  (reads as 0)
#define REG_STATUS 0x0008
#define STATUS_FULL_DUPLEX (1 << 0)
#define STATUS_LINK_UP (1 << 1)
#define STATUS_FUNCTION_ID_MASK (0b11 << 2)
#define STATUS_LAN_A (0b00 << 2)
#define STATUS_LAN_B (0b01 << 2)
#define STATUS_TXOFF (1 << 4)
#define STATUS_TBIMODE (1 << 5)
#define STATUS_LINK_SPEED_MASK (0b11 << 6)
#define STATUS_LINK_SPEED_10MBS (0b00 << 6)
#define STATUS_LINK_SPEED_100MBS (0b01 << 6)
#define STATUS_LINK_SPEED_1000MBS_0 (0b10 << 6)
#define STATUS_LINK_SPEED_1000MBS_1 (0b11 << 6)
#define STATUS_ASDV (0b11 << 8)
#define STATUS_PCI66 (1 << 11)
#define STATUS_BUS64 (1 << 12)
#define STATUS_PCIX_MODE (1 << 13)
#define STATUS_PCIX_SPEED_MASK (0b11 << 14)
#define STATUS_PCIX_SPEED_50_66_MHZ (0b00 << 14)
#define STATUS_PCIX_SPEED_66_100_MHZ (0b01 << 14)
#define STATUS_PCIX_SPEED_100_133_MHZ (0b10 << 14)


/// Category:    General
/// Permissions: R/W
/// EECD  EEPROM/Flash Control/Data
/// Bits:
///   0      SK  Clock input to the EEPROM (default 0)
///            The EESK output signal is mapped to this bit and
///            provides the serial clock input to the EEPROM. Software
///            clocks the EEPROM by means of toggling this bit with
///            successive writes to EECD.
///   1      CS  Chip select input to the EEPROM (default 0)
///            The EECS output signal is mapped to the chip select of
///            the EEPROM device. Software enables the EEPROM by
///            writing a 1b to this bit.
///   2      DI  Data input to the EEPROM (default 0)
///            The EEDI output signal is mapped directly to this bit.
///            Software provides data input to the EEPROM through
///            writes to this bit.
///   3      DO  Data output bit from the EEPROM
///            The EEDO input signal is mapped directly to this bit in
///            the register and contains the EEPROM data output. This
///            bit is read-only from the software perspective – writes
///            to this bit have no effect.
///   5:4    FWE  Flash Write Enable Control (default 0b01)
///            These two bits control whether writes to Flash memory are allowed.
///            00b = Not allowed
///            01b = Flash writes disabled
///            10b = Flash writes enabled
///            11b = Not allowed
///   6*     EE_REQ  Request EEPROM Access (default 0)
///            The software must write a 1b to this bit to get direct
///            EEPROM access. It has access when EE_GNT is 1b. When the
///            software completes the access it must write a 0b.
///   7*     EE_GNT  Grant EEPROM Access (default 0)
///            When this bit is 1b the software can access the EEPROM
///            using the SK, CS, DI, and DO bits.
///   8*     EE_PRES  EEPROM Present (default 1, default 0 for 82541xx, 82547GI, and 82547EI only)
///            This bit indicates that an EEPROM is present by
///            monitoring the EEDO input for a active-low acknowledge
///            by the serial EEPROM during initial EEPROM scan.
///            1b == EEPROM present.
///   9*     EE_SIZE  EEPROM Size (default 0)
///            0b = 1024-bit (64 word) NM93C46 compatible EEPROM
///            1b = 4096-bit (256 word) NM93C66 compatible EEPROM
///            This bit indicates the EEPROM size, based on
///            acknowledges seen during EEPROM scans of different
///            addresses. This bit is read-only.
///            NOTE: This is a reserved bit for the 82541xx and 82547GI/EI.
///   10*    EE_SIZE (82541xx, 82547GI, and 82547EI only)
///            For Microwire EEPROMs:
///              0b = 6-bit addressable (64 words).
///              1b = 8-bit addressable (256 words).
///            For SPI EEPROMs:
///              0b = 8-bit addressable.
///              1b = 16-bit addressable.
///   12:11  RESERVED  (clear these bits)
///   13*    EE_TYPE  EEPROM Type: Reflects the EE_MODE pin. (82541xx, 82547GI, 82547EI)
///            0b = Microwire.
///            1b = SPI.
///   31:14  RESERVED (clear these bits)
/// * == not applicable to 82544GC or 82544EI
///
/// Bits (for 82544GC and 82544EI):
///   0      SK
///   1      CS
///   2      DI
///   3      DO
///   5:4    FWE
///   31:6   RESERVED (clear these bits)
#define REG_EECD 0x0010

/// SEE ABOVE FOR MORE ON EECD BITS AND THEIR MEANING
#define EECD_CLOCK (1 << 0)
#define EECD_CHIP_SELECT (1 << 1)
#define EECD_DATA_IN (1 << 2)
#define EECD_DATA_OUT (1 << 3)
/// NOTE: One of these bits must be set, but not both and not neither!
/// So always write 0b01 or 0b10 to these bits.
#define EECD_FLASH_WRITE_DISABLED (0b01 << 4)
#define EECD_FLASH_WRITE_ENABLED (0b10 << 4)
#define EECD_FLASH_WRITE_MASK (0b11 << 4)
#define EECD_EEPROM_REQUEST (1 << 6)
#define EECD_EEPROM_GRANT (1 << 7)
#define EECD_EEPROM_PRESENT (1 << 8)
#define EECD_EEPROM_SIZE (1 << 9)
#define EECD_EEPROM_TYPE (1 << 13)

/// Category:    General
/// Permissions: R/W
/// EERD  EEPROM Read
/// NOTE: (not applicable to the 82544GC or 82544EI)
/// NOTE: *_EXTRA are required to be used by 82541xx, 82547GI, and 82547EI.
/// NOTE: If EECD register indicates that software has direct pin
/// control of the EEPROM, access through the EERD register can stall
/// until that bit is clear. Software should ensure that EECD.EE_REQ
/// and EECD.EE_GNT bits are clear before attempting to use EERD to
/// access the EEPROM.
/// Bits:
///   0      START     Start Read (default 0)
///            Writing a 1 to this bit causes the EEPROM to read
///            16-bits at the address stored in the EE_ADDR
///            field and then store the result in the EE_DATA
///            field. This bit is self-clearing.
///   3:1    RESERVED  Reads as 0
///   4      DONE      Read Done
///            Set to 1 when the EEPROM read completes. Set to 0
///            when the EEPROM read is in progress. Writes by
///            software are ignored.
///   7:5    RESERVED  Reads as 0
///   15:8   ADDR      Read Address
///            This field is written by software along with Start
///            Read to indicate the word to read.
///   31:16  DATA   Read Data
///            Data returned from the EEPROM read.
///
/// Bits (for 82541xx, 82547GI, and 82547EI):
///   0      START     Start Read (default 0)
///            Writing a 1b to this bit causes the EEPROM to read a
///            (16-bit) word at the address stored in the EE_ADDR
///            field and then storing the result in the EE_DATA
///            field. This bit is self-clearing.
///   1      DONE      Read Done (default 0)
///            Set to 1b when the EEPROM read completes.
///            Set to 0b when the EEPROM read is in progress.
///            Writes by software are ignored.
///   15:2   ADDR      Read Address
///            This field is written by software along with Start
///            Read to indicate the word to read.
///   31:16  DATA      Read Data
///            Data returned from the EEPROM read.
#define REG_EEPROM 0x0014
/// Set this bit to issue a read of the EEPROM. Write
/// EERD_START | EERD_ADDRESS(address) to EERD to set the address that
/// will be read from in the EEPROM address space.
#define EERD_START (1 << 0)
/// Check this bit in EERD register to see if the read previously
/// issued is done/has finished.
#define EERD_DONE (1 << 4)
#define EERD_DONE_EXTRA (1 << 1)
/// Use this to prepare an address to store into the EERD register.
constexpr auto EERD_ADDRESS(u8 address) { return u32(address) << 8; };
constexpr auto EERD_ADDRESS_EXTRA(u8 address) { return u32(address) << 2; };
/// Get 16-bit data word from EERD register after a read is done.
constexpr auto EERD_DATA(u32 eerd) { return u16(eerd >> 16); };

/// Category:    General
/// Permissions: R/W
/// FLA  Flash Access
/// NOTE: (appicable to the 82541xx, 82547GI, and 82547EI only)
#define REG_FLASH_ACCESS 0x001c
/// Category:    General
/// Permissions: R/W
/// CTRL_EXT  Extended Device Control
#define REG_CTRL_EXT 0x0018
/// Category:    General
/// Permissions: R/W
/// MDIC  MDI Control
#define REG_MDI_CONTROL 0x0020
/// Category:    General
/// Permissions: R/W
/// FCAL  Flow Control Address Low
#define REG_FCAL 0x0028
/// Category:    General
/// Permissions: R/W
/// FCAH  Flow Control Address High
#define REG_FCAH 0x002c
/// Category:    General
/// Permissions: R/W
/// FCT  Flow Control Type
#define REG_FCT 0x0030
/// Category:    General
/// Permissions: R/W
/// VET  VLAN Ether Type
#define REG_VLAN_ETHERTYPE 0x0030
/// Category:    General
/// Permissions: R/W
/// FCTTV  Flow Control Transmit Timer Value
#define REG_FCTTV 0x0170
/// Category:    General
/// Permissions: R/W
/// TXCW  Transmit Configuration Word
/// NOTE: (not applicable to the 82540EP, 82540EM, 82541xx, 82547GI, or 82547EI)
#define REG_TXCW 0x0178
/// Category:    General
/// Permissions: R
/// RXCW  Receive Configuration Word
/// NOTE: (not applicable to the 82540EP, 82540EM, 82541xx, 82547GI, or 82547EI)
#define REG_RXCW 0x0180
/// Category:    General
/// Permissions: R/W
/// LEDCTL  LED Control
/// NOTE: (not applicable to the 82544GC or 82544EI)
#define REG_LEDCTL 0x0e00
/// Category:    DMA
/// Permissions: R/W
/// PBA  Packet Buffer Allocation
#define REG_PBA 0x1000
/// Category:    Interrupt
/// Permissions: R
/// ICR  Interrupt Cause Read
/// Bits:
///   0      TXDW    Transmit Descriptor Written Back
///   1      TXQE    Transmit Queue Empty
///   2      LSC     Link Status Change
///   3      RXSEQ   Receive Sequence Error
///   4      RXDMT0  Receive Descriptor Minimum Threshold Reached
///   5      RESERVED (reads as 0)
///   6      RXO     Receiver Overrun
///   7      RXT0    Receiver Timer Interrupt
///   8      RESERVED (reads as 0)
///   9      MDAC    MDI/O Access Complete
///   10     RXCFG   Receiving /C/ ordered sets. Mapped to RXCW.RxConfig
///   15     TXD_LOW Transmit Descriptor Low Threshold hit
///   16     SRPD    Small Receive Packet Detected
///   31:17  RESERVED (reads as 0)
#define REG_ICR 0x00c0
#define ICR_TX_DESC_WRITTEN_BACK (1 << 0)
#define ICR_TX_QUEUE_EMPTY (1 << 1)
#define ICR_LINK_STATUS_CHANGE (1 << 2)
#define ICR_RX_SEQUENCE_ERROR (1 << 3)
#define ICR_RX_DESC_MINIMUM_THRESHOLD_REACHED (1 << 4)
#define ICR_RX_OVERRUN (1 << 6)
#define ICR_RX_TIMER_INTERRUPT (1 << 7)
#define ICR_MDIO_ACCESS_COMLETE (1 << 9)
#define ICR_RX_CONFIGURATION (1 << 10)
#define ICR_TX_DESC_LOW_THRESHOLD_HIT (1 << 15)
#define ICR_SMALL_RECEIVE_PACKET (1 << 16)
/// Category:    Interrupt
/// Permissions: R/W
/// ITR  Interrupt Throttling
/// NOTE: (not applicable to the 82544GC or 82544EI)
#define REG_ITR 0x00c4
/// Category:    Interrupt
/// Permissions: W
/// ICS  Interrupt Cause Set
#define REG_ICS 0x00c8
/// Category:    Interrupt
/// Permissions: R/W
/// IMS  Interrupt Mask Set/Read
/// Bits:
///   0      TXDW  Transmit Descriptor Written Back
///   1      TXQE  Transmit Queue Empty
///   2      LSC  Link Status Change
///   3      RXSEQ  Receive Sequence Error
///   4      RXDMT0  Receive Descriptor Minimum Threshold hit
///   5      RESERVED  (clear this bit)
///   6      RXO  Receiver FIFO Overrun
///   7      RXT0  Receiver Timer Interrupt
///   8      RESERVED  (clear this bit)
///   9      MDAC  MDI/O Access Complete Interrupt
///   10     RXCFG  Receiving /C/ ordered sets.
///            NOTE: This is a reserved bit for the 82541xx and
///            82547GI/EI. Set to 0b.
///   11     RESERVED  (clear this bit)
///            Should be written with 0b to ensure future compatibility (not
///            applicable to the 82544GC/EI).
///   12     PHYINT  PHY Interrupt (not applicable to the 82544GC/EI)
///   14:11  GPI  General Purpose Interrupts (8254GC/EI only)
///   14:13  GPI  General Purpose Interrupts
///   15     TXD_LOW  Transmit Descriptor Low Threshold hit
///            NOTE: not applicable to the 82544GC/EI.
///   16     SRPD  Small Receive Packet Detection
///            NOTE: not applicable to the 82544GC/EI.
///   31:17  RESERVED  (clear these bits)
#define REG_IMASK 0x00d0
#define IMASK_TX_DESC_WRITTEN_BACK (1 << 0)
#define IMASK_TX_QUEUE_EMPTY (1 << 1)
#define IMASK_LINK_STATUS_CHANGE (1 << 2)
#define IMASK_RX_SEQUENCE_ERROR (1 << 3)
#define IMASK_RX_DESC_MIN_THRESHOLD_HIT (1 << 4)
#define IMASK_RX_FIFO_OVERRUN (1 << 6)
#define IMASK_RX_TIMER_INTERRUPT (1 << 7)
#define IMASK_MDIO_ACCESS_COMPLETE (1 << 9)
#define IMASK_RX_C_ORDERED_SETS (1 << 10)
#define IMASK_PHY_INTERRUPT (1 << 12)
#define IMASK_TX_DESC_LOW_THRESHOLD_HIT (1 << 15)
#define IMASK_RX_SMALL_PACKET_DETECTION (1 << 16)

/// Category:    Interrupt
/// Permissions: W
/// IMC  Interrupt Mask Clear
#define REG_IMASK_CLEAR 0x00d8
/// Category:    Receive
/// Permissions: R/W
/// RCTL  Receive Control
/// Bits:
///   0      RESERVED  (clear this bit)
///   1      EN  Receiver Enable (default 0)
///            The receiver is enabled when this bit is 1b.
///            Writing this bit to 0b stops reception after receipt of
///            any in-progress packets. Data remains in the receive
///            FIFO until the device is re–enabled.
///            Disabling or re-enabling the receiver does not
///            reinitialize the packet filter logic that demarcates
///            packet start and end locations in the FIFO; Therefore
///            the receiver must be reset before re-enabling it.
///   2      SBP  Store Bad Packets (default 0)
///            0b = do not store.
///            1b = store bad packets.
///            When set, the Ethernet controller stores bad packets
///            (CRC error, symbol error, sequence error, length error,
///            alignment error, short packets or where carrier
///            extension or RX_ERR errors) that pass the filter
///            function in host memory. When the Ethernet controller is
///            in promiscuous mode, and SBP is set, it might possibly
///            store all packets.
///   3      UPE  Unicast Promiscuous Enabled (default 0)
///            0b = Disabled.
///            1b = Enabled.
///            When set, passes without filtering out all received
///            unicast packets.
///            Otherwise, the Ethernet controller accepts or rejects
///            unicast packets based on the received packet destination
///            address match with 1 of the 16 stored addresses.
///   4      MPE  Multicast Promiscuous Enabled (default 0)
///            0b = Disabled.
///            1b = Enabled.
///            When set, passes without filtering out all received
///            multicast packets.
///            Otherwise, the Ethernet controller accepts or rejects a
///            multicast packet based on its 4096-bit vector multicast
///            filtering table.
///   5      LPE  Long Packet Reception Enable (default 0)
///            0b = Disabled.
///            1b = Enabled.
///            LPE controls whether long packet reception is permitted.
///            When LPE is cleared, the Ethernet controller discards
///            packets longer than 1522 bytes. When LPE is set, the
///            Ethernet controller discards packets that are longer
///            than 16384 bytes.
///            NOTE: For the 82541xx and 82547GI/EI, packets larger
///            than 2 KB require full duplex operation.
///   7:6    LBM  Loopback mode (default 0)
///            Controls the loopback mode of the Ethernet controller.
///            00b = No loopback.
///            01b = Undefined.
///            10b = Undefined.
///            11b = PHY or external SerDes loopback.
///            All loopback modes are only allowed when the Ethernet
///            controller is configured for full-duplex operation.
///            Receive data from transmit data looped back internally
///            to the SerDes or internal PHY. In TBI mode (82544GC/EI),
///            the EWRAP signal is asserted.
///            NOTE: The 82540EP/EM, 82541xx, and 82547GI/EI do not
///            support SerDes functionality.
///   9:8    RDMTS  Receive Descriptor Minimum Threshold Size (default 0)
///            The corresponding interrupt ICR.RXDMT0 is set each time
///            the fractional number of free descriptors becomes equal
///            to RDMTS. The following table lists which fractional
///            values correspond to RDMTS values. The size of the total
///            receiver circular descriptor buffer is set by RDLEN.
///            00b = Free Buffer threshold is set to 1/2 of RDLEN.
///            01b = Free Buffer threshold is set to 1/4 of RDLEN.
///            10b = Free Buffer threshold is set to 1/8 of RDLEN.
///            11b = Reserved.
///   11:10  RESERVED  (clear these bits)
///   13:12  MO  Multicast Offset
///            The Ethernet controller is capable of filtering
///            multicast packets based on 4096-bit vector multicast
///            filtering table. The MO determines which bits of the
///            incoming multicast address are used in looking up the
///            4096-bit vector.
///            00b = bits [47:36] of received destination multicast address.
///            01b = bits [46:35] of received destination multicast address.
///            10b = bits [45:34] of received destination multicast address.
///            11b = bits [43:32] of received destination multicast address.
///   14     RESERVED  (clear this bit)
///   15     BAM  Broadcast Accept Mode (default 0)
///            0 = ignore broadcast; 1 = accept broadcast packets.
///            When set, passes and does not filter out all received
///            broadcast packets. Otherwise, the Ethernet controller
///            accepts, or rejects a broadcast packet only if it
///            matches through perfect or imperfect filters.
///   17:16  BSIZE  Receive Buffer Size (default 0)
///            Controls the size of the receive buffers, allowing the
///            software to trade off between system performance and
///            storage space. Small buffers maximize memory efficiency
///            at the cost of multiple descriptors for bigger packets.
///            RCTL.BSEX = 0b:
///              00b = 2048 Bytes
///              01b = 1024 Bytes
///              10b = 512 Bytes
///              11b = 256 Bytes
///            RCTL.BSEX = 1b:
///              00b = Reserved; software should not program this value
///              01b = 16384 Bytes
///              10b = 8192 Bytes
///              11b = 4096 Bytes
///   18     VFE  VLAN Filter Enable
///            0b = Disabled (filter table does not decide packet acceptance).
///            1b = Enabled (filter table decides packet acceptance for 802.1Q packets).
///            Three bits control the VLAN filter table. RCTL.VFE
///            determines whether the VLAN filter table participates in
///            the packet acceptance criteria. RCTL.CFIEN and RCTL.CFI
///            are used to decide whether the CFI bit found in the 802.
///            1Q packet’s tag should be used as part of the acceptance
///            criteria.
///   19     CFIEN  Canonical Form Indicator Enable
///   20     CFI  Canonical Form Indicator bit value
///            If RCTL.CFIEN is set, then 802.1Q packets with CFI equal
///            to this field is accepted; otherwise, the 802.1Q packet
///            is discarded.
///   21     RESERVED  (clear this bit)
///   22     DPF  Discard Pause Frames (default 0)
///            0 = incoming pause frames subject to filter comparison.
///            1 = incoming pause frames are filtered out even if they
///                match filter registers.
///            DPF controls the DMA function of flow control PAUSE
///            packets addressed to the station address (RAH/L[0]). If
///            a packet is a valid flow control packet and is addressed
///            to the station’s address, it is not transferred to host
///            memory if RCTL.DPF = 1b. However, it is transferred when
///            DPF is set to 0b.
///   23     PMCF  Pass MAC Control Frames (default 0)
///            0b = Do not (specially) pass MAC control frames.
///            1b = Pass any MAC control frame (type field value of
///            0x8808) that does not contain the pause opcode of 0x0001.
///            PMCF controls the DMA function of MAC control frames
///            (other than flow control). A MAC control frame in this
///            context must be addressed to either the MAC control
///            frame multicast address or the station address, match
///            the type field and NOT match the PAUSE opcode of 0x0001.
///            If PMCF = 1 then frames meeting this criteria are
///            transferred to host memory. Otherwise, they are filtered
///            out.
///   24     RESERVED (clear this bit)
///   25     BSEX  Buffer Size Extension (default 0)
///            When set to one, the original BSIZE values are
///            multiplied by 16. Refer to the RCTL.BSIZE bit
///            description.
///   26     SECRC  Strip Ethernet CRC from incoming packet (default 0)
///            0b = Do not strip CRC field.
///            1b = Strip CRC field.
///            Controls whether the hardware strips the Ethernet CRC
///            from the received packet. This stripping occurs prior to
///            any checksum calculations. The stripped CRC is not
///            transferred to host memory and is not included in the
///            length reported in the descriptor.
///   31:27  RESERVED (clear these bits)
#define REG_RX_CONTROL 0x0100
#define RCTL_ENABLE (1 << 1)
#define RCTL_STORE_BAD_PACKETS (1 << 2)
#define RCTL_UNICAST_PROMISCUOUS_ENABLED (1 << 3)
#define RCTL_MULTICAST_PROMISCUOUS_ENABLED (1 << 4)
#define RCTL_LONG_PACKET_RECEPTION_ENABLE (1 << 5)
#define RCTL_LOOPBACK_MODE_MASK (0b11 << 6)
#define RCTL_LOOPBACK_MODE_OFF (0b00 << 6)
#define RCTL_LOOPBACK_MODE_ON (0b11 << 6)
#define RCTL_DESC_MIN_THRESHOLD_SIZE_MASK (0b11 << 8)
#define RCTL_DESC_MIN_THRESHOLD_SIZE_HALF (0b00 << 8)
#define RCTL_DESC_MIN_THRESHOLD_SIZE_FOURTH (0b01 << 8)
#define RCTL_DESC_MIN_THRESHOLD_SIZE_EIGHTH (0b10 << 8)
#define RCTL_MULTICAST_OFFSET_MASK (0b11 << 12)
#define RCTL_MULTICAST_OFFSET_47_36 (0b00 << 12)
#define RCTL_MULTICAST_OFFSET_46_35 (0b01 << 12)
#define RCTL_MULTICAST_OFFSET_45_34 (0b10 << 12)
#define RCTL_MULTICAST_OFFSET_43_32 (0b11 << 12)
#define RCTL_BROADCAST_ACCEPT_MODE (1 << 15)
#define RCTL_BUFFER_SIZE_MASK (0b11 << 16)
#define RCTL_BUFFER_SIZE_2048 (0b00 << 16)
#define RCTL_BUFFER_SIZE_1024 (0b01 << 16)
#define RCTL_BUFFER_SIZE_512 (0b10 << 16)
#define RCTL_BUFFER_SIZE_256 (0b11 << 16)
#define RCTL_BUFFER_SIZE_16384 ((0b01 << 16) & (1 << 25))
#define RCTL_BUFFER_SIZE_8192 ((0b10 << 16) & (1 << 25))
#define RCTL_BUFFER_SIZE_4096 ((0b11 << 16) & (1 << 25))
#define RCTL_VLAN_FILTER_ENABLE (1 << 18)
#define RCTL_CANONICAL_FORM_INDICATOR_ENABLE (1 << 19)
#define RCTL_CANONICAL_FORM_INDICATOR_VALUE (1 << 20)
#define RCTL_DISCARD_PAUSE_FRAMES (1 << 22)
#define RCTL_PASS_MAC_CONTROL_FRAMES (1 << 23)
#define RCTL_BUFFER_SIZE_EXTEND (1 << 25)
#define RCTL_STRIP_ETHERNET_CRC (1 << 26)

/// Category:    Receive
/// Permissions: R/W
/// FCRTL  Flow Control Receive Threshold Low
#define REG_FCRTL 0x2160
/// Category:    Receive
/// Permissions: R/W
/// FCRTH  Flow Control Receive Threshold High
#define REG_FCRTH 0x2168
/// Category:    Receive
/// Permissions: R/W
/// RDBAL  Receive Descriptor Base Low
#define REG_RXDESCLO 0x2800
/// Category:    Receive
/// Permissions: R/W
/// RDBAH  Receive Descriptor Base High
#define REG_RXDESCHI 0x2804
/// Category:    Receive
/// Permissions: R/W
/// RDLEN  Receive Descriptor Length
#define REG_RXDESCLEN 0x2808
/// Category:    Receive
/// Permissions: R/W
/// RDH  Receive Descriptor Head
#define REG_RXDESCHEAD 0x2810
/// Category:    Receive
/// Permissions: R/W
/// RDT  Receive Descriptor Tail
#define REG_RXDESCTAIL 0x2818
/// Category:    Receive
/// Permissions: R/W
/// RDTR  Receive Delay Timer
#define REG_RDTR 0x2820
/// Category:    Receive
/// Permissions: R/W
/// RADV  Receive Interrupt Absolute Delay Timer
/// NOTE: (not applicable to the 82544GC or 82544EI)
#define REG_RADV 0x282c
/// Category:    Receive
/// Permissions: R/W
/// RSRPD  Receive Small Packet Detect Interrupt
/// NOTE: (not applicable to the 82544GC or 82544EI)
#define REG_RSRPD 0x2c00
/// Category:    Transmit
/// Permissions: R/W
/// TCTL  Transmit Control
#define REG_TCTL 0x0400
/// Category:    Transmit
/// Permissions: R/W
/// TIPG  Transmit IPG
#define REG_TIPG 0x0410
/// Category:    Transmit
/// Permissions: R/W
/// AIFS  Adaptive IFS Throttle - AIT
#define REG_AIFS 0x0458
/// Category:    Transmit
/// Permissions: R/W
/// TDBAL  Transmit Descriptor Base Low
#define REG_TDBAL 0x3800
/// Category:    Transmit
/// Permissions: R/W
/// TDBAH  Transmit Descriptor Base High
#define REG_TDBAH 0x3804
/// Category:    Transmit
/// Permissions: R/W
/// TDLEN  Transmit Descriptor Length
#define REG_TDLEN 0x3808
/// Category:    Transmit
/// Permissions: R/W
/// TDH  Transmit Descriptor Head
#define REG_TDHEAD 0x3810
/// Category:    Transmit
/// Permissions: R/W
/// TDT  Transmit Descriptor Tail
#define REG_TDTAIL 0x3818
/// Category:    Transmit
/// Permissions: R/W
/// TIDV  Transmit Interrupt Delay Value
#define REG_TIDV 0x3820
/// Category:    TX DMA
/// Permissions: R/W
/// TXDMAC  TX DMAControl
/// NOTE: (applicable to the 82544GC and 82544EI only).
#define REG_TXDMAC 0x3000
/// Category:    TX DMA
/// Permissions: R/W
/// TXDCTL  Transmit Descriptor Control
/// NOTE: (applicable to the 82544GC and 82544EI only).
#define REG_TXDCTL 0x3828
/// Category:    TX DMA
/// Permissions: R/W
/// TADV  Transmit Absolute Interrupt Delay Timer
/// NOTE: (not applicable to the 82544GC and 82544EI).
#define REG_TADV 0x282c
/// Category:    TX DMA
/// Permissions: R/W
/// TSPMT  TCP Segmentation Pad and Threshold
#define REG_TSPMT 0x3830
/// Category:    RX DMA
/// Permissions: R/W
/// RXDCTL  Receive Descriptor Control
#define REG_RXDCTL 0x2828
/// Category:    RX DMA
/// Permissions: R/W
/// RXCSUM  Receive Checksum Control
#define REG_RXCSUM 0x5000
/// Category:    Receive
/// Permissions: R/W
/// MTA[127:0]  Multicast Table Array (n)
#define REG_MTA_BEGIN 0x5200
#define REG_MTA_END 0x53fc
/// Category:    Receive
/// Permissions: R/W
/// RAL(8*n)  Receive Address Low (n)
#define REG_RAL_BEGIN 0x5400
#define REG_RAL_END 0x5478
/// Category:    Receive
/// Permissions: R/W
/// RAH(8*n)  Receive Address High (n)
#define REG_RAH_BEGIN 0x5404
#define REG_RAH_END 0x547c
/// Category:    Receive
/// Permissions: R/W
/// VFTA[127:0]  VLAN Filter Table Array (n)
/// NOTE: (not applicable to the 82541ER)
#define REG_VFTA_BEGIN 0x5600
#define REG_VFTA_END 0x57fc
/// Category:    Wakeup
/// Permissions: R/W
/// WUC  Wakeup Control
#define REG_WAKEUP_CONTROL 0x5800
/// Category:    Wakeup
/// Permissions: R/W
/// WUFC  Wakeup Filter Control
#define REG_WAKEUP_FILTER 0x5808
/// Category:    Wakeup
/// Permissions: R
/// WUS  Wakeup Status
#define REG_WAKEUP_STATUS 0x5810
/// Category:    Wakeup
/// Permissions: R/W
/// IPAV  IP Address Valid
#define REG_IP_ADDRESS_VALID 0x5838
/// Category:    Wakeup
/// Permissions: R/W
/// IP4AT  IPv4 Address Table
/// For 82544GC and 82544EI: IPAT  IP Address Table
#define REG_IPV4_TABLE_BEGIN 0x5840
#define REG_IPV4_TABLE_END 0x5858
/// Category:    Wakeup
/// Permissions: R/W
/// IP6AT  IPv6 Address Table
/// NOTE: (not applicable to the 82544GC and 82544EI)
#define REG_IPV6_TABLE_BEGIN 0x5880
#define REG_IPV6_TABLE_END 0x588c
/// Category:    Wakeup
/// Permissions: R/W
/// WUPL  Wakeup Packet Length
#define REG_WAKEUP_PACKET_LENGTH 0x5900
/// Category:    Wakeup
/// Permissions: R/W
/// WUPM  Wakeup Packet Memory
#define REG_WAKEUP_PACKET_MEMORY_BEGIN 0x5a00
#define REG_WAKEUP_PACKET_MEMORY_END 0x5a7c
/// Category:    Wakeup
/// Permissions: R/W
/// FFLT  Flexible Filter Length Table
#define REG_WAKEUP_FILTER_LENGTH_TABLE_BEGIN 0x5f00
#define REG_WAKEUP_FILTER_LENGTH_TABLE_END 0x5f18
/// Category:    Wakeup
/// Permissions: R/W
/// FFMT  Flexible Filter Mask Table
#define REG_WAKEUP_FILTER_MASK_TABLE_BEGIN 0x9000
#define REG_WAKEUP_FILTER_MASK_TABLE_END 0x93f8
/// Category:    Wakeup
/// Permissions: R/W
/// FFVT  Flexible Filter Value Table
#define REG_WAKEUP_FILTER_VALUE_TABLE_BEGIN 0x9800
#define REG_WAKEUP_FILTER_VALUE_TABLE_END 0x9bf8
/// Category:    Statistics
/// Permissions: R
/// CRCERRS  CRC Error Count
#define REG_CRC_ERROR_COUNT 0x4000
/// Category:    Statistics
/// Permissions: R
/// ALGNERRC  Alignment Error Count
#define REG_ALIGN_ERROR_COUNT 0x4004
/// Category:    Statistics
/// Permissions: R
/// SYMERRS  Symbol Error Count
#define REG_SYMBOL_ERROR_COUNT 0x4008
/// Category:    Statistics
/// Permissions: R
/// RXERRC  RX Error Count
#define REG_RX_ERROR_COUNT 0x400c
/// Category:    Statistics
/// Permissions: R
/// MPC  Missed Packet Count
#define REG_MISSED_PACKET_COUNT 0x4010
/// Category:    Statistics
/// Permissions: R
/// SCC  Single Collission Count
#define REG_SINGLE_COLLISSION_COUNT 0x4014
/// Category:    Statistics
/// Permissions: R
/// ECOL  Excessive Collissions Count
#define REG_EXCESSIVE_COLLISSIONS_COUNT 0x4018
/// Category:    Statistics
/// Permissions: R
/// MCC  Multiple Collision Count
#define REG_MULTIPLE_COLLISION_COUNT 0x401c
/// Category:    Statistics
/// Permissions: R
/// LATECOL  Late Collisions Count
#define REG_LATE_COLLISIONS_COUNT 0x4020
/// Category:    Statistics
/// Permissions: R
/// COLC  Collision Count
#define REG_COLLISION_COUNT 0x4028
/// Category:    Statistics
/// Permissions: R
/// DC  Defer Count
#define REG_DEFER_COUNT 0x4030
/// Category:    Statistics
/// Permissions: R
/// TNCRS  Transmit - No CRS
#define REG_TX_NO_CRS 0x4034
/// Category:    Statistics
/// Permissions: R
/// SEC  Sequence Error Count
#define REG_SEQUENCE_ERROR_COUNT 0x4038
/// Category:    Statistics
/// Permissions: R
/// CEXTERR  Carrier Extension Error Count
#define REG_CARRER_EXT_ERROR_COUNT 0x403c
/// Category:    Statistics
/// Permissions: R
/// RLEC  Receive Length Error Count
#define REG_RX_LENGTH_ERROR_COUNT 0x4040
/// Category:    Statistics
/// Permissions: R
/// XONRXC  XON Received Count
#define REG_XON_RECEIVED_COUNT 0x4048
/// Category:    Statistics
/// Permissions: R
/// XONTXC  XON Transmitted Count
#define REG_XON_TRANSMITTED_COUNT 0x404c
/// Category:    Statistics
/// Permissions: R
/// XOFFRXC  XOFF Received Count
#define REG_XOFF_RECEIVED_COUNT 0x4050
/// Category:    Statistics
/// Permissions: R
/// XOFFTXC  XOFF Transmitted Count
#define REG_XOFF_TRANSMITTED_COUNT 0x4054
/// Category:    Statistics
/// Permissions: R/W
/// FCRUC  FC Received Unsupported Count
#define REG_FC_RX_UNSUPPORTED_COUNT 0x4058
/// Category:    Statistics
/// Permissions: R/W
/// PRC64  Packets Received (64 Bytes) Count
#define REG_PACKETS_RX_64B_COUNT 0x405c
/// Category:    Statistics
/// Permissions: R/W
/// PRC127  Packets Received (65-127 Bytes) Count
#define REG_PACKETS_RX_127B_COUNT 0x4060
/// Category:    Statistics
/// Permissions: R/W
/// PRC255  Packets Received (128-255 Bytes) Count
#define REG_PACKETS_RX_255B_COUNT 0x4064
/// Category:    Statistics
/// Permissions: R/W
/// PRC511  Packets Received (256-511 Bytes) Count
#define REG_PACKETS_RX_511B_COUNT 0x4068
/// Category:    Statistics
/// Permissions: R/W
/// PRC1023  Packets Received (512-1023 Bytes) Count
#define REG_PACKETS_RX_1023B_COUNT 0x407c
/// Category:    Statistics
/// Permissions: R/W
/// PRC1522  Packets Received (1024-MAX Bytes) Count
#define REG_PACKETS_RX_MAX_COUNT 0x4070
/// Category:    Statistics
/// Permissions: R
/// GPRC  Good Packets Received Count
#define REG_GOOD_PACKETS_RX_COUNT 0x4074
/// Category:    Statistics
/// Permissions: R
/// BPRC  Broadcast Packets Received Count
#define REG_BROADCAST_PACKETS_RX_COUNT 0x4078
/// Category:    Statistics
/// Permissions: R
/// MPRC  Multicast Packets Received Count
#define REG_MULTICAST_PACKETS_RX_COUNT 0x407c
/// Category:    Statistics
/// Permissions: R
/// GPTC  Good Packets Transmitted Count
#define REG_GOOD_PACKETS_TX_COUNT 0x4080
/// Category:    Statistics
/// Permissions: R
/// GORCL  Good Octets Received Count (Low)
#define REG_GOOD_OCTETS_RX_COUNT_LOW 0x4088
/// Category:    Statistics
/// Permissions: R
/// GORCH  Good Octets Received Count (High)
#define REG_GOOD_OCTETS_RX_COUNT_HIGH 0x408c
/// Category:    Statistics
/// Permissions: R
/// GOTCL  Good Octets Transmitted Count (Low)
#define REG_GOOD_OCTETS_TX_COUNT_LOW 0x4090
/// Category:    Statistics
/// Permissions: R
/// GOTCH  Good Octets Transmitted Count (High)
#define REG_GOOD_OCTETS_TX_COUNT_HIGH 0x4094
/// Category:    Statistics
/// Permissions: R
/// RNBV  Receive No Buffers Count
#define REG_RX_NO_BUFFERS_COUNT 0x40a0
/// Category:    Statistics
/// Permissions: R
/// RUC  Receive Undersize Count
#define REG_RX_UNDERSIZE_COUNT 0x40a4
/// Category:    Statistics
/// Permissions: R
/// RFC  Receive Fragment Count
#define REG_RX_FRAGMENT_COUNT 0x40a8
/// Category:    Statistics
/// Permissions: R
/// ROC  Receive Oversize Count
#define REG_RX_OVERSIZE_COUNT 0x40ac
/// Category:    Statistics
/// Permissions: R
/// RJC  Receive Jabber Count
#define REG_RX_JABBER_COUNT 0x40b0
/// Category:    Statistics
/// Permissions: R
/// MGTPRC  Management Packets Received Count
/// NOTE: (not applicable to the 82544GC, 82544EI, or 82541ER)
#define REG_MANAGEMENT_PACKETS_RX_COUNT 0x40b4
/// Category:    Statistics
/// Permissions: R
/// MGTPDC  Management Packets Dropped Count
/// NOTE: (not applicable to the 82544GC, 82544EI, or 82541ER)
#define REG_MANAGEMENT_PACKETS_DROPPED_COUNT 0x40b8
/// Category:    Statistics
/// Permissions: R
/// MGTPTC  Management Packets Transmitted Count
/// NOTE: (not applicable to the 82544GC, 82544EI, or 82541ER)
#define REG_MANAGEMENT_PACKETS_TRANSMITTED_COUNT 0x40bc
/// Category:    Statistics
/// Permissions: R
/// TORL  Total Octets Received (Low)
#define REG_TOTAL_OCTETS_RX_LOW 0x40c0
/// Category:    Statistics
/// Permissions: R
/// TORH  Total Octets Received (High)
#define REG_TOTAL_OCTETS_RX_HIGH 0x40c4
/// Category:    Statistics
/// Permissions: R
/// TOTL  Total Octets Transmitted (Low)
#define REG_TOTAL_OCTETS_TX_LOW 0x40c8
/// Category:    Statistics
/// Permissions: R
/// TOTH  Total Octets Transmitted (High)
#define REG_TOTAL_OCTETS_TX_HIGH 0x40cc
/// Category:    Statistics
/// Permissions: R
/// TPR  Total Packets Received
#define REG_TOTAL_PACKETS_RX 0x40d0
/// Category:    Statistics
/// Permissions: R
/// TPT  Total Packets Transmitted
#define REG_TOTAL_PACKETS_TX 0x40d4
/// Category:    Statistics
/// Permissions: R
/// PTC64  Packets Transmitted (64 Bytes) Count
#define REG_PACKETS_TX_64B_COUNT 0x40d8
/// Category:    Statistics
/// Permissions: R
/// PTC127  Packets Transmitted (65-127 Bytes) Count
#define REG_PACKETS_TX_127B_COUNT 0x40dc
/// Category:    Statistics
/// Permissions: R
/// PTC255  Packets Transmitted (128-255 Bytes) Count
#define REG_PACKETS_TX_255B_COUNT 0x40e0
/// Category:    Statistics
/// Permissions: R
/// PTC511  Packets Transmitted (256-511 Bytes) Count
#define REG_PACKETS_TX_511B_COUNT 0x40e4
/// Category:    Statistics
/// Permissions: R
/// PTC1023  Packets Transmitted (512-1023 Bytes) Count
#define REG_PACKETS_TX_1023B_COUNT 0x40e8
/// Category:    Statistics
/// Permissions: R
/// PTC1522  Packets Transmitted (>=1024 Bytes) Count
#define REG_PACKETS_TX_MAX_COUNT 0x40ec
/// Category:    Statistics
/// Permissions: R
/// MPTC  Multicast Packets Transmitted Count
#define REG_MULTICAST_PACKETS_TX_COUNT 0x40f0
/// Category:    Statistics
/// Permissions: R
/// BPTC  Broadcast Packets Transmitted Count
#define REG_BROADCAST_PACKETS_TX_COUNT 0x40f4
/// Category:    Statistics
/// Permissions: R
/// TSCTC  TCP Segmentation Context Transmitted Count
#define REG_TCP_SEGMENTATION_CONTEXT_TX_COUNT 0x40f8
/// Category:    Statistics
/// Permissions: R
/// TSCTFC  TCP Segmentation Context Transmitted Fail Count
#define REG_TCP_SEGMENTATION_CONTEXT_TX_FAIL_COUNT 0x40fc
/// Category:    Diagnostic
/// Permissions: R/W
/// RDFH  Receive Data FIFO Head
#define REG_RX_DATA_FIFO_HEAD 0x2410
/// Category:    Diagnostic
/// Permissions: R/W
/// RDFT  Receive Data FIFO Tail
#define REG_RX_DATA_FIFO_TAIL 0x2418
/// Category:    Diagnostic
/// Permissions: R/W
/// RDFHS  Receive Data FIFO Head Saved Register
#define REG_RX_DATA_FIFO_HEAD_SAVED_REGISTER 0x2420
/// Category:    Diagnostic
/// Permissions: R/W
/// RDFTS  Receive Data FIFO Tail Saved Register
#define REG_RX_DATA_FIFO_TAIL_SAVED_REGISTER 0x2428
/// Category:    Diagnostic
/// Permissions: R/W
/// RDFPC  Receive Data FIFO Packet Count
#define REG_RX_DATA_FIFO_PACKET_COUNT 0x2430
/// Category:    Diagnostic
/// Permissions: R/W
/// TDFH  Transmit Data FIFO Head
#define REG_TX_DATA_FIFO_HEAD 0x3410
/// Category:    Diagnostic
/// Permissions: R/W
/// TDFT  Transmit Data FIFO Tail
#define REG_TX_DATA_FIFO_TAIL 0x3418
/// Category:    Diagnostic
/// Permissions: R/W
/// TDFHS  Transmit Data FIFO Head Saved Register
#define REG_TX_DATA_FIFO_HEAD_SAVED_REGISTER 0x3420
/// Category:    Diagnostic
/// Permissions: R/W
/// TDFTS  Transmit Data FIFO Tail Saved Register
#define REG_TX_DATA_FIFO_TAIL_SAVED_REGISTER 0x3428
/// Category:    Diagnostic
/// Permissions: R/W
/// TDFPC  Transmit Data FIFO Packet Count
#define REG_TX_DATA_FIFO_PACKET_COUNT 0x3430
/// Category:    Diagnostic
/// Permissions: R/W
/// PBM  Packet Buffer Memory
#define REG_PACKET_BUFFER_MEMORY_BEGIN 0x10000
#define REG_PACKET_BUFFER_MEMORY_END   0x1fffc

/// SLU == Set Link Up
#define ECTRL_SLU 0x40

/// End Of Packet
/// When set, indicates the last descriptor making up the packet. One
/// or many descriptors can be used to form a packet.
#define CMD_EOP   (1 << 0)
/// Insert FCS
/// Controls the insertion of the FCS/CRC field in normal Ethernet
/// packets. IFCS is valid only when EOP is set.
#define CMD_IFCS  (1 << 1)
/// Insert Checksum
/// When set, the Ethernet controller needs to insert a checksum at the
/// offset indicated by the CSO field. The checksum calculations are
/// performed for the entire packet starting at the byte indicated by
/// the CCS field. IC is ignored if CSO and CCS are out of the packet
/// range. This occurs when (CSS >= length) OR (CSO >= length - 1). IC
/// is valid only when EOP is set.
#define CMD_IC    (1 << 2)
/// Report Status
/// When set, the Ethernet controller needs to report the status
/// information. This ability may be used by software that does
/// in-memory checks of the transmit descriptors to determine which
/// ones are done and packets have been buffered in the transmit FIFO.
/// Software does it by looking at the descriptor status byte and
/// checking the Descriptor Done (DD) bit.
#define CMD_RS    (1 << 3)
/// Report Packet Sent
/// When set, the 82544GC/EI defers writing the DD bit in the status
/// byte (DESC.STATUS) until the packet has been sent, or transmission
/// results in an error such as excessive collisions. It is used is
/// cases where the software must know that the packet has been sent,
/// and not just loaded to the transmit FIFO. The 82544GC/ EI might
/// continue to prefetch data from descriptors logically after the one
/// with RPS set, but does not advance the descriptor head pointer or
/// write back any other descriptor until it sent the packet with the
/// RPS set. RPS is valid only when EOP is set.
/// This bit is reserved and should be programmed to 0b for all
/// Ethernet controllers except the 82544GC/EI.
#define CMD_RPS   (1 << 4)
/// VLAN Packet Enable
/// When set, indicates that the packet is a VLAN packet and the
/// Ethernet controller should add the VLAN Ethertype and an 802.1q
/// VLAN tag to the packet. The Ethertype field comes from the VET
/// register and the VLAN tag comes from the special field of the TX
/// descriptor. The hardware inserts the FCS/CRC field in that case.
/// When cleared, the Ethernet controller sends a generic Ethernet
/// packet. The IFCS controls the insertion of the FCS field in that
/// case.
/// In order to have this capability CTRL.VME bit should also be set,
/// otherwise VLE capability is ignored. VLE is valid only when EOP is
/// set.
#define CMD_VLE   (1 << 6)
/// Interrupt Delay Enable
/// When set, activates the transmit interrupt delay timer. The
/// Ethernet controller loads a countdown register when it writes back
/// a transmit descriptor that has RS and IDE set. The value loaded
/// comes from the IDV field of the Interrupt Delay (TIDV) register.
/// When the count reaches 0, a transmit interrupt occurs if transmit
/// descriptor write-back interrupts (IMS.TXDW) are enabled. Hardware
/// always loads the transmit interrupt counter whenever it processes a
/// descriptor with IDE set even if it is already counting down due to
/// a previous descriptor. If hardware encounters a descriptor that has
/// RS set, but not IDE, it generates an interrupt immediately after
/// writing back the descriptor. The interrupt delay timer is cleared.
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

/// TSTA == Transmission Status, field of transmission descriptor
///         (struct E1000TXDesc).
/// DD  Descriptor Done
/// Indicates that the descriptor is finished and is written back
/// either after the descriptor has been processed (with RS set) or for
/// the 82544GC/EI, after the packet has been transmitted on the wire
/// (with RPS set).
#define TSTA_EN (1 << 0)
/// EC  Excess Collisions
/// Indicates that the packet has experienced more than the maximum
/// excessive collisions as defined by TCTL.CT control field and was
/// not transmitted. It has no meaning while working in full duplex
/// mode.
#define TSTA_EC (1 << 1)
/// LC  Late Collision
/// Indicates that late collision occurred while working in half-duplex
/// mode. It has no meaning while working in full duplex mode. Note
/// that the collision window is speed dependent: 64 bytes for 10/100
/// Mb/s and 512 bytes for 1000 Mb/s operation.
#define TSTA_LC (1 << 2)
/// TU/RSV  Transmit Underrun/RESERVED
/// Indicates a transmit underrun event occurred. Transmit Underrun might occur if Early
/// Transmits are enabled (based on ETT.Txthreshold value) and the
/// 82544GC/EI was not able to complete the early transmission of the
/// packet due to lack of data in the packet buffer. This does not
/// necessarily mean the packet failed to be eventually transmitted.
/// The packet is successfully re-transmitted if the TCTL.NRTU bit is
/// cleared (and excessive collisions do not occur).
/// This bit is reserved and should be programmed to 0b for all
/// Ethernet controllers except the 82544GC/EI.
#define TSTA_TU (1 << 3)


/// EEPROM Address Map (16-bit word offsets)

/// The first three words (six bytes) make up the MAC Address (unique
/// address for this NIC). They are all "Used by HW".
/// Image Value IA(2,1)
#define EEPROM_ETHERNET_ADDRESS_BYTES0 0x0
/// Image Value IA(4,3)
#define EEPROM_ETHERNET_ADDRESS_BYTES1 0x1
/// Image Value IA(6,5)
#define EEPROM_ETHERNET_ADDRESS_BYTES2 0x2

/// Contains additional initialization values that:
/// - Sets defaults for some internal registers
/// - Enables/disables specific features
/// NOTE: For the 82544GC/EI, typical values are 0000h for a fiber-
/// based design and 0400h for a copperbased design
/// Bits:
///   15:12  RESERVED  For future use.
///   11     LOM  (set (default) to enable LAN On Motherboard, clear to disable)
///   10     SRV  (set (default) to enable Server card, clear to disable)
///   9      CLI  (set to enable Client card, clear (default) to disable)
///   8      OEM  (set (default) to enable OEM card, clear to disable)
///   7:6    RESERVED  Clear these bits (0)
///   5      RESERVED  Set this bit (1). For 82540EP, 82540EM, 82541xx,
///                    82547GI, and 82547EI, clear this bit (0).
///   4      SMB  SMBus (set (default) to enable SMBus, clear to disable)
///               Not applicable to 82544GC, 82544EI, or 82541ER.
///   3      RESERVED  Clear this bit (0)
///   2      BOB       PCI bridge (set this bit to enable PCI bridge, clear (default) to disable))
///   1:0    RESERVED  Clear these bits (0)
/// Image Value 0x0000
#define EEPROM_SOFTWARE_COMPATIBILITY 0x3

/// I don't really know what this does, candidly.
/// If this word has a value of other than 0xffff, software programs
/// its value into the Extended PHY Specific Control Register 2,
/// located at address 26d in the PHY register space.
/// NOTE: *Not* applicable to 82540EP, 82540EM, 82541xx, 82547GI or
/// 82547EI.
/// Used by SW
/// Image Value 0xffff
#define EEPROM_SERDES_CONFIG 0x4

/// NOTE: Applicable to 82541xx, 82547GI, and 82547EI *only*.
/// Used by SW
/// Image Value 0x0000
#define EEPROM_IMAGE_VERSION 0x5

/// NOTE: Words 0x6 and 0x7 reserved for the 82541xx, 82547GI, and
/// 82547EI.
/// Image Value 0x0000 for all three
#define EEPROM_COMPATIBILITY0 0x5
#define EEPROM_COMPATIBILITY1 0x6
#define EEPROM_COMPATIBILITY2 0x7

/// A nine-digit Printed Board Assembly (PBA) number, used for Intel
/// manufactured adapter cards, are stored in a four-byte field. Other
/// hardware manufactureers can use these fields as they wish.
/// Softwaredevice drivers should not rely on this field.
/// Long story short: A bunch of numbers that don't matter.
#define EEPROM_PBA0 0x8
#define EEPROM_PBA1 0x9

/// Used by HW
/// Image Value 0x4408
/// For 82541xx, 82547GI, and 82547EI, Image Value 0x640a
/// Bits:
///   15:14  Signature  THE MOST IMPORTANT BITS
///          When Signature != 0b01, the EEPROM is INVALID!!
///          Invalid EEPROM means no further EEPROM read is performed,
///          and the default values are used for the configuration
///          space IDs.
///   13     BAR32  When set, disables 64-bit memory mapping.
///          By default, it is cleared.
///          NOTE: Bit set by default for the 82540EP and 82540EM.
///   12     IPS0  When clear, does not invert Power State Output bit 0 (CTRL_EXT[14])
///          NOTE: Reserved (clear this bit) for 82541xx, 82547GI, and 82547EI.
///   11     FRCSPD  Force Speed bit in the Device Control reigster (CTRL[11]).
///          When clear (default), does not force speed.
///          NOTE: Bit set by default for the 82540EP and 82540EM.
///          NOTE: Bit reserved (clear this bit) for 82541xx, 82547GI, and 82547EI.
///   10     FD  Full Duplex (mapped to CTRL[0] and TXCW[5]).
///          When set (default), enables full duplex (TBI mode/internal SerDes only).
///          When clear, disables full duplex (TBI mode only/internal SerDes).
///          NOTE: Reserved bit for the 82541PI/GI/EI and 82547GI/EI (clear this bit).
///          NOTE: Reserved bit for the 82541ER (set this bit).
///   9      LRST  Link Reset (mapped to CTRL[3]).
///          When clear, enables Auto-Negotiation at power up or when asserting
///          RST# without driver intervention.
///          When set, disables Auto-Negotiation at power up or when asserting
///          RST# without driver intervention.
///          NOTE: Reserved bit for the 82541xx and 82547GI/EI (clear this bit).
///   8      IPS1  When clear (default), does not invert the Power State Output bit 1
///          (CTRL_EXT[16]).
///          When set, inverts the Power State Output invert bit 1
///          (CTRL_EXT[16]).
///          NOTE: Reserved bit for the 82541xx and 82547GI/EI (clear this bit).
///   7      Internal VREG Power Down Control
///          NOTE: 82541xx and 82547GI/EI Only
///          This bit is used to define the usage of the internal 1.2 V dc and 1.8 V dc
///          regulators to supply power.
///          0b = Yes (default).
///          1b = No (external regulators used).
///          NOTE: Reserved bit for all other Ethernet controllers.
///   6:5    RESERVED  Clear these bits.
///   4      RESERVED for copper PHY. Clear this bit.
///   3      Power Management
///          When set (default), enables full support for power management.
///          When clear, the Power Management Registers set is read only. The
///          Ethernet controller does not execute a hardware transition to D3.
///          NOTE: Reserved bit for the 82541PI/GI/EI and 82547GI/EI (set this bit).
///          NOTE: Reserved bit for the 82541ER (clear this bit).
///   2      PME Clock
///          When clear (default), indicates that the PCICLK is not required for
///          PME# output.
///          When set, indicates that the PCICLK is required for PME# output.
///          NOTE: Reserved bit for the 82541xx and 82547GI/EI (clear this bit).
///   1      Load Subsystem IDs
///          When set (default), indicates that the Ethernet controller is to load its
///          PCI Subsystem ID and Subsystem Vendor ID from the EEPROM (words 0x0b, 0x0c).
///          When clear, indicates that the Ethernet controller is to load the default
///          PCI Subsystem ID and Subsystem Vendor ID.
///   0      Load Vendor/Device IDs
///          When clear (default), indicates that the Ethernet controller is to load the
///          default values for PCI Vendor and Device IDs.
///          When set (default for the 82541xx and 82547GI/EI only),
///          indicates that the Ethernet controller is to load its PCI
///          Vendor and Device IDs from the EEPROM (words 0x0d, 0x0e).
#define EEPROM_INIT_CONTROL1 0xa

/// If the signature bits (15:14) and bit 1 (Load Subsystem IDs) of
/// word 0Ah are valid, this word is read in to initialize the Subsystem ID.
/// Used by HW
/// Image Value TODO: See table 5-1
#define EEPROM_SUBSYSTEM_ID 0xb

/// If the signature bits (15:14) and bit 1 (Load Subsystem IDs) of
/// word 0Ah are valid, this word is read in to initialize the Subsystem ID.
/// Used by HW
/// Image Value 0x8086
#define EEPROM_SUBSYSTEM_VENDOR_ID 0xc

/// If the signature bits (15:14) and bit 1 (Load Subsystem IDs) of
/// word 0Ah are valid, this word is read in to initialize the Subsystem ID.
/// For the 82546GB, the Device ID must be forced to 107Bh for SerDes-
/// SerDes interface operation.
/// For the 82545GM, the Device ID should be 1028h. This ensures proper
/// functionality with Intel drivers and boot agent.
/// NOTE: Since the 82546GB/EB is a dual-port device, the Device ID in
/// 0x0d corresponds to LAN A and the Device ID in 0x11 corresponds to LAN B.
/// Used by HW
/// Image Value TODO: See table 5-1
#define EEPROM_DEVICE_ID 0xd

/// If the signature bits (15:14) and bit 1 (Load Subsystem IDs) of word
/// 0x0a are valid, this word is read in to initialize the Subsystem ID.
/// Used by HW
/// Image Value 0x8086
#define EEPROM_VENDOR_ID 0xe


/// Used by HW
/// For 82545GM, 82545EM, 82540EP, and 82540EM, Image Value 0x3040
/// For 82541xx, 82547GI, and 82547EI, Image Value 0xb080
#define EEPROM_INIT_CONTROL2 0xf

/// These settings are specific to individual platform configurations
/// for the 82541xx and 82547GI/EI and should not be altered from the
/// reference design unless instructed to do so. Future Intel Ethernet
/// controllers might use this space differently
/// NOTE: 82541xx, 82547GI, and 82547EI only.
/// Used by SW
/// Image Value 0x00ba
#define EEPROM_PHY_REGISTERS0 0x10
/// Image Value 0x0000
#define EEPROM_PHY_REGISTERS1 0x11

/// Words 10h, 11h, and 13h through 1Fh of the EEPROM are reserved
/// areas for general OEM use for all Ethernet controllers except the
/// 82546GB/EB.
/// NOTE: Words 0x10, 0x11, and 0x13 through 0x1f are reserved for the 82545GM, 82545EM, 82540EP, and 82540EM.

/// This word is only applicable to 82541xx and 82547GI/EI Ethernet
/// controllers that use SPI EEPROMs. Unused bits are reserved and
/// should be programmed to 0. Bits 15:13 and 8:0 are reserved.
/// bits
/// 12:10  9  Size (bits)  Size (bytes)
///   000  0         1 Kb           128
///   001  1         4 Kb           512
///   010  1         8 Kb          1 KB
///   011  1        16 Kb          2 KB
///   100  1        32 Kb          4 KB
///   101  1        64 Kb          8 KB
///   110  1       128 Kb         16 KB
///   111  1     RESERVED      RESERVED
#define EEPROM_SIZE 0x12

/// For all Ethernet controllers except the 82541xx, 82547GI, and
/// 82547GIEI, if the signature bits are valid and Power Management is
/// not disabled, the value in this field is used in the PCI Power
/// Management Data Register when the Data_Select field of the Power
/// Management Control/Status Register (PMCSR) is set to 8. This
/// setting indicates the power usage and heat dissipation of the
/// common logic that is shared by both functions in tenths of a watt.
#define EEPROM_COMMON_POWER 0x12

/// NOTE: 82546GB and 82546EB use a *different* word: 0x10
/// Used by HW
#define EEPROM_SW_DEFINED_PINS_CONTROL 0x20

/// NOTE: PCI-X not applicable to the 82540EP, 82540EM, 82541xx,
/// 82547GI, and 82547EI.
/// Used by HW
/// Image Value 0x7863
/// For 82540EP and 82540EM, Image Value 0x7061
#define EEPROM_CIRCUIT_CONTROL 0x21

/// Bits 0:7 D3 Power, bits 8:15 D0 Power
/// D0:
/// If the signature bits are valid and Power Management is not
/// disabled, the value in this field is used in the PCI Power
/// Management Data Register when the Data_Select field of the Power
/// Management Control/Status Register (PMCSR) is set to 0 or 4. This
/// indicates the power usage and heat dissipation of the networking
/// function (including the Ethernet controller and any other devices
/// managed by the Ethernet controller) in tenths of a watt.
/// For example:
///      If word22h = 290E, POWER CONSUMPTION (in 1/10W, hex), then:
///      bits 15:8 = 0x29 Power in D0a, 0x29 = 4.1W
///      bits 7:0 = 0x0e Power in D3h, 0x0e = 1.4W
/// D3:
/// If the signature bits are valid and Power Management is not
/// disabled, the value in this field is used in the PCI Power
/// Management Data Register when the Data_Select field of the Power
/// Management Control/Status Register (PMCSR) is set to 3 or 7. This
/// indicates the power usage and heat dissipation of the networking
/// function (including the Ethernet controller and any other devices
/// managed by the Ethernet controller) in tenths of a watt.
/// Used by HW
/// Image Value 0x280c
/// For 82541xx, 82547GI, and 82547EI, Image Value 0x280b
#define EEPROM_POWER 0x22

/// Used by HW
#define EEPROM_MANAGEMENT_CONTROL 0x23

/// Used by HW
/// NOTE: INIT_CONTROL3 is in bits 8:15 while SMBus Address is in bits 0:7
#define EEPROM_INIT_CONTROL3 0x24
#define EEPROM_SMBUS_ADDRESS 0x24

/// Used by HW
/// Image Value IP(2,1)
#define EEPROM_IPV4_ADDRESS0 0x25
/// Used by HW
/// Image Value IP(4,3)
#define EEPROM_IPV4_ADDRESS1 0x26

/// Used by HW
/// Image Value IP(2,1)
#define EEPROM_IPV6_ADDRESS0 0x27
/// Used by HW
/// Image Value IP(4,3)
#define EEPROM_IPV6_ADDRESS1 0x28
/// Used by HW
/// Image Value IP(6,5)
#define EEPROM_IPV6_ADDRESS2 0x29
/// Used by HW
/// Image Value IP(8,7)
#define EEPROM_IPV6_ADDRESS3 0x2a
/// Used by HW
/// Image Value IP(10,9)
#define EEPROM_IPV6_ADDRESS4 0x2b
/// Used by HW
/// Image Value IP(12,11)
#define EEPROM_IPV6_ADDRESS5 0x2c
/// Used by HW
/// Image Value IP(14,13)
#define EEPROM_IPV6_ADDRESS6 0x2d
/// Used by HW
/// Image Value IP(16,15)
#define EEPROM_IPV6_ADDRESS7 0x2e

/// Used by HW
/// Image Value 0x0602
#define EEPROM_LEDCTL_DEFAULT 0x2f

#define EEPROM_FIRMWARE_BEGIN 0x30
#define EEPROM_FIRMWARE_END   0x3e

/// Checksum of words 0x00 through 0x3f
#define EEPROM_SOFTWARE_CHECKSUM 0x3f

#define EEPROM_SOFTWARE_AVAIABLE_BEGIN 0xf8
#define EEPROM_SOFTWARE_AVAIABLE_END   0xff

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

void E1000::detect_eeprom() {
    ASSERT(State >= E1000::BASE_ADDRESS_DECODED,
           "[E1000]: Base address must be decoded before detecting EEPROM!");

    /// EEPROM is explicitly missing from 82544GC and 82544EI cards.
    if (PCIHeader->Header.VendorID == 0x1107 || PCIHeader->Header.VendorID == 0x1112) {
        State = E1000::UNRECOVERABLE;
        return;
    }

    /// Otherwise, it's as simple as checking whether or not the EECD.EE_PRES bit is set.
    /// FIXME: Decide whether or not to include the actual read and success check below.
    EEPROMExists = read_command(REG_EECD) & EECD_EEPROM_PRESENT;
    State = E1000::EEPROM_PROBED;
    return;

    /// Attempt a read from the EEPROM using EERD (EEPROM Read Register).
    write_command(REG_EEPROM, 1);
    io_wait();
    /// Read the EERD register back and check if the read has completed
    /// by seeing if the EERD_DONE bit is set.
    /// We do this 1000 times in a row to make sure the hardware has
    /// time to complete the read.
    // FIXME: Use EERD_DONE_EXTRA for 82541xx, 82547GI, and 82547EI.
    for (usz i = 0; i < 1000; ++i) {
        if (read_command(REG_EEPROM) & EERD_DONE) {
            EEPROMExists = true;
            return;
        }
    }
    EEPROMExists = false;
}

u16 E1000::read_eeprom(u8 address) {
    ASSERT(State >= E1000::EEPROM_PROBED,
           "[E1000]: EEPROM must be probed before reading from it!");

    u32 calculatedAddress = 0;
    u32 successMask = 0;
    // TODO: Actually check for 82541xx, 82547GI, or 82547EI; if the
    // eeprom doesn't exist, that's another story.
    if (EEPROMExists) {
        calculatedAddress = EERD_ADDRESS(address);
        successMask = EERD_DONE;
    } else {
        calculatedAddress = EERD_ADDRESS_EXTRA(address);
        successMask = EERD_DONE_EXTRA;
    }
    // TODO: If EECD register indicates that software has direct pin
    /// control of the EEPROM, access through the EERD register can stall
    /// until that bit is clear. Software should ensure that EECD.EE_REQ
    /// and EECD.EE_GNT bits are clear before attempting to use EERD to
    /// access the EEPROM.
    /// Write address to EEPROM register along with the Start Read bit
    /// to indicate to the NIC that it needs to do an EEPROM read with
    /// given address.
    write_command(REG_EEPROM, EERD_START | calculatedAddress);

    // TODO: Maybe put a max spin so we don't hang forever in case of
    // something going wrong!
    // Read from EEPROM register until READ_DONE bit is set.
    u32 out = 0;
    do out = read_command(REG_EEPROM);
    while (!(out & successMask));

    return EERD_DATA(out);
}

void E1000::get_mac_address() {
    ASSERT(State >= E1000::EEPROM_PROBED,
           "[E1000]: EEPROM must be probed before attempting to get MAC Address!");

    if (EEPROMExists) {
        u16 value = 0;
        // FIXME: Assumes little-endian architecture.
        // Byte order of each two-byte pair is flipped.
        value = read_eeprom(EEPROM_ETHERNET_ADDRESS_BYTES0);
        MACAddress[0] = value & 0xff;
        MACAddress[1] = value >> 8;
        value = read_eeprom(EEPROM_ETHERNET_ADDRESS_BYTES1);
        MACAddress[2] = value & 0xff;
        MACAddress[3] = value >> 8;
        value = read_eeprom(EEPROM_ETHERNET_ADDRESS_BYTES2);
        MACAddress[4] = value & 0xff;
        MACAddress[5] = value >> 8;
    } else {
        // TODO: What if BARType is IO? We can probably do this same thing through BARIOAddress and 3 in32()s.
        u8* base = (u8*)(BARMemoryAddress + REG_RAL_BEGIN);
        for (uint i = 0; i < 6; ++i, ++base) MACAddress[i] = *base;
        if (!MACAddress[0] && !MACAddress[1] && !MACAddress[2] && !MACAddress[3])
            std::print("[E1000]:\033[31mERROR:\033[m First four bytes of MACAddress are zero!\n");
    }
    State = E1000::MAC_ADDRESS_DECODED;
}

void E1000::decode_base_address() {
    /// Set member function to keep track of which address space the
    /// address is in. BARType acts like the tag and BARMemoryAddress/BARIOAddress
    /// acts as the union of a tagged union.
    BARType = PCI::get_bar_type(PCIHeader->BAR0);
    if (BARType == PCI::BarType::Memory) {
        /// Remove bottom 2 bits from address.
        BARMemoryAddress = PCIHeader->BAR0 & ~usz(3);
        /// Map address in virtual page table.
        Memory::map_pages(Memory::active_page_map(),
                          (void*)BARMemoryAddress, (void*)BARMemoryAddress
                          , (u64)Memory::PageTableFlag::Present
                          | (u64)Memory::PageTableFlag::ReadWrite
                          , KiB(128) / PAGE_SIZE
                          , Memory::ShowDebug::No
                          );
        std::print("[E1000]: BAR0 is memory! addr={}\n", (void*)BARMemoryAddress);
    }
    else {
        /// Remove bottom bit from address.
        BARIOAddress = PCIHeader->BAR0 & ~usz(1);
        std::print("[E1000]: BAR0 is IO! addr={}\n", (void*)BARIOAddress);
    }
    State = E1000::BASE_ADDRESS_DECODED;
}

void E1000::configure_device() {
    /// DEVICE CONFIGURATION THROUGH CONTROL REGISTER

    /// Read Control Register
    u32 control_register = read_command(REG_CTRL);

    /// Link Reset (CTRL.LRST) should be set to 0b (normal). The
    /// Ethernet controller defaults to LRST = 1b which disables
    /// Auto-Negotiation.
    /// Bitwise AND control register with negated link reset bit to
    /// clear it (set to 0).
    control_register &= ~CTRL_LINK_RESET;

    /// PHY Reset (CTRL.PHY_RST) should be set to 0b.
    control_register &= ~CTRL_PHY_RESET;

    /// CTRL.ILOS should be set to 0b (not applicable to the
    /// 82541xx and 82547GI/EI).
    if (!is_82541xx(PCIHeader->Header.DeviceID) && !is_82547_GI_EI(PCIHeader->Header.DeviceID))
        control_register &= ~CTRL_INVERT_LOSS_OF_SIGNAL;

    /// If VLANs are not used, software should clear VLAN Mode
    /// Enable (CTRL.VME) bit.
    control_register &= ~CTRL_VLAN_MODE_ENABLE;

    /// TODO: For the 82541xx and 82547GI/EI, clear all statistical
    /// counters.

    /// Write the CTRL register with this new value.
    write_command(REG_CTRL, control_register);
}

void E1000::initialise_rx() {
    /// Program the Receive Address Register(s) (RAL/RAH) with the desired
    /// Ethernet addresses. RAL[0]/RAH[0] should always be used to store the
    /// Individual Ethernet MAC address of the Ethernet controller. This can
    /// come from the EEPROM or from any other means
    // TODO: What if BARType is IO? We can probably do this same thing through BARIOAddress and 3 in32()s.
    // FIXME: What about REG_RAH??
    u8* base = (u8*)(BARMemoryAddress + REG_RAL_BEGIN);
    for (uint i = 0; i < 6; ++i, ++base) *base = MACAddress[i];

    /// Initialize the MTA (Multicast Table Array) to 0b.
    // FIXME: I don't think this is right. REG_* are /word/ offsets, so
    // I think we need to just use it as an index into a u16 array,
    // effectively.
    for (uintptr_t mta = REG_MTA_BEGIN; mta < REG_MTA_END; mta += 1)
        *((u8*)mta) = 0;

    /// Program the Interrupt Mask Set/Read (IMS) register to enable any
    /// interrupt the software driver wants to be notified of when the event
    /// occurs. Suggested bits include RXT, RXO, RXDMT, RXSEQ, and LSC.
    write_command(REG_IMASK,
                  IMASK_RX_TIMER_INTERRUPT
                  | IMASK_RX_FIFO_OVERRUN
                  | IMASK_RX_DESC_MIN_THRESHOLD_HIT
                  | IMASK_RX_SEQUENCE_ERROR
                  | IMASK_LINK_STATUS_CHANGE
                  );

    // FIXME: Continue implementation...
}

void E1000::initialise_tx() {
    ;
}

E1000::E1000(PCI::PCIHeader0* header) : PCIHeader(header) {
    if (!PCIHeader) {
        State = E1000::UNRECOVERABLE;
        return;
    }

    decode_base_address();
    detect_eeprom();

    get_mac_address();
    std::print("[E1000]:MACAddress: ");
    for (u8 c : MACAddress)
        std::print("{:x}-", c);
    std::print("\n");

    configure_device();

    initialise_rx();
    initialise_tx();
}
