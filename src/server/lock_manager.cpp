#include "lock_manager.h"
#include <iostream>
#include <iomanip>

LockManager::LockManager() {
    activeReaders = 0;
    activeWriters = 0;
    waitingClients = 0;
}

LockManager::~LockManager() {
}

bool LockManager::acquireReadLock(const std::string& clientId) {
    {
        std::lock_guard<std::mutex> statsGuard(statsMutex);
        waitingClients++;
    }
    databaseLock.lock_shared();
    
    {
        std::lock_guard<std::mutex> statsGuard(statsMutex);
        waitingClients--;
        activeReaders++;
    }
    
    {
        std::lock_guard<std::mutex> client_guard(clientLocksMutex);
        clientLocks[clientId] = LOCK_TYPE_READ;
    }
    
    return true;
}

bool LockManager::acquireWriteLock(const std::string& clientId) {
    {
        std::lock_guard<std::mutex> statsGuard(statsMutex);
        waitingClients++;
    }
    databaseLock.lock();
    
    {
        std::lock_guard<std::mutex> statsGuard(statsMutex);
        waitingClients--;
        activeWriters++;
    }
    
    {
        std::lock_guard<std::mutex> client_guard(clientLocksMutex);
        clientLocks[clientId] = LOCK_TYPE_WRITE;
    }
    
    return true;
}

void LockManager::releaseLock(const std::string& clientId) {
    LockType lock_type;
    
    {
        std::lock_guard<std::mutex> client_guard(clientLocksMutex);
        auto it = clientLocks.find(clientId);
        
        if (it == clientLocks.end()) {
            return;
        }
        
        lock_type = it->second;
        clientLocks.erase(it);
    }
    
    if (lock_type == LOCK_TYPE_READ) {
        databaseLock.unlock_shared();
        
        std::lock_guard<std::mutex> statsGuard(statsMutex);
        activeReaders--;
    } else {
        databaseLock.unlock();
        
        std::lock_guard<std::mutex> statsGuard(statsMutex);
        activeWriters--;
    }
}

int LockManager::getActiveReaders() const {
    std::lock_guard<std::mutex> guard(statsMutex);
    return activeReaders;
}

int LockManager::getActiveWriters() const {
    std::lock_guard<std::mutex> guard(statsMutex);
    return activeWriters;
}

int LockManager::getWaitingClients() const {
    std::lock_guard<std::mutex> guard(statsMutex);
    return waitingClients;
}

void LockManager::printLockStatus() const {
    std::lock_guard<std::mutex> guard(statsMutex);
    
    std::cout << "\n======= LOCK STATUS =======" << std::endl;
    std::cout << "Active Readers:  " << activeReaders << std::endl;
    std::cout << "Active Writers:  " << activeWriters << std::endl;
    std::cout << "Waiting Clients: " << waitingClients << std::endl;
    std::cout << "===========================\n" << std::endl;
}


LockGuard::LockGuard(LockManager* manager, const std::string& clientId, LockType type) {
    lockManager = manager;
    this->clientId = clientId;
    locked = false;
    
    if (lockManager != nullptr) {
        if (type == LOCK_TYPE_READ) {
            locked = lockManager->acquireReadLock(clientId);
        } else {
            locked = lockManager->acquireWriteLock(clientId);
        }
    }
}

LockGuard::~LockGuard() {
    if (lockManager != nullptr && locked) {
        lockManager->releaseLock(clientId);
    }
}
