#include "../include/Cache.h"
#include "../include/Memory.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>

/*
Name: George Candal
Date: 3/4/26
Updated: 3/30/26 - Added page table simulation for Stage 2
Purpose: This simulates a cache. It opens a trace file of memory accesses. Decodes
the address and simulates the data movement in the cache. It then records certain metrics 
and prints them out. (hit rate, miss rate, eviction rate, data movement, and 
cache utilization)
*/

int main(int argc, char* argv[]) {

	if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <num_sets> <num_ways> <block_size> <memory_size> <page_size>\n";
        return 1;
    }

    uint64_t num_sets = std::stoull(argv[1]);
    uint64_t num_ways = std::stoull(argv[2]);
    uint64_t block_size = std::stoull(argv[3]);
    uint64_t memory_size = std::stoull(argv[4]);
    uint64_t page_size = std::stoull(argv[5]);

    mem::Cache cache(num_sets, num_ways, block_size);
    mem::Memory memory(memory_size, page_size);


    // Open trace file
    std::ifstream traceFile("./benchmark/trace.txt");
    if (!traceFile.is_open()) {
        std::cerr << "Error: Could not open benchmark/trace.txt\n";
        return 1;
    }

    // Read addresses from file
    uint64_t addr;
    uint64_t set_id, tag, offset, victim_tag;
    std::string line;
    uint64_t hit_counter = 0, miss_counter = 0, eviction_counter = 0, data_size = 0;

    while (std::getline(traceFile, line)) {
        // Split line into access_type and address
        char access_type = line[0];
        std::string address = line.substr(2);

        // Convert address to int
        try {
            addr = std::stoull(address);
        } catch (...) {
            continue;
        }
        
        // Decode address, use findblock to see if hit or miss
        cache.DecodeAddress(addr, set_id, tag, offset);
        uint64_t way_id;
        mem::BlockState state;
        bool hit = cache.FindBlock(addr, set_id, way_id, state);

        // If hit update data size by 8 bytes (We are assuming that on a hit,
        // the CPU moves 8 bytes between cache and CPU). Update hit counter by
        // 1, update LRU bits in tree by calling AccessBlock, and if its a 
        // write, we update block state to modified
        if (hit) {
            data_size += 8;
            hit_counter += 1;
            cache.AccessBlock(set_id, way_id);
            if (access_type == 'W') {
                cache.setBlock(set_id, way_id, tag, mem::BlockState::Modified);
            } 
        // Else, its a cache miss. Update miss_counter, walk the page table, 
        // Find the block to be replaced and get the victim's block state. If 
        // the block is not invalid, update eviction counter. If block has been 
        // modified increase data size by the block size (to write the old block back)
        // Set the block state accordingly depending on if write or read.
        // Call accessBlock to update LRU bits.
        // Update data size by block size(to write block into cache) plus 8 (to
        // move data from cache to cpu)
        } else {
            miss_counter += 1;
            memory.pageWalk(addr, access_type);
            uint64_t victim_way_id = cache.ReplaceBlock(set_id);
            cache.getBlock(set_id, victim_way_id, victim_tag, state);

            if (state != mem::BlockState::Invalid) {
                eviction_counter += 1;
                if (state == mem::BlockState::Modified) {
                    data_size += block_size;
                }
            }

            if (access_type == 'W') {
                cache.setBlock(set_id, victim_way_id, tag, mem::BlockState::Modified);
            } else {
                cache.setBlock(set_id, victim_way_id, tag, mem::BlockState::Exclusive);
            }
            
            cache.AccessBlock(set_id, victim_way_id);
            data_size += (block_size + 8);
        }

    }

    // Calculate hit rate, miss rate, and eviction rate
    uint64_t total = hit_counter + miss_counter;
    float hit_rate = hit_counter * 100.0 / total;
    float miss_rate = miss_counter * 100.0 / total;
    float eviction_rate = eviction_counter * 100.0 / miss_counter;

    // This is used to calculate cache utilization. Loop through every block and
    // check state. If its not invalid, then the way contains a block and
    // valid_blocks is increased by 1
    mem::BlockState state;
    uint64_t temp_tag;
    uint64_t valid_blocks = 0;
    for (uint64_t set_id = 0; set_id < num_sets; set_id++) {
        for (uint64_t way_id = 0; way_id < num_ways; way_id++) {
            cache.getBlock(set_id, way_id, temp_tag, state);
            if (state != mem::BlockState::Invalid) {
                valid_blocks += 1;
            }
        }
    }

    // Calculate cache utilization 
    uint64_t total_blocks = num_sets * num_ways;
    float cache_utilization = valid_blocks * 100.0 / total_blocks;
    

    // Print out results
    std::cout<<"\n--Cache Stats--"<<std::endl;
    std::cout<<"Cache Hit Rate: "<<hit_rate<<std::endl;
    std::cout<<"Cache Miss Rate: "<<miss_rate<<std::endl;
    std::cout<<"Eviction Rate: "<<eviction_rate<<std::endl;
    std::cout<<"Data Movement: "<<data_size<<" bytes"<<std::endl;
    std::cout<<"Cache Utilization: "<<cache_utilization<<std::endl;

    std::cout<<"\n--Page Table Stats--"<<std::endl;
    std::cout<<"Hit Rate: "<<memory.getHitRatio()<<std::endl;
    std::cout<<"Miss Rate: "<<memory.getMissRatio()<<std::endl;
    std::cout<<"Evictions: "<<memory.getEvectionCount()<<std::endl;
    std::cout<<"Data Movement: "<<memory.getDataMovement()<<" bytes"<<std::endl;

    traceFile.close();

    return 0;
}
