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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#ifndef LENSOROS_KERNEL_FILE_ALLOCATION_TABLE_CLUSTER_ITERATOR_H
#define LENSOROS_KERNEL_FILE_ALLOCATION_TABLE_CLUSTER_ITERATOR_H

#include <fat_definitions.h>
#include <stdint.h>

#include <iterator>
#include <span>

namespace FAT {

// NOTE:
// Cluster       :: Fixed Size Chunk Of Data
// Cluster Entry :: Metadata about a particular cluster, located within
//                  the file allocation table (FAT). The cluster referenced is the cluster
//                  with the same index as the index of the cluster entry in the FAT. Entry
//                  0 corresponds to cluster 0, and so on.

// Each iteration returns the current cluster index, following to the next
// cluster upon advancing, beginning at the given cluster index.
// The first iteration, upon creation, will therefore always return the
// given cluster index.
struct ClusterIterator {
    using iterator_category = std::forward_iterator_tag;
    using value_type = uint64_t;             // The type of element being iterated
    using difference_type = std::ptrdiff_t;  // Type for distance between iterators
    using pointer = value_type*;             // Pointer to the element
    using reference = value_type&;           // Reference to the element
    // TODO: Parameterize; template or constructor
    constexpr static auto Type = FATType::FAT32;

    auto EntryCount(FATType type) const {
        // 2 because of implicit 0 and 1 clusters
        if constexpr (Type == FATType::FAT32 or Type == FATType::ExFAT)
            return 2 + (table.size() / sizeof(uint32_t));
        else if constexpr (Type == FATType::FAT16)
            return 2 + (table.size() / sizeof(uint16_t));
        else if constexpr (Type == FATType::FAT12)
            return 2 + ((table.size() * 8) / 12);
    }

    explicit ClusterIterator(std::span<uint8_t> FAT, u64 firstClusterIndex)
        : table(FAT)
        , first_index(firstClusterIndex)
        , next_index(firstClusterIndex) {
        ++(*this);
    }

    auto operator++() -> ClusterIterator& {
        current_index = next_index;

        // 2 because of implicit 0 and 1 clusters
        if (current_index >= 2 and current_index < EntryCount(Type))
            next_index = current_cluster_entry();

        return *this;
    }
    auto operator++(int) -> ClusterIterator {
        ClusterIterator tmp = *this;
        ++(*this);
        return tmp;
    }
    reference operator*() { return current_index; }
    auto operator->() -> value_type* { return &current_index; }

    bool operator!=(std::default_sentinel_t) const {
        return current_index < EntryCount(Type);
    };
    bool operator==(std::default_sentinel_t v) const {
        return not operator!=(v);
    };

    // Get an iterator that points to the first entry.
    auto begin() -> ClusterIterator {
        return ClusterIterator{table, first_index};
    }

    // We don’t really have a predetermined ‘end’, so this just returns a
    // dummy value.
    auto end() const -> std::default_sentinel_t { return {}; }

   private:
    uint32_t current_cluster_entry_32() {
        return *(uint32_t*)(table.data() + (current_index * sizeof(uint32_t)));
    }
    uint16_t current_cluster_entry_16() {
        return *(uint16_t*)(table.data() + (current_index * sizeof(uint16_t)));
    }
    uint16_t current_cluster_entry_12() {
        // C = Cluster
        // Index = C + (C / 2) (i.e., 1.5 × C).
        // If Cluster is even, take the lower 12 bits of the 16-bit word at that
        // position. Otherwise, if Cluster is odd, take the upper 12 bits.
        const auto index = current_index + current_index / 2;
        const uint16_t word = *(uint16_t*)(table.data()
                                           + (index * sizeof(uint16_t)));
        const bool even = (current_index % 2) == 0;
        if (even)
            return word & 0xfff;
        return word >> 4;
    }

    constexpr uint32_t current_cluster_entry() {
        if constexpr (Type == FATType::FAT32 or Type == FATType::ExFAT)
            return current_cluster_entry_32();
        else if constexpr (Type == FATType::FAT16)
            return current_cluster_entry_16();
        else if constexpr (Type == FATType::FAT12)
            return current_cluster_entry_12();
    }

    std::span<uint8_t> table{};
    usz first_index{0};
    // This is the index of the cluster that is currently available at
    // current_cluster_entry.
    usz current_index{0};
    // This is the index stored within the current cluster entry.
    // It may be an invalid index (i.e. -1).
    usz next_index{0};
};

}  // namespace FAT

#endif /* LENSOROS_KERNEL_FILE_ALLOCATION_TABLE_CLUSTER_ITERATOR_H */
