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

#ifndef LENSOROS_KERNEL_FILE_ALLOCATION_TABLE_FILE_DATA_ITERATOR_H
#define LENSOROS_KERNEL_FILE_ALLOCATION_TABLE_FILE_DATA_ITERATOR_H

#include <fat_cluster_iterator.h>
#include <stddef.h>
#include <stdint.h>
#include <storage/filesystem_drivers/file_allocation_table.h>

#include <iterator>
#include <vector>

namespace FAT {

// Returns the rest of a file's data in cluster-sized chunks.
struct FileDataIterator {
    using iterator_category = std::input_iterator_tag;
    using value_type = std::vector<uint8_t>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    explicit FileDataIterator(
        FileAllocationTableDriver& driver,
        std::span<uint8_t> FAT,
        u64 firstClusterIndex)
        : driver(driver), iterator(FAT, firstClusterIndex) {}

    explicit FileDataIterator(
        FileAllocationTableDriver& driver,
        ClusterIterator&& in_iterator)
        : driver(driver), iterator(in_iterator) {}

    auto operator++() -> FileDataIterator& {
        cache_fresh = false;
        ++iterator;
        return *this;
    }
    auto operator++(int) -> FileDataIterator {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }
    reference operator*() {
        ensure_cache_fresh();
        return cache;
    }
    pointer operator->() {
        ensure_cache_fresh();
        return &cache;
    }

    bool operator!=(std::default_sentinel_t) const {
        return iterator != iterator.end();
    };
    bool operator==(std::default_sentinel_t v) const {
        return not operator!=(v);
    };

    // Get an iterator that points to the first entry.
    auto begin() -> FileDataIterator {
        return FileDataIterator{driver, iterator.begin()};
    }

    // We don’t really have a predetermined ‘end’, so this just returns a
    // dummy value.
    auto end() const -> std::default_sentinel_t { return {}; }

    auto get_driver() const -> FileAllocationTableDriver& { return driver; }
    auto get_underlying_iterator() -> ClusterIterator& { return iterator; }

   private:
    void ensure_cache_fresh() {
        if (cache_fresh) return;
        cache.resize(0);
        driver.read_cluster_into(cache, *iterator);
        cache_fresh = true;
    }

    FileAllocationTableDriver& driver;
    ClusterIterator iterator;
    std::vector<uint8_t> cache{};
    bool cache_fresh{false};
};

};  // namespace FAT

#endif  // LENSOROS_KERNEL_FILE_ALLOCATION_TABLE_FILE_DATA_ITERATOR_H
