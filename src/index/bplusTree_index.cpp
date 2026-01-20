#include "bplusTree_index.h"
#include<algorithm>
#include<iostream>
#include<fstream>
#include<filesystem>

namespace fs = std::filesystem;
using namespace std;

BPTreeNode::BPTreeNode(bool leaf): isLeaf(leaf),next(nullptr){
    //order 4,max key 3,max child 3
    keys.reserve(3);
    if(leaf){
        values.reserve(3);
    }else{
        children.reserve(4);
    }
}

BPTreeNode::~BPTreeNode(){
    if(!isLeaf){
        for(auto child : children){
            delete child;
        }
    }
}

BPlusTreeIndex::BPlusTreeIndex(){
    root = new BPTreeNode(true);
}

BPlusTreeIndex::~BPlusTreeIndex(){
    if(root){
        delete root;
    }
}

BPTreeNode* BPlusTreeIndex::findLeaf(const string &key){

    BPTreeNode* current = root;

    while(!current->isLeaf){
        int i =0;
        while(i < current -> keys.size() && key >= current -> keys[i]){
            i++;
        }
        current = current -> children[i];
    }
    return current;
}

void BPlusTreeIndex::insert(const string &key,uint64_t offset){

    //first key
    if(root -> keys.empty()){
        root -> keys.push_back(key);
        root -> values.push_back(offset);
        return;
    }

    SplitResult result = insertRecursive(root, key, offset);
    if(result.isSplit){
        BPTreeNode* newRoot = new BPTreeNode(false);
        newRoot -> keys.push_back(result.separatorKey);
        newRoot -> children.push_back(root);
        newRoot -> children.push_back(result.newNode);
        root = newRoot;
    }
}

SplitResult BPlusTreeIndex::insertRecursive(BPTreeNode* node, const string &key, uint64_t offset){
    
    if(node -> isLeaf){

        // Insertto leaf node
        int keyPosition = 0;
        while(keyPosition < node -> keys.size() && key > node -> keys[keyPosition]){
            keyPosition++;
        }
        
        node -> keys.insert(node -> keys.begin() + keyPosition, key);
        node -> values.insert(node -> values.begin() + keyPosition, offset);
        
        // Check for overflow (max 3 keys for order 4)
        if(node -> keys.size() > 3){
            string separatorKey = splitLeaf(node);
            return SplitResult(true, separatorKey, node -> next);
        }
        
        return SplitResult(false, "", nullptr);
    }
    else{
        // Internal node - find child to descend
        int childIndex = 0;
        while(childIndex < node -> keys.size() && key >= node -> keys[childIndex]){
            childIndex++;
        }
        
        SplitResult childResult = insertRecursive(node -> children[childIndex], key, offset);
        if(!childResult.isSplit){
            return SplitResult(false, "", nullptr);
        }
        
        insertIntoInternal(node, childResult.separatorKey, childResult.newNode, childIndex);
        if(node -> keys.size() > 3){
            string promotedKey;
            BPTreeNode* newInternal = splitInternalNode(node, promotedKey);
            return SplitResult(true, promotedKey, newInternal);
        }
        
        return SplitResult(false, "", nullptr);
    }
}

string BPlusTreeIndex::splitLeaf(BPTreeNode* leaf){
    BPTreeNode* newLeaf = new BPTreeNode(true);

    //split middle (right biased);
    int mid = (leaf -> keys.size()) / 2;

    //design right part
    newLeaf -> keys.assign(leaf -> keys.begin() + mid, leaf -> keys.end());
    newLeaf -> values.assign(leaf -> values.begin() + mid, leaf -> values.end());

    //resize left part
    leaf -> keys.resize(mid);
    leaf -> values.resize(mid);

    newLeaf -> next = leaf -> next;
    leaf -> next = newLeaf;

    return newLeaf -> keys[0];
}

BPTreeNode* BPlusTreeIndex::splitInternalNode(BPTreeNode* node, string& promotedKey){
    BPTreeNode* newInternal = new BPTreeNode(false);
    
    int mid = node -> keys.size() / 2;
    
    // Middle key is promoted
    promotedKey = node -> keys[mid];
    
    // Right node gpart
    newInternal -> keys.assign(node -> keys.begin() + mid + 1, node -> keys.end());
    newInternal -> children.assign(node -> children.begin() + mid + 1, node -> children.end());
    
    // Left node part
    node -> keys.resize(mid);
    node -> children.resize(mid + 1);
    
    return newInternal;
}

void BPlusTreeIndex::insertIntoInternal(BPTreeNode* node, const string& key, BPTreeNode* child, int childIndex){

    node -> keys.insert(node -> keys.begin() + childIndex, key);
    node -> children.insert(node -> children.begin() + childIndex + 1, child);
}

vector<uint64_t> BPlusTreeIndex::search(const string &key){
    vector<uint64_t> results;
    
    if (!root) {
        return results;
    }
    
    BPTreeNode* leaf = findLeaf(key);
    
    if (!leaf) {
        return results;
    }
    
    // Search for key in the leaf node
    for(size_t i = 0; i < leaf->keys.size(); i++){
        if(leaf->keys[i] == key){
            results.push_back(leaf->values[i]);
        }
    }

    return results;
}

vector<uint64_t> BPlusTreeIndex::rangeSearch(const string &low,const string &high){
    vector<uint64_t> allOffset;
    BPTreeNode* leaf = findLeaf(low);

    while(leaf != nullptr){
        for(int i =0;i< leaf -> keys.size();i++){
            const string &key = leaf -> keys[i];
            if(key < low) continue;
            if(key > high) return allOffset;
            allOffset.push_back(leaf -> values[i]);
        }
        leaf = leaf -> next;
    }
    return allOffset;
}

void BPlusTreeIndex::saveNodeToDisk(ofstream& out, BPTreeNode* node) {

    /*out format like this-

    1.write node type leaf or internal
    2.write number of keys
    3.write keys
    4.if leaf write values
    5.if internal write number of children
    6.recursively write children
    */

    if (!node) return;
    out.write(reinterpret_cast<const char*>(&node->isLeaf), sizeof(bool));
    
    size_t num_keys = node->keys.size();
    out.write(reinterpret_cast<const char*>(&num_keys), sizeof(num_keys));
    
    for (size_t i = 0; i < num_keys; i++) {
        size_t key_len = node->keys[i].size();
        out.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
        out.write(node->keys[i].c_str(), key_len);
    }
    
    if (node->isLeaf) {
        for (size_t i = 0; i < node->values.size(); i++) {
            out.write(reinterpret_cast<const char*>(&node->values[i]), sizeof(uint64_t));
        }
    } else {
        size_t num_children = node->children.size();
        out.write(reinterpret_cast<const char*>(&num_children), sizeof(num_children));
        
        for (auto child : node->children) {
            saveNodeToDisk(out, child);
        }
    }
}

void BPlusTreeIndex::saveToDisk(const string &table) {
    fs::create_directories("data/" + table);
    string filePath = "data/" + table + "/" + table + ".bptidx";
    
    ofstream out(filePath, ios::binary);
    if (!out) {
        cerr << "[ERROR] Cannot create B+ tree index file\n";
        return;
    }
    
    saveNodeToDisk(out, root);
    out.close();
}

BPTreeNode* BPlusTreeIndex::loadNodeFromDisk(ifstream& in) {

    /*
    in format like this-
    1.read node type leaf or internal
    2.read number of keys
    3.read keys
    4.if leaf read values
    5.if internal read number of children
    6.recursively read children  
    */

    bool isLeaf;
    in.read(reinterpret_cast<char*>(&isLeaf), sizeof(bool));
    
    if (in.eof() || in.fail()) {
        return nullptr;
    }
    
    BPTreeNode* node = new BPTreeNode(isLeaf);
    size_t num_keys;
    in.read(reinterpret_cast<char*>(&num_keys), sizeof(num_keys));
    
    if (in.eof() || in.fail()) {
        delete node;
        return nullptr;
    }
    
    for (size_t i = 0; i < num_keys; i++) {
        size_t key_len;
        in.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));
        
        string key(key_len, '\0');
        in.read(&key[0], key_len);
        
        node->keys.push_back(key);
    }
    
    if (isLeaf) {
        for (size_t i = 0; i < num_keys; i++) {
            uint64_t offset;
            in.read(reinterpret_cast<char*>(&offset), sizeof(uint64_t));
            node->values.push_back(offset);
        }
    } else {
        size_t num_children;
        in.read(reinterpret_cast<char*>(&num_children), sizeof(num_children));
        
        for (size_t i = 0; i < num_children; i++) {
            BPTreeNode* child = loadNodeFromDisk(in);
            if (child) {
                node->children.push_back(child);
            }
        }
    }
    
    return node;
}

void BPlusTreeIndex::rebuildLeafLinks(BPTreeNode* node, BPTreeNode*& prevLeaf) {
    if (!node) return;
    
    if (node->isLeaf) {
        // Link this leaf to the previous leaf
        if (prevLeaf) {
            prevLeaf->next = node;
        }
        prevLeaf = node;
        node->next = nullptr;  // Will be set by next leaf
    } else {
        // Traverse children in order (left to right)
        for (auto child : node->children) {
            rebuildLeafLinks(child, prevLeaf);
        }
    }
}

void BPlusTreeIndex::loadFromDisk(const string &table) {
    string filePath = "data/" + table + "/" + table + ".bptidx";
    
    ifstream in(filePath, ios::binary);
    if (!in) {
        if (root) {
            delete root;
        }
        root = new BPTreeNode(true);
        return;
    }
    
    if (root) {
        delete root;
    }
    
    root = loadNodeFromDisk(in);
    
    if (!root) {
        root = new BPTreeNode(true);
    } else {
        // Rebuild leaf node links for range queries
        BPTreeNode* prevLeaf = nullptr;
        rebuildLeafLinks(root, prevLeaf);
    }
    
    in.close();
}