#ifndef LENSOR_OS_ACPI_H
#define LENSOR_OS_ACPI_H

/* Advanced Configuration and Power Interface
 *   ACPI Spec 6.4 Table 5.5 DESCRIPTION_HEADER Signatures for tables
 *   |- https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html?highlight=system%20descript#description-header-signatures-for-tables-defined-by-acpi
 *   |- "APIC"  -- Multiple APIC Description Table (MADT)
 *   |- "BGRT"  -- Boot Graphics Resource Table (BGRT; UEFI-only)
 *   |- "BERT"  -- Boot Error Record Table (BERT)
 *   |- "CPEP"  -- Corrected Platform Error Polling Table (CPEP)
 *   |- "DSDT"  -- Differentiated System Descriptor Table (DSDT)
 *   |- "ECDT"  -- Embedded Controller Boot Resources Table (ECDT)
 *   |- "EINJ"  -- Error Injection Table (EINJ)
 *   |- "ERST"  -- Error Record Serialization Table (ERST)
 *   |- "FACP"  -- Fixed ACPI Description Table (FADT)
 *   |- "FACS"  -- Fixed ACPPI Control Structure (FACS)
 *   |- "FPDT"  -- Firmware Performance Data Table (FPDT)
 *   |- "GTDT"  -- Generic Timer Description Table (GTDT)
 *   |- "HEST"  -- Hardware Error Source Table (HEST)
 *   |- "MSCT"  -- Maximum System Characteristics Table (MSCT)
 *   |- "MPST"  -- Memory Power State Table (MPST)
 *   |- "NFIT"  -- NVDIMM Firmware Interface Table
 *   |- "OEMx"  -- OEM Specific Information Tables (x = anything)
 *   |- "PCCT"  -- Platform Communications Channel Table (PCCT)
 *   |- "PHAT"  -- Platform Health Assessment Table
 *   |- "PMTT"  -- Platform Memory Topology Table (PMTT)
 *   |- "PSDT"  -- Persistent System Descriptor Table (PSDT)
 *   |- "RASF"  -- ACPI RAS FeatureTable (RASF)
 *   |- "RSDT"  -- Root System Descriptor Table (RSDT)
 *   |- "SBST"  -- Smart Battery Table (SBST)
 *   |- "SDEV"  -- Secure DEVices Table (SDEV)
 *   |- "SLIT"  -- System Locality System Information Table (SLIT)
 *   |- "SRAT"  -- System Resource Affinity Table (SRAT)
 *   |- "SSDT"  -- Secondary System Descriptor Table (SSDT)
 *   `- "XSDT"  -- Extended System Descriptor Table (XSDT)
 *
 *   ACPI Spec 6.4 Table 5.6 DESCRIPTION_HEADER Signatures for reserved tables
 *   |- https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html?highlight=system%20descript#description-header-signatures-for-tables-reserved-by-acpi
 *   |- "AEST"  -- Arm Error Source Table
 *   |- "BDAT"  -- BIOS Data ACPI Table
 *   |- "BOOT"  -- Reserved Signature
 *   |- "CDIT"  -- Component Distance Information Table
 *   |- "CEDT"  -- CXL Early Discovery Table
 *   |- "CRAT"  -- Component Resource Attribute Table
 *   |- "CSRT"  -- Core System Resource Table
 *   |- "DBGP"  -- Debug Port Table
 *   |- "DBGP2" -- Debug Port Table 2
 *   |- "DMAR"  -- DMA Remapping Table
 *   |- "DRTM"  -- Dynamic Root of Trust for Measurement Table
 *   |- "ETDT"  -- Event Timer Description Table (obsolete)
 *   |- "HPET"  -- High Precision Event Timer Table
 *   |- "IBFT"  -- iSCSI Boot Firmware Table
 *   |- "IORT"  -- I/O Remapping Table
 *   |- "IVRS"  -- I/O Virtualization Reporting Structure
 *   |- "LPIT"  -- Low Power Idle Table
 *   |- "MCFG"  -- Memory-mapped ConFiguration Space Base Address Description Table
 *   |- "MCHI"  -- Management Controller Host Interface Table
 *   |- "MPAM"  -- Memory Partitioning and Monitoring
 *   |- "MSDM"  -- Microsoft Data Management Table
 *   |- "PRMT"  -- Platform Runtime Mechanism Table
 *   |- "RGRT"  -- Regulatory Graphics Resource Table
 *   |- "SDEI"  -- Software Delegated Exceptions Interface
 *   |- "SLIC"  -- Microsoft Software LICensing Table
 *   |- "SPCR"  -- Microsoft Serial Port Console Redirection Table
 *   |- "SPMI"  -- Server Platform Managment Interface Table
 *   |- "STAO"  -- _STA Override Table
 *   |- "SVKL"  -- Storage Volume Key Data
 *   |- "TCPA"  -- Trusted Computing Platform Alliance Capabilities Table
 *   |- "TPM2"  -- Trusted Platform Module 2
 *   |- "UEFI"  -- Unified Extensibe Firmware Interface Specification
 *   |- "WAET"  -- Windows ACPI Emulated Devices Table
 *   |- "WDAT"  -- Watchdog Action Table
 *   |- "WDRT"  -- Watchdog Resource Table
 *   |- "WPBT"  -- Windows Platform Binary Table
 *   |- "WSMT"  -- Windows Security Mitigations Table
 *   |- "XENV"  -- Xen Project
 */

#include "integers.h"

namespace ACPI {
    /* Root System Descriptor Pointer
     *   36 BYTES
     *
     *   https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html?highlight=system%20descript#rsdp-structure
     */ 
    struct RSDP2 {
        unsigned char Signature[8];
        u8  Checksum;
        u8  OEMId[6];
        u8  Revision;
        u32 RSDTAddress;
        u32 Length;
        u64 XSDTAddress;
        u8  ExtendedChecksum;
        // Ignored in reading, must not be written.
        u8  Reserved[3];
    } __attribute__((packed));

    /* System Descriptor Table Header
     *   36 BYTES
     *
     *   https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html?highlight=system%20descript#description-header-fields
     */
    struct SDTHeader {
        unsigned char Signature[4];
        u32 Length;
        u8  Revision;
        u8  Checksum;
        u8  OEMID[6];
        u8  OEMTableID[8];
        u32 OEMRevision;
        u32 CreatorID;
        u32 CreatorRevision;
    } __attribute__((packed));

    /* Memory-mapped ConFiguration Space Header
     *   44 BYTES
     *
     *   https://wiki.osdev.org/PCI_Express 
     */
    struct MCFGHeader : public SDTHeader{
        u64 Reserved;
    } __attribute__((packed));

    /* Generic Address Structure Format
     *   12 BYTES
     *
     *   https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html?highlight=system%20descript#generic-address-structure-gas
     */
    struct GenericAddressStructure {
        /* ID of address space format
         *   0x00  -- System Memory space
         *   0x01  -- System I/O space
         *   0x02  -- PCI Configuration Space
         *   0x03  -- Embedded Controller
         *   0x04  -- SMBus
         *   0x05  -- SystemCMOS
         *   0x06  -- PciBarTarget
         *   0x07  -- IPMI
         *   0x08  -- General PurposeIO
         *   0x09  -- GenericSerialBus
         *   0x0a  -- Platform Communications Channel (PCC)
         *   0x0b thru 0x7e  -- Reserved
         *   0x7f  -- Functional Fixed Hardware
         *   0x80 thru 0xbf  -- Reserved
         *   0xc0 thru oxff  -- OEM defined
         *
         *   System Memory: 64-bit physical memory address.
         *   System I/O: 64-bit I/O address.
         *   PCI Configuration Space:
         *     I can't figure out what they are trying to say in the spec.
         *   PciBarTarget:
         *     Bits 63:56  -- PCI Segment
         *     Bits 55:48  -- PCI Bus
         *     Bits 47:43  -- PCI Device
         *     Bits 42:40  -- PCI Function
         *     Bits 39:37  -- BAR index
         *     Bits 36:0   -- Offset from BAR in Double Words.
         */
        u8  AddressSpaceID;
        /* Size in bits of the given register.
         * When addressing a data structure, this must be zero.
         */
        u8  RegisterBitWidth;
        /* Bit offset of the given register at the given address.
         * When addressing a data structure, this must be zero.
         */
        u8  RegisterBitOffset;
        /* 0  -- Undefined
         * 1  -- Byte access (8-bit)
         * 2  -- Word access (16-bit)
         * 3  -- Double Word access (32-bit)
         * 4  -- Quad Word access (64-bit)
         */
        u8  AccessSize;
        /* 64-bit address of data structure or register in given address space. 
         *   Format determined by AddressSpaceID above. 
         */
        u64 Address;
    } __attribute__((packed));


    /* High Precision Event Timer Table 
     *   56 BYTES
     *   https://github.com/freebsd/freebsd-src/blob/97c0b5ab18b6131ab11ed03b38d5e239fc811a3e/sys/contrib/dev/acpica/include/actbl1.h#L2010
     *   https://wiki.osdev.org/HPET
     */
    struct HPETTable : public SDTHeader{
        u8  RevisionID;
        u8  ID;
        u16 PCIvendorID;
        GenericAddressStructure Address;
        u8  Number;
        u16 MinimumTick;
        u8  PageProtection;
    } __attribute__((packed));

    /* HPET Identifier sub-fields */
#define ACPI_HPET_ID_COMPARATORS      0b00011111
#define ACPI_HPET_ID_COUNT_SIZE_CAP   0b00100000
#define ACPI_HPET_ID_LEGACY_CAPABLE   0b10000000

    /* HPET Flags masks */
#define ACPI_HPET_FLAGS_PAGE_PROTECT_MASK (0b11)

    enum class HPETPageProtect {
        NO_PAGE_PROTECT  = 0,
        PAGE_PROTECT4    = 1,
        PAGE_PROTECT64   = 2,
    };

    /* Fixed ACPI Description Table
     *   276 BYTES
     *   https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/05_ACPI_Software_Programming_Model/ACPI_Software_Programming_Model.html#fixed-acpi-description-table-fadt 
     */
    struct FADTHeader : public SDTHeader {
        u32 FIRMWARE_CTRL;
        /* Physical memory address of Differentiated System Descriptor Table */
        u32 DSDT;
        /* ACPI 1.0 INT_MODEL, eliminated as of ACPI 2.0 */
        u8  Reserved0;
        /* Preferred Power Management Profile (set by OEM) 
         *   Values:
         *   |- 0  -- Unspecified
         *   |- 1  -- Desktop
         *   |- 2  -- Mobile
         *   |- 3  -- Workstation
         *   |- 4  -- Enterprise Server
         *   |- 5  -- SOHO Server
         *   |- 6  -- Appliance PC
         *   |- 7  -- Performance Server
         *   |- 8  -- Tablet
         *   `- >8 -- Reserved
         */
        u8  Preferred_PM_Profile;
        /* System vector the SCI interrupt is wired to in 8259 mode. 
         * If system not in 8259-mode, contains Global System 
         *   interrupt number of the SCI interrupt. 
         */
        u16 SCI_INT;
        /* System port address of the SMI Command Port. */
        u32 SMI_CMD;
        /* The value to write to SMI_CMD to disable SMI ownership of ACPI hardware registers. */
        u8 ACPI_ENABLE;
        /* The value to write to SMI_CMD to re-enable SMI ownership of ACPI hardware registers. */
        u8 ACPI_DISABLE;
        /* The value to write to SMI_CMD to enter the S4BIOS state. */
        u8 S4BIOS_REQ;
        /* If non-zero, contains value to write to SMI_CMD register to assume
         *   processor performance state control responsibility. 
         */
        u8 PSTATE_CNT;
        /* System port address of the PM1a Event Register Block. 
         *   REQUIRED
         */
        u32 PM1a_EVT_BLK;
        /* System port address of the PM1b Event Register Block.
         *   Contains zero if not supported.
         */
        u32 PM1b_EVT_BLK;
        /* System port address of the PM1a Control Register Block. 
         *   REQUIRED
         */
        u32 PM1a_CNT_BLK;
        /* System port address of the PM1b Control Register Block.
         *   Contains zero if not supported.
         */
        u32 PM1b_CNT_BLK;
        /* System port address of the PM2 Control Register Block. 
         *   Contains zero if not supported. 
         */
        u32 PM2_CNT_BLK;
        /* System port address of the Power Management Timer Control Register Block. 
         *   Contains zero if not supported.
         */
        u32 PM_TMR_BLK;
        /* System port address of the General Purpose Event 0 Register Block. 
         *   Contains zero if not supported.
         */
        u32 GPE0_BLK;
        /* System port address of the General Purpose Event 1 Register Block. 
         *   Contains zero if not supported.
         */
        u32 GPE1_BLK;
        /* Number of bytes decoded by PM1a_EVT_BLK and PM1b_EVT_BLK. 
         *   MUST BE GREATER THAN OR EQUAL TO FOUR.
         */
        u8 PM1_EVT_LEN;
        /* Number of bytes decoded by PM1a_CNT_BLK and PM1b_CNT_BLK. 
         *   MUST BE GREATER THAN OR EQUAL TO TWO.
         */
        u8 PM1_CNT_LEN;
        /* Number of bytes decoded by PM2_CNT_BLK. 
         *   MUST BE GREATER THAN OR EQUAL TO ONE IF PM2_CNT_BLK IS SUPPORTED.
         *   Contains zero if not supported.
         */
        u8 PM2_CNT_LEN;
        /* Number of bytes decoded by PM_TMR_BLK.
         *   MUST BE EQUAL TO FOUR IF PM_TMR_BLK IS SUPPORTED.
         *   Contains zero if not supported.
         */
        u8 PM_TMR_LEN;
        /* Number of bytes decoded by GPEO_BLK.
         *   MUST BE NON-NEGATIVE MULTIPLE OF TWO. 
         */
        u8 GPEO_BLK_LEN;
        /* Number of bytes decoded by GPE1_BLK.
         *   MUST BE NON-NEGATIVE MULTIPLE OF TWO. 
         */
        u8 GPE1_BLK_LEN;
        /* Offset within the ACPI general-purpose event 
         *   model where GPE1 based events start. 
         */
        u8 GPE1_BASE;
        /* If non-zero, contains value to write to SMI_CMD to indicate OS 
         *   support for the _CST object and C States Changed notification. 
         */
        u8 CST_CNT;
        /* Worst case hardware latency in microseconds to enter and exit a C2 satte.
         *   If greater than 100, system does not support C2 state. 
         */
        u16 P_LVL2_LAT;
        /* Worst case hardware latency in microseconds to enter and exit a C3 satte.
         *   If greater than 1000, system does not support C3 state. 
         */
        u16 P_LVL3_LAT;
        /* if WBINVD is equal to zero, contains the number 
         *   of flush strides that need to be read to completely 
         *   flush dirty lines from any processor's memory caches.
         * If not supported, contains zero along with WBINVD.
         */
        u16 FLUSH_SIZE;
        /* if WBINVD is equal to zero, contains the cache line 
         *   width in bytes of the processor's memory caches.
         */
        u16 FLUSH_STRIDE;
        /* Zero-based index of where the processor's duty cycle 
         *   setting is within the processor's P_CNT register. 
         */
        u8 DUTY_OFFSET;
        /* The bit width o fthe processor's duty cycle 
         *   setting value in the P_CNT register. 
         */
        u8 DUTY_WIDTH;
        /* RTC CMOS RAM index to the day-of-month alarm value.
         *   If zero, RTC does not support the day-of-month alarm.
         *   If non-zero, contains index into RTC RAM space that can
         *     be used to program the day-of-month alarm.
         */
        u8 DAY_ALRM;
        /* RTC CMOS RAM index to the month-of-year alarm value.
         *   If zero, RTC does not support the month-of-year alarm.
         *   If non-zero, contains index into RTC RAM space that can
         *     be used to program the month-of-year alarm.
         *   If this feature is supported, DAY_ALRM must also be supported.
         */
        u8 MON_ALRM;
        /* RTC CMOS RAM index to the century data value.
         *   If zero, RTC centenary feature is not supported.
         *   If non-zero, contains index into RTC RAM space that can
         *     be used to program the centenary field.
         */
        u8 CENTURY;
        /* IA-PC Boot Architecture Flags 
         *   0b0000000000000000
         *                    =  LEGACY_DEVICES 
         *                   =   8042
         *                  =    VGA Not Present
         *                 =     MSI Not Supported
         *                =      PCIe ASPM Controls
         *               =       CMOS RTC Not Present
         *     ==========        Reserved
         *  
         * LEGACY_DEVICES: If set, drivers are needed to parse system hardware.
         * 8042: If set, indicates that the motherboard contains support 
         *   for a port 60 and 64 based keyboard controller.
         * VGA Not Present: If set, must not blindly probe VGA hardware.
         * MSI Not Supported: If set, Message Signaled Interrupts must not be enabled.
         * PCIe ASPM Controls: If set, must not enable OSPM ASPM control on this platform.
         * CMOS RTC Not Present: If set, RTC is not implemented or doesn't exist at legacy addresses.
         * Reserved: Must be cleared (set to `0`).
         */
        u16 IAPC_BOOT_ARCH;
        u8 Reserved1;
        /* Fixed Feature Flags 
         *   0b00000000000000000000000000000000
         *                                    =  WBINVD
         *                                   =   WBINVD_FLUSH
         *                                  =    PROC_C1
         *                                 =     P_LVL2_UP
         *                                =      PWR_BUTTON
         *                               =       SLP_BUTTON
         *                              =        FIX_RTC
         *                             =         RTC_S4
         *                            =          TMR_VAL_EXT
         *                           =           DCK_CAP
         *                          =            RESET_REG_SUP
         *                         =             SEALED_CASE
         *                        =              HEADLESS
         *                       =               CPU_SW_SLP
         *                      =                PCI_EXP_WAK
         *                     =                 USE_PLATFORM_CLOCK
         *                    =                  S4_RTC_STS_VALID
         *                   =                   REMOTE_POWER_ON_CAPABLE
         *                  =                    FORCE_APIC_CLUSTER_MODEL
         *                 =                     FORCE_APIC_PHYSICAL_DESTINATION_MODE
         *                =                      HW_REDUCED_ACPI
         *               =                       LOW_POWER_S0_IDLE_CAPABLE
         *     ==========                        Reserved
         *
         * WBINVD: If set, signifies the processor implements `WBINVD` IA-32 instruction,
         *           and does so while correctly flushing all processor caches, maintaining
         *           memory coherency, and upon completion, all caches for the current processor
         *           contain no cached data other than what OSPM references and allows to be cached.
         * WBINVD_FLUSH: If set, hardware flushes all caches on `WBINVD` instruction,
         *   maintains memory coherency, but does not guarantee the caches are invalidated.
         * PROC_C1: If set, C1 power state is supported on all processors.
         * P_LVL2_UP: 
         *   If zero, C2 power state only works on a uniprocessor system.
         *   If one, C2 power state works with single or multi-processor systems.          
         * PWR_BUTTON: If one, system has no power button.
         * SLP_BUTTON: If one, system has no sleep button.
         * FIX_RTC: If zero, RTC wake status is supported in fixed register space.
         * RTC_S4: If set, RTC alarm function may wake system from the S4 state.
         * TMR_VAL_EXT: If zero, timer value is 24-bit; if one: 32-bit.
         * DCK_CAP: If zero, system does not support docking.
         * RESET_REG_SUP: If set, system supports reset via the FADT RESET_REG.
         * SEALED_CASE: If set, system has no internal expansion capabilities (case is sealed).
         * HEADLESS: If set, indicated system can not detect monitor or keyboard/mouse devices.
         * CPU_SW_SLP: If set, processor native instruction must be executed
         *   after writing to the SLP_TYPx register.
         * PCI_EXP_WAK: If set, platform supports PCIEXP_WAKE_STS bit in the PM1 Status register,
         *   and the PCIEXP_WAKE_EN bit in the PM1 Enable register.
         * USE_PLATFORM_CLOCK: 
         *   If set, use platform provided timer for monotonically non-decreasing counters.
         * S4_RTC_STS_VALID: 
         *   If set, contents of the RTC_STS flag is valid when waking the system from S4.
         * REMOTE_POWER_ON_CAPABLE: If set, patform is compatible with remote power-on.
         * FORCE_APIC_CLUSTER_MODEL: If set, all local APICs must be configured for the
         *   cluster destination model when delivering interrupts in logical mode.
         * FORCE_APIC_PHYSICAL_DESTINATION_MODE: If set, all local xAPICs must be 
         *   configured for phsyical destination mode.
         *   On machine with less than eight local xAPICs, this bit is ignored.
         * HW_REDUCED_ACPI: If set, hardware-reduced ACPI is implemented,
         *   therefore software-only alternatives are used for supported fixed-features.
         * LOW_POWER_S0_IDLE_CAPABLE: If set, platform is able to achieve power savings in 
         *   S0 similar to or better than those achieved in S3. (If set, don't transition to S3).
         * Reserved: Must be set to zero.
         */
        u32 Flags;
        /* The address of the reset register */
        GenericAddressStructure RESET_REG;
        /* The value to write to the RESET_REG port to reset the system. */
        u8 RESET_VALUE;
        /* ARM Boot Architecture Flags 
         *   0b0000000000000000
         *                    =  PSCI_COMPLIANT
         *                   =   PSCI_USE_HVC
         *     ==============    Reserved
         *
         * PSCI_COMPLIANT: If set, PSCI is implemented.
         * PSCI_USE_HVC: If set, HVC must be used as the PSCI conduit instead of SMC.
         * Reserved: Must be set to zero.
         */
        u16 ARM_BOOT_ARCH;
        /* Minor version of this FADT structure in "Major.Minor" form.
         *   Bits 0-3: Minor version of ACPI Spec.
         *   Bits 4-7: Version of ACPI Spec errata this table complies with.
         * Major version can be found in Header.Revision
         */
        u8 FADT_MINOR_VERSION;
        /* Extended physical addrress of the FACS.
         *   If non-zero, FIRMWARE_CTRL must be ignored.
         *   If HW_REDUCED_ACPI field is set in Flags and both this field
         *     and FIRMWARE_CTRL are zero, there is no FACS available. 
         */
        u64 X_FIRMWARE_CTRL;
        /* Extended physical address of the DSDT.
         *   If non-zero, DSDT must be ignored 
         */
        u64 X_DSDT;
        /* Extended address of PM1a Event Register Block.
         *   If non-zero, PM1a_EVT_BLK must be ignored.
         */
        GenericAddressStructure X_PM1a_EVT_BLK;
        /* Extended address of PM1b Event Register Block.
         *   If zero, not supported.
         *   If non-zero, PM1b_EVT_BLK must be ignored.
         */
        GenericAddressStructure X_PM1b_EVT_BLK;
        /* Extended address of PM1a Control Register Block.
         *   If non-zero, PM1a_CNT_BLK must be ignored.
         */
        GenericAddressStructure X_PM1a_CNT_BLK;
        /* Extended address of PM1b Control Register Block.
         *   If zero, not supported.
         *   If non-zero, PM1b_CNT_BLK must be ignored.
         */
        GenericAddressStructure X_PM1b_CNT_BLK;
        /* Extended address of PM2 Control Register Block.
         *   If zero, not supported.
         *   If non-zero, PM2_CNT_BLK must be ignored.
         */
        GenericAddressStructure X_PM2_CNT_BLK;
        /* Extended address of Power Management Timer Control Register Block.
         *   If zero, not supported.
         *   If non-zero, PM_TMR_BLK must be ignored.
         */
        GenericAddressStructure X_PM_TMR_BLK;
        /* Extended address of General Purpose Event 0 Register Block.
         *   If zero, not supported.
         *   If non-zero, GPE0_BLK must be ignored.
         */
        GenericAddressStructure X_GPE0_BLK;
        /* Extended address of General Purpose Event 1 Register Block.
         *   If zero, not supported.
         *   If non-zero, GPE1_BLK must be ignored.
         */
        GenericAddressStructure X_GPE1_BLK;
        /* Address of the Sleep control register. */
        GenericAddressStructure SLEEP_CONTROL_REG;
        /* Address of the Sleep status register. */
        GenericAddressStructure SLEEP_STATUS_REG;
        /* 64-bit identifier of hypervisor vendor. */
        u64 HypervisorVendorID;
    } __attribute__((packed));

    // 16 BYTES
    struct DeviceConfig {
        u64 BaseAddress;
        u16 PCISegmentGroup;
        u8  StartBus;
        u8  EndBus;
        u32 Reserved;
    } __attribute__((packed));

    void* find_table(SDTHeader* sdt, char* signature);
}

#endif
