/*
 *  Basic Cache Memory Simulator
 *  Implements Least Recently Used (LRU) replacement policy
 *  and Write Back write policy.
 *
 *  Copyright (C) 2026  Essa Alarusie
 *
 *  Usage:
 *  ------
 *  1. Compile the simulator:
 *         make
 *
 *  2. Run the simulator:
 *         make run NUM_SETS=32 NUM_WAYS=8 BLOCK_SIZE=128
 *
 *  Note:
 *  - The simulator expects a trace file at ./benchmark/trace.txt
 *  - The cache parameters can be adjusted in Cache.h / Cache.cc
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <list>
#include <stdexcept>
#include <cassert>


namespace mem {

// ---------------------------
// Cache Block
// ---------------------------
enum class BlockState {
    Invalid,
    Shared,
    Exclusive,
    Modified,
    Owned,
    NonCoherent
};

struct Block {
    uint64_t tag = 0;           // Tag for address matching
    BlockState state = BlockState::Invalid;
    unsigned way_id = 0;        // Leaf index in PLRU tree
};

// ---------------------------
// PLRU Tree Node (for true tree-based PLRU)
// ---------------------------
struct PLRUTreeNode {
    bool lru_bit = false;            // Only for internal nodes
    bool is_leaf = false;            // True if this node is a leaf
    Block* block = nullptr;          // Valid only if leaf
    PLRUTreeNode* left = nullptr;    // Left child
    PLRUTreeNode* right = nullptr;   // Right child

    // Leaf constructor
    PLRUTreeNode(Block* blk) : is_leaf(true), block(blk), lru_bit(false) {}

    // Internal node constructor
    PLRUTreeNode(PLRUTreeNode* l, PLRUTreeNode* r)
        : is_leaf(false), left(l), right(r), lru_bit(false), block(nullptr) {}
};


// ---------------------------
// Cache Set (Tree-based PLRU)
// ---------------------------
struct Set {
    std::vector<Block*> blocks;       // Pointers to blocks in the set
    PLRUTreeNode* plru_root = nullptr; // Root of the PLRU tree

    Set() = default;
};

// ---------------------------
// Cache class
// ---------------------------
class Cache {
public:
    enum class ReplacementPolicy { LRU, FIFO, Random, PLRU };
    enum class WritePolicy { WriteBack, WriteThrough };

private:
    uint64_t num_sets_;
    uint64_t num_ways_;
    uint64_t block_size_;
    uint64_t num_blocks_;

    ReplacementPolicy replacement_policy_;
    WritePolicy write_policy_;

    std::vector<Block> blocks_;      // All blocks, linearized
    std::vector<Set> sets_;          // All sets

    uint64_t block_mask_;             // Mask for block offset

public:
    // ---------------------------
    // Constructor
    // ---------------------------
    Cache(uint64_t num_sets, uint64_t num_ways, uint64_t block_size,
          ReplacementPolicy repl = ReplacementPolicy::PLRU,
          WritePolicy write = WritePolicy::WriteBack);

    // ---------------------------
    // Utility
    // ---------------------------
    uint64_t LogBase2(uint64_t value) const;

    void DecodeAddress(uint64_t address,
                       uint64_t &set_id,
                       uint64_t &tag,
                       uint64_t &block_offset) const;

    // ---------------------------
    // Cache Operations
    // ---------------------------
    bool FindBlock(uint64_t address,
                   uint64_t &set_id,
                   uint64_t &way_id,
                   BlockState &state) const;

    void setBlock(uint64_t set_id, uint64_t way_id,
                  uint64_t tag, BlockState state);

    void getBlock(uint64_t set_id, uint64_t way_id,
                  uint64_t &tag, BlockState &state) const;

    void AccessBlock(uint64_t set_id, uint64_t way_id); // Update PLRU tree

    uint64_t ReplaceBlock(uint64_t set_id);             // Select victim using PLRU
};

} // namespace mem


