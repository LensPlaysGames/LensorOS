#include <gpt.h>

#include <ahci.h>
#include <smart_pointer.h>
#include <uart.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_GPT

namespace GPT {
    constexpr const char* HEADER_SIGNATURE = "EFI PART";
    bool is_gpt_present(StorageDeviceDriver* driver) {
#ifdef DEBUG_GPT
        UART::out("[GPT]: Checking for valid GPT\r\n");
#endif /* DEBUG_GPT */
        if (driver == nullptr) {
            UART::out("  Driver is nullptr\r\n");
            return false;
        }

        SmartPtr<Header> hdr = SmartPtr<Header>(new Header);
        driver->read(512, sizeof(Header), (u8*)hdr.get());
        // Validate GPT Header
        if (hdr->Revision == 0) {
#ifdef DEBUG_GPT
            UART::out("  Revision is not zero\r\n");
#endif /* DEBUG_GPT */
            return false;
        }
        for (u8 i = 0; i < 8; ++i) {
            if (hdr->Signature[i] != HEADER_SIGNATURE[i]) {
#ifdef DEBUG_GPT
                UART::out("  Signature doesn't match\r\n");
#endif /* DEBUG_GPT */
                return false;
            }
        }
#ifdef DEBUG_GPT
        UART::out("  Valid GPT\r\n");
#endif /* DEBUG_GPT */
        return true;
    }
}
