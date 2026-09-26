/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>
 */

#ifndef LENSOR_OS_INTEL_HDA_REGS_H
#define LENSOR_OS_INTEL_HDA_REGS_H

#include <stdint.h>

#define HDA_REG_INSTRMPAY 0x1a /**< Input Stream Payload Capability (2 bytes) */
#define HDA_REG_INTCTL 0x20    /**< Interrupt Control (4 bytes) */
#define HDA_REG_INTSTS 0x24    /**< Interrupt Status (4 bytes) */
#define HDA_REG_WALCLK 0x30    /**< Wall Clock Counter (4 bytes) */
#define HDA_REG_SSYNC 0x38     /**< Stream Synchronization (4 bytes) */

/* CORB (Command Outbound Ring Buffer) Registers */
#define HDA_REG_CORBLBASE 0x40 /**< CORB Lower Base Address (4 bytes) */
#define HDA_REG_CORBUBASE 0x44 /**< CORB Upper Base Address (4 bytes) */
#define HDA_REG_CORBWP 0x48    /**< CORB Write Pointer (2 bytes) */
#define HDA_REG_CORBRP 0x4a    /**< CORB Read Pointer (2 bytes) */
#define HDA_REG_CORBCTL 0x4c   /**< CORB Control (1 byte) */
#define HDA_REG_CORBSTS 0x4d   /**< CORB Status (1 byte) */
#define HDA_REG_CORBSIZE 0x4e  /**< CORB Size (1 byte) */

/* RIRB (Response Inbound Ring Buffer) Registers */
#define HDA_REG_RIRBLBASE 0x50 /**< RIRB Lower Base Address (4 bytes) */
#define HDA_REG_RIRBUBASE 0x54 /**< RIRB Upper Base Address (4 bytes) */
#define HDA_REG_RIRBWP 0x58    /**< RIRB Write Pointer (2 bytes) */
#define HDA_REG_RINTCNT 0x5a   /**< Response Interrupt Count (2 bytes) */
#define HDA_REG_RIRBCTL 0x5c   /**< RIRB Control (1 byte) */
#define HDA_REG_RIRBSTS 0x5d   /**< RIRB Status (1 byte) */
#define HDA_REG_RIRBSIZE 0x5e  /**< RIRB Size (1 byte) */

/* Immediate Command Interface */
#define HDA_REG_ICOI 0x60 /**< Immediate Command Output Interface (4 bytes) */
#define HDA_REG_ICII 0x64 /**< Immediate Command Input Interface (4 bytes) */
#define HDA_REG_ICIS 0x68 /**< Immediate Command Status (2 bytes) */

/* DMA Position Buffer Registers */
#define HDA_REG_DPIBLBASE 0x70 /**< DMA Position Buffer Lower Base (4 bytes) */
#define HDA_REG_DPIBUBASE 0x74 /**< DMA Position Buffer Upper Base (4 bytes) */

/**
 * @brief Dynamic base offset helpers based on the number of streams configured
 * in the Global Capabilities Register (GCAP).
 *
 * @param _iss  Input Streams Supported count
 * @param _oss  Output Streams Supported count
 * @param _bss  Bidirectional Streams Supported count
 */
#define HDA_STREAM_BASE_INPUT 0x80
#define HDA_STREAM_BASE_OUTPUT(_iss) (0x80 + ((_iss) * 0x20))
#define HDA_STREAM_BASE_BIDIRECT(_iss, _oss) (0x80 + ((_iss) * 0x20) + ((_oss) * 0x20))

/**
 * @brief Absolute address for a specific Input Stream Descriptor (ISD)
 * @param _n    The stream index (0-based)
 * @param _reg  The HDA_SD_REG_* layout offset identifier
 */
#define HDA_REG_ISD(_n, _reg) \
    (HDA_STREAM_BASE_INPUT + ((_n) * 0x20) + (_reg))

/**
 * @brief Absolute address for a specific Output Stream Descriptor (OSD)
 * @param _iss  Total Input Streams Supported count (from GCAP)
 * @param _n    The stream index (0-based)
 * @param _reg  The HDA_SD_REG_* layout offset identifier
 */
#define HDA_REG_OSD(_iss, _n, _reg) \
    (HDA_STREAM_BASE_OUTPUT(_iss) + ((_n) * 0x20) + (_reg))

/**
 * @brief Absolute address for a specific Bidirectional Stream Descriptor (BSD)
 * @param _iss  Total Input Streams Supported count (from GCAP)
 * @param _oss  Total Output Streams Supported count (from GCAP)
 * @param _n    The stream index (0-based)
 * @param _reg  The HDA_SD_REG_* layout offset identifier
 */
#define HDA_REG_BSD(_iss, _oss, _n, _reg) \
    (HDA_STREAM_BASE_BIDIRECT(_iss, _oss) + ((_n) * 0x20) + (_reg))

/*
 * Individual stream descriptor layout offsets relative to that stream's base.
 * Every stream descriptor block is 0x20 (32) bytes wide.
 */
#define HDA_SD_REG_CTL 0x00   /**< Stream Control (3 bytes) */
#define HDA_SD_REG_STS 0x03   /**< Stream Status (1 byte) */
#define HDA_SD_REG_LPIB 0x04  /**< Link Position in Current Buffer (4 bytes) */
#define HDA_SD_REG_CBL 0x08   /**< Cyclic Buffer Length (4 bytes) */
#define HDA_SD_REG_LVI 0x0c   /**< Last Valid Index (2 bytes) */
#define HDA_SD_REG_FIFOD 0x10 /**< FIFO Data / Size (2 bytes) */
#define HDA_SD_REG_FMT 0x12   /**< Stream Format (2 bytes) */
#define HDA_SD_REG_BDPL 0x18  /**< Buffer Descriptor List Pointer - Lower (4 bytes) */
#define HDA_SD_REG_BDPU 0x1c  /**< Buffer Descriptor List Pointer - Upper (4 bytes) */

#define HDA_REG_WALCLKA 0x2030                        /**< Wall Clock Counter Alias (4 bytes) */
#define HDA_REG_SD_LPIBA(_n) (0x2084 + ((_n) * 0x20)) /**< Stream Descriptor 'n' Link Position Alias */

#endif  // LENSOR_OS_INTEL_HDA_REGS_H
