#include <e1000.h>

#include <io.h>
#include <memory/paging.h>
#include <memory/virtual_memory_manager.h>
#include <pci.h>

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
 *
 * General Configuration:
 * Several values in the Device Control Register (CTRL) need to be set upon power up or after an Ethernet controller reset for normal operation. • Speed and duplex are determined via Auto-Negotiation by the PHY, Auto-Negotiation by the MAC for internal SerDes1 mode, or forced by software if the link is forced. In internal PHY mode, the Ethernet controller can be configured automatically by hardware or forced by software to the same configuration as the PHY.
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
 *   LEDCTRL. • Link Reset (CTRL.LRST) should be set to 0b (normal). The
 *   Ethernet controller defaults to LRST = 1b which disables Auto-
 *   Negotiation. A transition to 0b initiates the Auto-Negotiation
 *   function. LRST can be defined in the EEPROM. This bit is only valid
 *   in internal SerDes mode and has no effect in internal PHY mode. 1.
 *   The 82540EP/EM, 82541xx, and 82547GI/EI do not support any SerDes
 *   functionality. 376 Software Developer’s Manual General
 *   Initialization and Reset Operation
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
 * - If VLANs are not used, software should clear VLAN Mode Enable (CTRL.
 *   VME) bit. In this instance, there is no need then to initialize the
 *   VLAN Filter Table Array (VFTA). If VLANs are desired, the VFTA
 *   should be both initialized and loaded with the desired information.
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
 *   Software should insure this memory is aligned on a paragraph
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
 */

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

/// RCTL == Recieve Control Register
/// Bits:
///   31:27  RESERVED  (clear these bits)
///   26     SECRC  Strip Ethernet CRC from incoming packet when set (default 0)
///   25     BSEX  Buffer Size Extension (default 0)
///          When set, the original BSIZE values are multiplied by 16.
///   24     RESERVED  (clear this bit)
///   23     PMCF  Pass MAC Control Frames (default 0)
///          0b = Do not (specially) pass MAC control frames.
///          1b = Pass any MAC control frame
///          (type field value of 8808h) that does not contain the
///          pause opcode of 0001h.
///          PMCF controls the DMA function of MAC control frames
///          (other than flow control). A MAC control frame in this
///          context must be addressed to either the MAC control frame
///          multicast address or the station address, match the type
///          field and NOT match the PAUSE opcode of 0001h. If PMCF =
///          1b then frames meeting this criteria are transferred to
///          host memory. Otherwise, they are filtered out.
///   22     DPF  Discard Pause Frames (default 0)
///   21     RESERVED  (clear this bit)
///   20     CFI  Canonical Form Indicator bit value (default 0)
///          If RCTL.CFIEN is set, then 802.1Q packets with CFI equal
///          to this field is accepted; otherwise, the 802.1Q packet is
///          discarded.
///   19     CFIEN Canonical Form Indicator Enable (default 0)
///          0b = Disabled (CFI bit found in received 802.1Q packet’s
///          tag is not compared to decide packet acceptance).
///          1b = Enabled (CFI bit found in received 802.1Q packet’s
///          tag must match RCTL.CFI to accept 802.1Q type packet).
///   18     VFE  VLAN Filter Enable
///          0b = Disabled (filter table does not decide packet acceptance).
///          1b = Enabled (filter table decides packet acceptance for 802.1Q packets).
///          Three bits control the VLAN filter table. RCTL.VFE
///          determines whether the VLAN filter table participates in
///          the packet acceptance criteria. RCTL.CFIEN and RCTL.CFI
///          are used to decide whether the CFI bit found in the 802.1Q
///          packet’s tag should be used as part of the acceptance
///          criteria.
///          NOTE: Not applicable to the 82541ER.
///   17:16  BSIZE  Recieve Buffer Size (default 0)
///          Controls the size of the receive buffers, allowing the
///          software to trade off between system performance and
///          storage space. Small buffers maximize memory efficiency at
///          the cost of multiple descriptors for bigger packets.
///          RCTL.BSEX = 0b:
///          00b = 2048 Bytes.
///          01b = 1024 Bytes.
///          10b = 512 Bytes.
///          11b = 256 Bytes. NOTE: Corrected 1b1 -> 11b
///          RCTL.BSEX = 1b:
///          00b = Reserved; software should not program this value.
///          01b = 16384 Bytes.
///          10b = 8192 Bytes.
///          11b = 4096 Bytes.
///   15     BAM  Broadcast Accept Mode (default 0)
///          0 = ignore broadcast; 1 = accept broadcast packets.
///          When set, passes and does not filter out all received
///          broadcast packets. Otherwise, the Ethernet controller
///          accepts, or rejects a broadcast packet only if it matches
///          through perfect or imperfect filters.
///   14     RESERVED (clear this bit)
///   13:12  MO  Multicast Offset (default 0)
///          The Ethernet controller is capable of filtering multicast
///          packets based on 4096-bit vector multicast filtering
///          table. The MO determines which bits of the incoming
///          multicast address are used in looking up the 4096-bit
///          vector.
///          00b = bits [47:36] of received destination multicast address.
///          01b = bits [46:35] of received destination multicast address.
///          10b = bits [45:34] of received destination multicast address.
///          11b = bits [43:32] of received destination multicast address.
///   11:10  RESERVED (clear these bits)
///   9:8    RDMTS  Recieve Descriptor Minimum Threshold Size (deafult 0)
///          The corresponding interrupt ICR.RXDMT0 is set each time
///          the fractional number of free descriptors becomes equal to
///          RDMTS. The following table lists which fractional values
///          correspond to RDMTS values. The size of the total receiver
///          circular descriptor buffer is set by RDLEN. See Section
///          13.4.27 for details regarding RDLEN.
///          00b = Free Buffer threshold is set to 1/2 of RDLEN.
///          01b = Free Buffer threshold is set to 1/4 of RDLEN.
///          10b = Free Buffer threshold is set to 1/8 of RDLEN.
///          11b = Reserved.
///   7:6    LBM  Loopback mode.
///          Controls the loopback mode of the Ethernet controller.
///          00b = No loopback.
///          01b = Undefined.
///          10b = Undefined.
///          11b = PHY or external SerDes loopback.
///          All loopback modes are only allowed when the Ethernet
///          controller is configured for full-duplex operation.
///          Receive data from transmit data looped back internally to
///          the SerDes or internal PHY. In TBI mode (82544GC/EI), the
///          EWRAP signal is asserted.
///          NOTE: The 82540EP/EM, 82541xx, and 82547GI/EI do not
///          support SerDes functionality.
///   5      LPE  Long Packet Reception Enable
///          0b = Disabled.
///          1b = Enabled.
///          LPE controls whether long packet reception is permitted.
///          When LPE is cleared, the Ethernet controller discards
///          packets longer than 1522 bytes. When LPE is set, the
///          Ethernet controller discards packets that are longer than
///          16384 bytes.
///          For the 82541xx and 82547GI/EI, packets larger than 2 KB
///          require full duplex operation.
///   4      MPE  Multicast Promiscuous Enabled
///          0b = Disabled.
///          1b = Enabled.
///          When set, passes without filtering out all received
///          multicast packets. Otherwise, the Ethernet controller
///          accepts or rejects a multicast packet based on its 4096-
///          bit vector multicast filtering table.
///   3      UPE  Unicast Promiscuous Enabled
///          0b = Disabled.
///          1b = Enabled.
///          When set, passes without filtering out all received
///          unicast packets. Otherwise, the Ethernet controller
///          accepts or rejects unicast packets based on the received
///          packet destination address match with 1 of the 16 stored
///          addresses.
///   2      SBP  Store Bad Packets (default 0)
///          0b = do not store.
///          1b = store bad packets.
///          When set, the Ethernet controller stores bad packets (CRC
///          error, symbol error, sequence error, length error,
///          alignment error, short packets or where carrier extension
///          or RX_ERR errors) that pass the filter function in host
///          memory. When the Ethernet controller is in promiscuous
///          mode, and SBP is set, it might possibly store all packets.
///   1      EN  Receiver Enable (default 0)
///          The receiver is enabled when this bit is 1b. Writing this
///          bit to 0b stops reception after receipt of any in-progress
///          packets. Data remains in the receive FIFO until the device
///          is re–enabled. Disabling or re-enabling the receiver does
///          not reinitialize the packet filter logic that demarcates
///          packet start and end locations in the FIFO; Therefore the
///          receiver must be reset before re-enabling it.
///   0      RESERVED (clear this bit)

/// RCTL_* == bits within register
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
/// not transmitted. It has no meaning while working in full-duplex
/// mode.
#define TSTA_EC (1 << 1)
/// LC  Late Collision
/// Indicates that late collision occurred while working in half-duplex
/// mode. It has no meaning while working in full-duplex mode. Note
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
    /// EEPROM Read Register
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
    static constexpr u32 START_READ = (1 << 0);
    static constexpr u32 READ_DONE  = (1 << 4);
    static constexpr auto READ_ADDRESS = [](u8 address) { return u32(address) << 8; };
    static constexpr auto READ_DATA = [](u32 eepromRegister) { return u16(eepromRegister >> 16); };
    u32 calculatedAddress = 0;
    u32 successMask = 0;
    // FIXME: I'm not sure if this condition is correct or not. I at
    // least haven't found a basis for the else branch in the 82540EM
    // manual (yet). A better check would be specifically the 82544GC
    // or 82544EI, I think? Or just disallow those cards altogether.
    if (EEPROMExists) {
        calculatedAddress = READ_ADDRESS(address);
        successMask = READ_DONE;
    } else {
        calculatedAddress = u32(address) << 2;
        successMask = 1 << 1;
    }
    // Write address to EEPROM register along with the Start Read bit
    // to indicate to the NIC that it needs to do an EEPROM read with
    // given address.
    write_command(REG_EEPROM, START_READ | calculatedAddress);

    // TODO: Maybe put a max spin so we don't hang forever in case of
    // something going wrong!
    // Read from EEPROM register until READ_DONE bit is set.
    u32 out = 0;
    do out = read_command(REG_EEPROM);
    while (!(out & successMask));

    return READ_DATA(out);
}

void E1000::get_mac_address() {
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
