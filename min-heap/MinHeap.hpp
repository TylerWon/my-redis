#pragma once

#include <vector>

#include "components/MHNode.hpp"

class MinHeap {
    public:
        /**
         * Inserts a node into the MinHeap.
         * 
         * @param node      The node to insert.
         * @param is_less   A function that checks if a node is less than another node.
         */
        void insert(MHNode *node, bool (*is_less)(MHNode *, MHNode *));

        /**
         * Updates the position of the node in the heap if it violates the invariant that all nodes must be less than 
         * their children.
         * 
         * @param pos       The node to update.
         * @param is_less   A function that checks if a node is less than another node.
         */
        void update(MHNode *node, bool (*is_less)(MHNode *, MHNode *));

        /**
         * Removes a node from the MinHeap.
         * 
         * @param node      The node to remove.
         * @param is_less   A function that checks if a node is less than another node.
         */
        void remove(MHNode *node, bool (*is_less)(MHNode *, MHNode *));

        /* Returns the min node in the heap or NULL if the heap is empty */
        MHNode *min();

        /* Checks if the MinHeap is empty */
        bool is_empty();
    private:
        std::vector<MHNode *> nodes;

        /**
         * Updates the position of the node if it is less than its parent.
         * 
         * @param pos       The position of the node in the heap.
         * @param is_less   A function that checks if a node is less than another node.
         */
        void heapify_up(uint32_t pos, bool (*is_less)(MHNode *, MHNode *));
        
        /**
         * Updates the position of the node if it is greater than at least one of its children.
         * 
         * @param pos       The position of the node in the heap.
         * @param is_less   A function that checks if a node is less than another node.
         */
        void heapify_down(uint32_t pos, bool (*is_less)(MHNode *, MHNode *));

        /**
         * Gets the node's parent in the MinHeap.
         * 
         * @param pos   The position of the node.
         * 
         * @return  The node's parent.
         *          NULL if the node is the root.
         */
        MHNode *get_parent(uint32_t pos);

        /**
         * Gets the node's left child in the MinHeap.
         * 
         * @param pos   The position of the node.
         * 
         * @return  The node's left child.
         *          NULL if the node doesn't have a left child.
         */
        MHNode *get_left(uint32_t pos);
    
        /**
         * Gets the node's right child in the MinHeap.
         * 
         * @param pos   The position of the node.
         * 
         * @return  The node's right child.
         *          NULL if the node doesn't have a right child.
         */
        MHNode *get_right(uint32_t pos);
    
    #ifdef TEST_MODE
    public:      
        std::vector<MHNode *> get_nodes() { return nodes; }
    #endif
};
