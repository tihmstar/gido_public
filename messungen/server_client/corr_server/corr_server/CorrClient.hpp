//
//  CorrClient.hpp
//  corr_server
//
//  Created by tihmstar on 19.03.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#ifndef CorrClient_hpp
#define CorrClient_hpp

#include <thread>
#include <atomic>
#include <vector>
#include <iostream>
#include <set>
#include <mutex>

class CorrServer;
class CorrClient {
public:
    enum BatchStatus{
        BatchStatusUninitialized = 0,
        BatchStatusWaitingForWork,  //we don't have a batch, but we are ready to serve one
        
        BatchStatusQueued,  //starting with this we aquired a batch
        BatchStatusSending,
        BatchStatusWorking,
        BatchStatusReceiving,
        BatchStatusCombining,
        BatchStatusDone,
        
        BatchStatusError
    };
    
private:
    CorrServer *_parent; //not owned
    int _clientFd;
    std::thread *_workerThread;
    std::atomic<bool> _workerShouldStop;
    std::atomic<bool> _workerIsRunning;

    std::mutex _workAvailablilityUpdateEventLock;
    
    uint32_t _processed;
    
    //received
    uint32_t _num_of_workers;
    
    //created
    std::set<std::string> _currentBatch;
    BatchStatus _currentBatchStatus;
    
    std::string _ipaddress;
    
    void worker();
    
public:
    CorrClient(CorrServer *parent, int clientFd, std::string ipaddr = "");
    ~CorrClient();
    
    void kill();
    
    void printStats();
    
    friend CorrServer;
};

#endif /* CorrClient_hpp */
