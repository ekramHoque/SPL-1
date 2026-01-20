#pragma once
#include<string>
#include<vector>
#include<cstdint>
#include<fstream>

class BPTreeNode;


struct SplitResult {
    bool isSplit;
    std::string separatorKey;
    BPTreeNode* newNode;
    
    SplitResult() : isSplit(false), newNode(nullptr) {}
    SplitResult(bool split, const std::string& key, BPTreeNode* node) 
        : isSplit(split), separatorKey(key), newNode(node) {}
};

class BPlusTreeIndex{

    private:
        BPTreeNode* root;
        BPTreeNode* findLeaf(const std::string &key);
        std::string splitLeaf(BPTreeNode* leaf);
        BPTreeNode* splitInternalNode(BPTreeNode* node, std::string& promotedKey);
        void insertIntoInternal(BPTreeNode* node, const std::string& key, BPTreeNode* child, int childIndex);
        SplitResult insertRecursive(BPTreeNode* node, const std::string& key, uint64_t offset);
        void saveNodeToDisk(std::ofstream& out, BPTreeNode* node);
        BPTreeNode* loadNodeFromDisk(std::ifstream& in);
        void rebuildLeafLinks(BPTreeNode* node, BPTreeNode*& prevLeaf);

    public:
        BPlusTreeIndex();
        ~BPlusTreeIndex();

        void insert(const std::string &key, uint64_t offset);
        std::vector<uint64_t> search(const std::string &key);
        std::vector<uint64_t> rangeSearch(const std::string &low, const std::string &high);
        void saveToDisk(const std::string &tableName);
        void loadFromDisk(const std::string &tableName);

};


class BPTreeNode{
    public:
        bool isLeaf;
        std::vector<std::string> keys;
        std::vector<uint64_t> values;
        std::vector<BPTreeNode*> children;
        BPTreeNode* next;

        BPTreeNode(bool leaf = true);
        ~BPTreeNode();

};