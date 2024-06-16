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

#include <memory/common.h>
#include <memory/physical_memory_manager.h>
#include <format>

bool test_pmm_single_page() {
  u8* mem = (u8*)Memory::request_page();
  mem[4] = 'a';
  if (mem[4] != 'a') {
    std::print("test_pmm_single_page() failed: Did not read back correct value after writing.\n");
    return false;
  }
  Memory::free_page(mem);
  return true;
}

bool test_pmm_multiple_pages() {
  u8* mem = (u8*)Memory::request_pages(2);
  mem[4] = 'a';
  mem[4 + PAGE_SIZE] = 'b';
  if (mem[4] != 'a' || mem[4 + PAGE_SIZE] != 'b') {
    std::print("test_pmm_multiple_pages() failed: Did not read back correct value(s) after writing.\n");
    return false;
  }
  Memory::free_pages(mem, 2);
  return true;
}

/// NOTE: Must be run before multi-processing, depending on how the memory
/// allocator reports free RAM.
bool test_pmm_frees() {
  u64 amountFree = Memory::free_ram();
  u64 amountUsed = Memory::used_ram();

  u8* mem = (u8*)Memory::request_page();
  if (Memory::free_ram() == amountFree) {
    std::print("test_pmm_frees() failed: Page does not appear to have been marked unavailable (no longer free).\n");
    return false;
  }
  if (Memory::used_ram() == amountUsed) {
    std::print("test_pmm_frees() failed: Page does not appear to have been marked used.\n");
    return false;
  }

  Memory::free_page(mem);
  if (Memory::free_ram() != amountFree) {
    std::print("test_pmm_frees() failed: Page does not appear to have been marked available after freeing (failure to free).\n");
    return false;
  }
  if (Memory::used_ram() != amountUsed) {
    std::print("test_pmm_frees() failed: Page does not appear to have been marked unused.\n");
    return false;
  }

  return true;
}

void run_tests() {
  constexpr const char* success = "    \033[32mSuccess\033[31m\n";
  std::print("Tests:\n\033[31m");
  if (test_pmm_single_page()) std::print(success);
  if (test_pmm_multiple_pages()) std::print(success);
  if (test_pmm_frees()) std::print(success);
  std::print("\033[0m");
}
