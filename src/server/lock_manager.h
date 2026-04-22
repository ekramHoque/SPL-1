#pragma once
#include <mutex>
#include <shared_mutex>
#include <map>
#include <string>

enum LockType {
    LOCK_TYPE_READ,   
    LOCK_TYPE_WRITE   
};

class LockManager {
private:
    std::shared_mutex databaseLock;
    
    int activeReaders;
    int activeWriters;
    int waitingClients;

    mutable std::mutex statsMutex;
    
    std::map<std::string, LockType> clientLocks; 
    std::mutex clientLocksMutex;
    
public:
    LockManager();
    ~LockManager();
    
    bool acquireReadLock(const std::string& clientId);
    bool acquireWriteLock(const std::string& clientId);
    void releaseLock(const std::string& clientId);
    int getActiveReaders() const;
    int getActiveWriters() const;
    int getWaitingClients() const;
    void printLockStatus() const;
};

class LockGuard {
private:
    LockManager* lockManager;
    std::string clientId;
    bool locked;
    
public:
    LockGuard(LockManager* manager, const std::string& client_id, LockType type);
    ~LockGuard();
    bool isLocked() const { return locked; }
};
