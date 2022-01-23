#include "acpi.h"

namespace ACPI {
	void* find_table(SDTHeader* header, char* signature) {
		int entries = (header->Length - sizeof(ACPI::SDTHeader)) / 8;
		for (int t = 0; t < entries; ++t) {
			ACPI::SDTHeader* sdt = (ACPI::SDTHeader*)*(uint64_t*)((uint64_t)header + sizeof(ACPI::SDTHeader) + (t * 8));
		    for (int i = 0; i < 4; ++i) {
				if (sdt->Signature[i] != signature[i]) {
					// Signature does not match given, check next header.
					break;
				}
				else if (i == 3) {
					// Successfully matched 4 char signature
					return sdt;
				}
			}
		}
		// Could not find table.
		return 0;
	}
}
