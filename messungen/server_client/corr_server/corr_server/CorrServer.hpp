//
//  CorrServer.hpp
//  corr_server
//
//  Created by tihmstar on 19.03.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#ifndef CorrServer_hpp
#define CorrServer_hpp

#include <thread>
#include <mutex>
#include <atomic>
#include <set>
#include "CorrClient.hpp"
#include <vector>
#include <iostream>
#include <queue>
#include <set>
#include "CPUModelResults.hpp"
#include <map>

class CorrServer {
    
    int _port;
    int _serverfd;
    std::thread *_acceptWorkerThread;
    std::atomic<bool> _acceptWorkerShouldStop;
    std::atomic<bool> _acceptWorkerIsRunning;
    std::set<CorrClient *> _clientWorkers;
    std::mutex _clientWorkersSetLock;
    
    uint32_t _point_per_trace;
    std::vector<std::string> _powermodels;
    std::queue<std::string> _fileQueue;
    std::mutex _fileQueueLock;
    
    std::set<std::set<std::string>> _workInProgressBatches;
    std::mutex _workInProgressBatchesLock;

    
    std::map<std::string,std::map<std::string,CPUModelResults*>> _allmodelresults;
    std::mutex _allmodelresultsLock;

    std::mutex _filesQueueUpdateEventLock;

    
    void acceptWorker();
    
private:
    std::set<std::string> dequeueBatch(uint32_t size);
    void giveBackUnfinishedBatch(std::set<std::string> batch);
    void combineAllModelResults(std::map<std::string,std::map<std::string,CPUModelResults*>> allmodelresults, std::set<std::string> batch);
    
public:
    CorrServer(int port, uint32_t point_per_trace, std::vector<std::string> powermodels, std::queue<std::string> fileQueue);
    ~CorrServer();
    

    size_t queuedFilesCnt();
    size_t unqueuedFilesCnt();
    size_t remainingFilesCnt();

    uint32_t clientsCnt();
    
    void hangUntilFilesUpdate(); //only a single caller possible

    void notifyMannualFilesUpdate();
    
    void notifyClientsWorkAvailable();
    
    void saveCorrToDir(std::string dstDir);
    
    void printClientStats();
    void killClient(int clientfd);
    
    friend CorrClient;
};


#endif /* CorrServer_hpp */
