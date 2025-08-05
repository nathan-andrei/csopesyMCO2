#pragma once
#ifndef memAllocatorH
#define memAllocatorH

#include <string>
#include <ctime>
#include <iostream>
#include <list>
#include <map>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include "process.h"
#include <set>
#include "frame.h"

using std::vector;
using std::map;
using std::list;
using std::string;
using std::cout;
using std::endl;
using std::string;
using std::ofstream;
using process::Process;

namespace memoryAllocator {

	// helper function to get timestamp
string getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%m/%d/%Y %I:%M:%S%p", std::localtime(&now));
    return string(buffer);
}


void writeMemorySnapshot(int quantumCycle, const vector<Frame>& frames, int memPerFrame) {
    string filename = "memory_stamp_" + std::to_string(quantumCycle) + ".txt";
    ofstream file(filename);
    cout << "writing to file" << endl;
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << " for writing.\n";
        return;
    }

    // Timestamp
    file << "Timestamp: (" << getCurrentTimestamp() << ")" << endl;

    // Count unique processes
    std::set<string> activeProcesses;
    for (const auto& frame : frames) {
        if (!frame.pid.empty()) activeProcesses.insert(frame.pid);
    }
    file << "Number of processes in memory: " << activeProcesses.size() << endl;

    //Fix calc, we no longer use memPerProc
    int totalExternalFragBytes = 0;//(4 - activeProcesses.size()) * memPerProc;

    // External fragmentation
    /*double totalExternalFragBytes = 0;
    double contiguousFreeSize = 0;

    int framesNeededPerProcess = memPerProc / memPerFrame;

    for (size_t i = 0; i < frames.size(); ++i) {
        if (frames[i].pid.empty()) {
            double frameSize = frames[i].endAddress - frames[i].startAddress;
            contiguousFreeSize += frameSize;
        } else {
            if (contiguousFreeSize > 0) {
                std::cout << "hole found, current free size is " << contiguousFreeSize << " bytes of contiguous free memory found." << std::endl;
                int numFreeFrames = contiguousFreeSize / memPerFrame;
                if (numFreeFrames < framesNeededPerProcess) {
                    totalExternalFragBytes += contiguousFreeSize;
                }
                contiguousFreeSize = 0;
            }
        }
    }
    int numFreeFrames = contiguousFreeSize / memPerFrame;
    if (contiguousFreeSize > 0 && numFreeFrames < framesNeededPerProcess) {
        totalExternalFragBytes += contiguousFreeSize;
    }*/

    file << "Total external fragmentation in KB: " << (totalExternalFragBytes) << " \n" << endl;

    // Memory layout
    file << "----end---- = " << frames.back().endAddress_i << endl;

    for (int i = frames.size() - 1; i >= 0;) {
        string pid = frames[i].pid;
        int end = frames[i].endAddress_i;
        int j = i;
        while (j >= 0 && frames[j].pid == pid) {
            --j;
        }
        int start = frames[j + 1].startAddress_i;

        if (!pid.empty()) {
            file << end << "\n";
            file << pid << "\n";
            file << start << "\n\n";
        } else {
            // Optional: comment this block out if you don't want to print gaps
            // file << end << "\n";
            // file << "FREE\n";
            // file << start << "\n\n";
        }

        i = j;
    }

    file << "----start---- = 0" << endl;
    file.close();
}

	
    class MemoryAllocator {
    public:
        int maxMemory;
        int numFrames;
        int memoryPerFrame;
        vector<Frame> frames;
        vector<Process*> allocatedProcesses;

        // Constructor with initialization
        MemoryAllocator(int maxOverallMemory, int memPerFrame)
            : maxMemory(maxOverallMemory),
              memoryPerFrame(memPerFrame)
        {
            numFrames = maxMemory / memoryPerFrame;
            for (int i = 0; i < numFrames; ++i) {
                Frame f;
                f.id = i;
                f.startAddress_i = i * memoryPerFrame;
                f.endAddress_i = f.startAddress_i + memoryPerFrame - 1;

                f.startAddress = "0x" + std::to_string(i);
                f.endAddress = "0x" + std::to_string(i*2);

                f.pid = "";
                frames.push_back(f);
            }
        }

        // Default constructor
        MemoryAllocator() {}

        // First-Fit allocation: non-contiguous
        bool AllocateProcess(process::Process& p) {
            int numNeededFrames = p.size / memoryPerFrame;
            int frameCounter = 0;
            int startFrameId = -1;

            list<int> idList;
            
            if(p.inBackingStore) p.reviveFromStore();

            // Search for free frames
            for (size_t i = 0; i < frames.size(); ++i) {
                if (frames[i].pid.empty()) {
                    frameCounter++;
                    idList.push_back(i);
                } 
                if (frameCounter == numNeededFrames) {
                    break;
                }
            }

            while(frameCounter < numNeededFrames){
                //look at the processes currently allocated (???)
                Process* leastRecent;
                for(Process* p : allocatedProcesses){
                    if(leastRecent != NULL){
                        if(leastRecent->waitingCounter < p->waitingCounter)
                            leastRecent = p;
                    }
                    else{
                        leastRecent = p;
                    }
                }

                //look for the one that ran the least recently used

                //put that process to backing store and store the freed ids
                list<int> freedIds = PutToBackingStore(*leastRecent);

                for(int id : freedIds){
                    idList.push_back(id);
                    frameCounter++;

                    if(frameCounter >= numNeededFrames) break;
                }

            }

            if (frameCounter == numNeededFrames) {
                for (int i : idList) {
                    frames[i].pid = p.pname;
                    p.frames.push_back(frames[i]);
                }
                /* For debugging.
                cout << "Allocated to " << p.pname << " are:" << endl;
                for(int j : idList){
                    cout << j << endl;
                }
                cout << "-----" << endl;*/
                allocatedProcesses.push_back(&p);
                return true;
            } else {
                return false;
            }
        }

        //First-fit allocation: contigouous
        bool AllocateProcessContiguous(process::Process& p) {
            int numNeededFrames = p.size / memoryPerFrame;
            int frameCounter = 0;
            int startFrameId = -1;

            // Search for free frames
            for (size_t i = 0; i < frames.size(); ++i) {
                if (frames[i].pid.empty()) {
                    frameCounter++;
                } 
                else frameCounter = 0; 

                if (frameCounter == numNeededFrames) {
                    startFrameId = static_cast<int>(i) - numNeededFrames + 1;
                    break;
                }
            }

            if (startFrameId != -1) {
                for (int i = startFrameId; i < startFrameId + numNeededFrames; ++i) {
                    frames[i].pid = p.pname;
                    p.frames.push_back(frames[i]);
                }
                return true;
            } else {
                return false;
            }
        }

        list<int> PutToBackingStore(process::Process& p){
            p.putToBackingStore();
            list<int> ids;

            for (Frame f : p.frames) {
                if (f.id >= 0 && f.id < frames.size()) {
                    frames[f.id].pid.clear();
                    ids.push_back(f.id);
                }
            }
            p.frames.clear();
            
            return ids;
        }

        void DeallocateProcess(process::Process& p) {
            for (Frame f : p.frames) {
                if (f.id >= 0 && f.id < frames.size()) {
                    frames[f.id].pid.clear();
                }
            }
            p.frames.clear();
        }
    };

} // namespace memoryAllocator

#endif
