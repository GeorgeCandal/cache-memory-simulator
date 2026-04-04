#include "../include/Memory.h"
#include <cassert>
#include <cmath>

/*
Name: George Candal
Date: 3/30/26
Purpose: This contains different methods for implementing a page table.
*/

namespace mem {

Memory::Memory(int memorySizeKB, int pageSizeKB)
    : mainMemorySizeKB(memorySizeKB), pageSizeKB(pageSizeKB)
{
    // Ensure page size is a power of 2
    assert(pageSizeKB > 0 && (pageSizeKB & (pageSizeKB - 1)) == 0);

    // Initialize Page Table (Virtual Memory)
    uint64_t num_pages_estimate = memorySizeKB / pageSizeKB;

    // Initialize Frame Table (Physical Memory)
    // Smaller than page table (1/10 size)
    uint64_t num_frames = num_pages_estimate / 10;

    for (uint64_t frame_id = 0; frame_id < num_frames; ++frame_id) {
        frameTable[frame_id] = Frame(frame_id);
    }
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

// ---------------------------
// Walk the page table
// ---------------------------
bool Memory::pageWalk(uint64_t address, char access_type)
{
    bool hit = false;
    // Extract page number and offset
    uint64_t page_number;
    uint64_t offset;
    DecodeAddress(address, page_number, offset);

    // If page hit: increase hit counter, move page to back of lru list, update 
    // flag to true.
    if (pageTable.count(page_number) && pageTable[page_number].isPresentInMemory) {
        num_hit++;
        lruList.remove(page_number);
        lruList.push_back(page_number);
        hit = true;        

    // else page miss: increase miss counter
    // IF lru list size is less than frame table size (this means there is an empty frame).
    //  add page to next open frame, update isPresent, add page number to end of
    //  lru list and update data movement for page in
    // ELSE (all frames are filled): call handleLRUReplacement
    // Then change isUpdated to true if its a write access
    // Return true for hit, false for miss  
    } else {
        num_miss++;
        if (lruList.size() < frameTable.size()) {
            pageTable[page_number].frameNumber = lruList.size();
            pageTable[page_number].isPresentInMemory = true;
            lruList.push_back(page_number);
            data_movement += pageSizeKB * 1024;

        } else {
            handleLRUReplacement(page_number);
        }
    }

     if (access_type == 'W') {
            pageTable[page_number].isUpdated = true;
    }

    return hit;
}

// ---------------------------
// LRU replacement
// ---------------------------
void Memory::handleLRUReplacement(uint64_t incomingPageNumber)
{
    // Least Recently Used Page number
    uint64_t lru = lruList.front();
    // Simulate data movement for write back if written to. Change isUpdated back
    // to false. Update eviction counter. Evictions are only updated during writeback.
    if (pageTable[lru].isUpdated == true) {
        data_movement += pageSizeKB * 1024;
        pageTable[lru].isUpdated = false;
        num_evection++;    
    } 

    // Assign frame number to incoming page, update
    // frame number of victim page to -1, update both isPresentInMemory bits,
    // remove victim from lru list and push new page to back of lru list.
    // Update data movement for page in.
    pageTable[incomingPageNumber].frameNumber = pageTable[lru].frameNumber;
    pageTable[lru].frameNumber = -1;
    pageTable[lru].isPresentInMemory = false;
    pageTable[incomingPageNumber].isPresentInMemory = true;
    lruList.remove(lru);
    lruList.push_back(incomingPageNumber);
    data_movement += pageSizeKB * 1024;
}

// Stats accessors
double Memory::getHitRatio() const {
    return static_cast<double>(num_hit) / (num_hit + num_miss) * 100;
}

double Memory::getMissRatio() const {
    return static_cast<double>(num_miss) / (num_hit + num_miss) * 100;
}

unsigned long Memory::getEvectionCount() const {
    return num_evection;
}

unsigned long Memory::getDataMovement() const {
    return data_movement;
}

} // namespace mem