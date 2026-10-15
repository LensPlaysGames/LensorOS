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

#include <bitops.h>
#include <integers.h>

#define REGOFF_CONFIG 0x0
#define REGOFF_CONTROL 0x4
#define REGOFF_TUNE 0x20

#define CONFIG_POWER_DOWN BIT(15)
#define CONFIG_SIGNAL_ISOLATION BIT(14)
#define CONFIG_UTMI_GATE BIT(1)

#define CONTROL_SOFT_RESET BIT(0)

struct AmlogicGlue {
    uintptr_t Base;

    volatile u32* config_reg() {
        return (volatile u32*)(Base + REGOFF_CONFIG);
    }
    volatile u32* control_reg() {
        return (volatile u32*)(Base + REGOFF_CONTROL);
    }
    volatile u32* tune_reg() {
        return (volatile u32*)(Base + REGOFF_TUNE);
    }

    bool init() {
        // Disable power down and PHY signal isolation
        // Basically, physical lines (wires) are "masked" by default. We first
        // clear the "disable power" bit, such that the port may receive voltage.
        // We also clear the signal isolation bit, which allows data signals to
        // pass through (I believe).
        // Finally, we enable the internal USB clock
        {
            auto config = *config_reg();
            // clear power down bit
            // clear PHY signal isolation bit
            config &= ~(CONFIG_POWER_DOWN | CONFIG_SIGNAL_ISOLATION);

            // enable internal reference clock
            config |= CONFIG_UTMI_GATE;

            *config_reg() = config;
        }

        {
            auto control = *control_reg();
            control |= CONTROL_SOFT_RESET;
            *control_reg() = control;

            // wait 10 microseconds, at least
            gPIT.wait_polling(1);

            control &= ~CONTROL_SOFT_RESET;
            *control_reg() = control;
        }

        {
            auto control = *control_reg();

            // bits [3:2] determin amlogic role routing matrix
            // 0 -> High-Z / Disabled
            // 1 -> Force Host Mode   (connects PHY to DWC2 host blocks)
            // 2 -> Force Device Mode (connects PHY to DWC2 device blocks)
            control &= ~BITMASK32(2, 2);
            control |= (1u) << 2;

            *control_reg() = control;
        }

        // Amlogic tuning
        *tune_reg() = 0x00023a89;

        // wait five milliseconds to allow phase-locked loops to synchronize
        gPIT.wait_polling(5);

        return true;
    }
};
