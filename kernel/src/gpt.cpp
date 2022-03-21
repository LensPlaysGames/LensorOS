#include "gpt.h"

#include "ahci.h"
#include "smart_pointer.h"

namespace GPT {
    bool is_gpt_present(AHCI::Port* port) {
        SmartPtr<Header> hdr = SmartPtr<Header>(new Header);
        if (port->read(1, 1, hdr.get(), sizeof(Header))) {
            // Validate GPT Header
            const char* HeaderSignature = "EFI PART";

            if (hdr->Revision == 0)
                return false;

            for (u8 i = 0; i < 8; ++i)
                if (hdr->Signature[i] != HeaderSignature[i])
                    return false;

            // TODO: Parse partitions!            
        }
        return true;
    }
}
