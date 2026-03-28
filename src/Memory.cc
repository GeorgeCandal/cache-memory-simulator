#include "../include/Memory.h"
#include <cassert>
#include <cmath>

namespace mem {

Memory::Memory(int memorySizeKB, int pageSizeKB)
    : mainMemorySizeKB(memorySizeKB), pageSizeKB(pageSizeKB)
{
    // Ensure page size is a power of 2
    assert(pageSizeKB > 0 && (pageSizeKB & (pageSizeKB - 1)) == 0);

    // Initialize Page Table (Virtual Memory)
    uint64_t num_pages_estimate = memorySizeKB / pageSizeKB;

    for (uint64_t page_id = 0; page_id < num_pages_estimate; ++page_id) {
        pageTable[page_id] = PageTableEntry();
    }

    // Initialize Frame Table (Physical Memory)
    // Smaller than page table (1/10 size)
    uint64_t num_frames = num_pages_estimate / 10;

    for (uint64_t frame_id = 0; frame_id < num_frames; ++frame_id) {
        frameTable[frame_id] = Frame(frame_id);
    }

    /*
     * STUDENT NOTE:
     * - pageTable represents virtual memory (large)
     * - frameTable represents physical memory (small)
     * - Frames will be allocated to pages on demand
     * - When frameTable is empty, LRU replacement must be used
     */
}

// Compute log2 of a value (must be power of 2)
uint64_t Memory::LogBase2(uint64_t value) const {
    assert(value > 0 && (value & (value - 1)) == 0);

    uint64_t result = 0;
    while ((value & 1) == 0) {
        value >>= 1;
        result++;
    }
    return result;
}

// Decode memory address into page number and offset
void Memory::DecodeAddress(uint64_t address, uint64_t &page_number, uint64_t &offset) const
{
    uint64_t page_size_bytes = pageSizeKB * 1024;
    assert(page_size_bytes > 0 && (page_size_bytes & (page_size_bytes - 1)) == 0);

    uint64_t log_page_size = LogBase2(page_size_bytes);

    offset = address & ((1ULL << log_page_size) - 1);
    page_number = address >> log_page_size;
}

// Student skeleton for page walk
bool Memory::pageWalk(uint64_t address, char access_type)
{
    // Students will:
    // 1. Extract page number and offset
    // 2. Check pageTable using page_number as key
    // 3. Handle hit or fault
    // 4. Allocate frame from frameTable OR call LRU

    return false;
}

// Student skeleton for LRU replacement
void Memory::handleLRUReplacement(int incomingPageNumber)
{
    // Students will:
    // 1. Identify LRU page from lruList
    // 2. If updated → increment num_evection
    // 3. Reassign its frame to incoming page
    // 4. Update pageTable and lruList
}

// Stats accessors
double Memory::getHitRatio() const {
    return static_cast<double>(num_hit) / (num_hit + num_miss);
}

double Memory::getMissRatio() const {
    return static_cast<double>(num_miss) / (num_hit + num_miss);
}

unsigned long Memory::getEvectionCount() const {
    return num_evection;
}

} // namespace mem