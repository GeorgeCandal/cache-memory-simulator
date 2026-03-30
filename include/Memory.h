#ifndef MEMORY_H
#define MEMORY_H

#include <unordered_map>
#include <list>
#include <cstdint>
#include <cassert>

/*
Name: George Candal
Date: 3/30/26
Purpose: Header file that contains the data structures and class definition 
for the page table simulation.
*/

namespace mem {

    /* Page table entry structure
       No page number stored here — key in the hash map represents the page number
    */
    struct PageTableEntry {
        int frameNumber;         // Frame in main memory (-1 if not loaded)
        bool isUpdated;          // True if page has been written
        bool isPresentInMemory;  // True if page is loaded in main memory

        PageTableEntry()
            : frameNumber(-1), isUpdated(false), isPresentInMemory(false) {}
    };

    /* Frame structure (physical memory) */
    struct Frame {
        int frameNumber;

        Frame(int id = -1) : frameNumber(id) {}
    };

    class Memory {
    private:
        int mainMemorySizeKB;        
        int pageSizeKB;              

        // Virtual memory (large)
        std::unordered_map<uint64_t, PageTableEntry> pageTable;

        // Physical memory (small)
        std::unordered_map<uint64_t, Frame> frameTable;

        // LRU tracking
        std::list<int> lruList;

        // Stats
        unsigned long num_hit = 0;
        unsigned long num_miss = 0;
        unsigned long num_evection = 0;
        unsigned long data_movement = 0;

    public:
        Memory(int memorySizeKB, int pageSizeKB);

        bool pageWalk(uint64_t address, char access_type);

        void handleLRUReplacement(int incomingPageNumber);

        void DecodeAddress(uint64_t address, uint64_t &page_number, uint64_t &offset) const;

        uint64_t LogBase2(uint64_t value) const;

        double getHitRatio() const;
        double getMissRatio() const;
        unsigned long getEvectionCount() const;
        unsigned long getDataMovement() const;
    };

} // namespace mem

#endif // MEMORY_H
