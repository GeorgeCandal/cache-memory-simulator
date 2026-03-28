#include "../include/Cache.h"
#include <algorithm>
#include <cstdlib>

/*
Name: George Candal
Date: 3/4/26
Purpose: This contains different methods for simulating a cache.
*/

namespace mem {
// ---------------------------
// Tree constructor
// --------------------------- 
PLRUTreeNode* buildPLRUTree(uint64_t level, uint64_t max_level, int &leaf_id, Set &set) {
    if (level == max_level) {
        PLRUTreeNode* node = new PLRUTreeNode(set.blocks[leaf_id]);
        leaf_id++;
        return node;
    }

    PLRUTreeNode* left = buildPLRUTree(level + 1, max_level, leaf_id, set);
    PLRUTreeNode* right = buildPLRUTree(level + 1, max_level, leaf_id, set);
    PLRUTreeNode* node = new PLRUTreeNode(left, right);
    return node;
}

// Constructor for tree-based PLRU cache
// ---------------------------
// Cache Constructor
// ---------------------------
Cache::Cache(uint64_t num_sets, uint64_t num_ways, uint64_t block_size,
             ReplacementPolicy repl, WritePolicy write)
    : num_sets_(num_sets),
      num_ways_(num_ways),
      block_size_(block_size),
      replacement_policy_(repl),
      write_policy_(write)
{
    // ---------------------------
    // Validate input: must be powers of two
    // ---------------------------
    assert(num_sets > 0 && (num_sets & (num_sets - 1)) == 0);
    assert(num_ways > 0 && (num_ways & (num_ways - 1)) == 0);
    assert(block_size > 0 && (block_size & (block_size - 1)) == 0);

    // ---------------------------
    // Compute total blocks and block mask
    // ---------------------------
    num_blocks_ = num_sets_ * num_ways_;
    block_mask_ = block_size_ - 1;

    // ---------------------------
    // Resize blocks and sets
    // ---------------------------
    blocks_.resize(num_blocks_);
    sets_.resize(num_sets_);

    // ---------------------------
    // Initialize each set and link blocks
    // ---------------------------
    for (uint64_t set_id = 0; set_id < num_sets_; ++set_id) {
        Set &set = sets_[set_id];

        // 1️⃣ Prepare vector of block pointers for this set
        set.blocks.resize(num_ways_);
        for (uint64_t way_id = 0; way_id < num_ways_; ++way_id) {
            Block &block = blocks_[set_id * num_ways_ + way_id];
            block.way_id = way_id;

            // Link block into set
            set.blocks[way_id] = &block;
        }

        // ---------------------------
        // Create the PLRU tree using recursion
        // ---------------------------
        
        int leaf_id = 0;
        uint64_t max_level = LogBase2(num_ways_);
        set.plru_root = buildPLRUTree(0, max_level, leaf_id, set);
    }
}



// Log2 computation
uint64_t Cache::LogBase2(uint64_t value) const {
    if (value == 0 || (value & (value - 1)) != 0)
        throw std::runtime_error("Value must be a power of 2 and non-zero");

    uint64_t result = 0;
    while ((value & 1) == 0) {
        value >>= 1;
        result++;
    }
    return result;
}

// Decode address into tag, set, and block offset
void Cache::DecodeAddress(uint64_t address,
                          uint64_t &set_id,
                          uint64_t &tag,
                          uint64_t &block_offset) const
{
    uint64_t log_block_size = LogBase2(block_size_);
    uint64_t log_num_sets   = LogBase2(num_sets_);

    block_offset = address & ((1ULL << log_block_size) - 1);
    set_id       = (address >> log_block_size) & ((1ULL << log_num_sets) - 1);
    tag          = address >> (log_block_size + log_num_sets);
}

// ---------------------------
// Find a block in the cache
// ---------------------------
bool Cache::FindBlock(uint64_t address,
                      uint64_t &set_id,
                      uint64_t &way_id,
                      BlockState &state) const
{
    //decode address to get set index and tag
    uint64_t tag;
    uint64_t block_offset;
    DecodeAddress(address, set_id, tag, block_offset);

    // Loop through each block, if tag matches fill in output parameters and 
    // return true
    for (uint64_t i = 0; i < num_ways_; i++) {
        Block* blk =  sets_[set_id].blocks[i];

        if (blk->tag == tag && blk->state != BlockState::Invalid) {
           way_id = blk->way_id;
           state = blk->state;
           return true;
        } 
    }

    // Block not found then update state and return false
    state = BlockState::Invalid;
    return false;
}
    
// ---------------------------
// Set the tag and state of a block
// ---------------------------
void Cache::setBlock(uint64_t set_id, uint64_t way_id,
                     uint64_t tag, BlockState state)
{
    Block* blk =  sets_[set_id].blocks[way_id];
    blk->tag = tag;
    blk->state = state;
}

// ---------------------------
// Get the tag and state of a block
// ---------------------------
void Cache::getBlock(uint64_t set_id, uint64_t way_id,
                     uint64_t &tag, BlockState &state) const
{
    Block* blk =  sets_[set_id].blocks[way_id];
    tag = blk->tag;
    state = blk->state;
}

// ---------------------------
// Mark a block as accessed (update LRU order)
// ---------------------------
void Cache::AccessBlock(uint64_t set_id, uint64_t way_id)
{
    // Start at the root of the tree
    PLRUTreeNode* current_node = sets_[set_id].plru_root;
    uint64_t low = 0;
    uint64_t high = num_ways_;

    // This is used to update the LRU bits when a block is accessed.
    // The ways are in numerical order at the bottom of the tree (w0, w1, w2, etc...)
    // To access the correct block its going to use a binary seach approach to 
    // walk the tree. If the way_id is less than the mid it will go left and update
    // the LRU bit. If it is >= mid it will go right and update the LRU bit accordingly.
    // For the LRU bits, false = LRU to the left, true = LRU to the right. Then it will
    // update the node to the one it walked to and repeat the process until the 
    // node is a leaf node. That way all LRU bits on the way to the node are updated. 
    while(current_node->is_leaf == false) {
        uint64_t mid = (high + low) / 2;

        if (way_id < mid) {
            current_node->lru_bit = true;
            current_node = current_node->left;
            high = mid;
        } else {
            current_node->lru_bit = false;
            current_node = current_node->right;
            low = mid;
        }
    }
}

// ---------------------------
// Determine which block to replace in a set
// ---------------------------
uint64_t Cache::ReplaceBlock(uint64_t set_id)
{
    // When a block needs to be saved to the cache. This will locate where it 
    // should be saved. This will start at the root and follow the LRU bits down
    // the tree. If bit is false go left, if true go right. It will flip the
    // LRU bit on its path down and stop when it reaches a leaf node. It will
    // then return the way_id of that node.   

    PLRUTreeNode* current_node = sets_[set_id].plru_root;

    while (current_node->is_leaf == false) {
        if (current_node->lru_bit == false) {
            current_node->lru_bit = true;
            current_node = current_node->left;
        } else {
            current_node->lru_bit = false;
            current_node = current_node->right;
        }
    }

    return current_node->block->way_id;
}
} // namespace mem
