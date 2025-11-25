#include <vector>

#include "./components/SPair.hpp"
#include "../avl-tree/AVLTree.hpp"
#include "../hashmap/HMap.hpp"

/* A collection of (score, name) pairs ordered from low to high */
class SortedSet {
    public:
        ~SortedSet();
        
        /**
         * Inserts a new (score, name) pair into the SortedSet if it doesn't already exist. Otherwise, updates the 
         * existing pair.
         * 
         * @param score The score.
         * @param name  Byte array that stores the name.
         * @param len   Length of the name.
         * 
         * @return  True if the a new pair is inserted.
         *          False if the pair already exists.
         */
        bool insert(double score, const char *name, uint32_t len);

        /** 
         * Searches for a pair with the given name in the SortedSet.
         * 
         * @param name  Byte array that stores the name.
         * @param len   Length of the name.
         * 
         * @return  A pointer to the pair if found.
         *          NULL if a pair is not found.
         */
        SPair *lookup(const char *name, uint32_t len);

        /**
         * Finds all pairs in the SortedSet greater than or equal to the given (score, name) pair. Optionally accepts an 
         * offset to exclude a certain number of pairs from the beginning of the result, and limit to restrict the 
         * number of pairs returned.
         * 
         * @param score     The score.
         * @param name      Byte array that stores the name.
         * @param len       Length of the name.
         * @param offset    (Optional) Number of pairs to exclude at the beginning of the result. Default is 0.
         * @param limit     (Optional) Maximum number of pairs to return. Default is 0 (no limit).
         * 
         * @return  Vector containing pointers to pairs in the SortedSet that are greater than or equal to the given 
         *          pair.
         */
        std::vector<SPair *>find_all_ge(double score, const char *name, uint32_t len, int64_t offset = 0, uint64_t limit = 0);

        /**
         * Removes the pair with the given name from the SortedSet.
         * 
         * @param name  Byte array that stores the name.
         * @param len   Length of the name.
         * 
         * @return  True if the pair is removed.
         *          False if the pair does not exist.
         */
        bool remove(const char *name, uint32_t len);

        /**
         * Finds the rank (position in sorted order) of the pair with the given name in the SortedSet.
         * 
         * The rank is 0-based, so the lowest pair is rank 0.
         * 
         * @param name  Byte array that stores the name.
         * @param len   Length of the name.
         * 
         * @return  The rank of the pair if found.
         *          -1 if the pair does not exist.
         */
        int64_t rank(const char *name, uint32_t len);

        /* Returns the number of pairs in the SortedSet */
        uint32_t length();
    private:
        HMap map; // used for point queries
        AVLTree tree; // used for range and rank queries

        /**
         * Updates the given pair and adjusts its order in the SortedSet.
         * 
         * @param pair  Pointer to the pair to update.
         * @param score New score for the pair.
         */
        void update(SPair *pair, double score);

        /**
         * Finds the first pair in the SortedSet greater than or equal to the given (score, name) pair. 
         * 
         * @param score The score.
         * @param name  Byte array that stores the name.
         * @param len   Length of the name.
         * 
         * @return  Pointer to the pair if found.
         *          NULL if no such pair exists in the SortedSet.
         */
        SPair *find_first_ge(double score, const char *name, uint32_t len);

        /**
         * Finds the pair located offset positions away from the specified pair in the SortedSet.
         *
         * @param pair      Pointer to the starting pair.
         * @param offset    Number of positions away the offset pair is from the starting pair.
         *
         * @return  Pointer to the pair if found.
         *          NULL if no such pair exists in the SortedSet.
         */
        SPair *find_offset(SPair *pair, int64_t offset);
};
