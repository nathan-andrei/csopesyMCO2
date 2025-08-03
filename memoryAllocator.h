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

using std::vector;
using std::map;
using std::list;
using std::string;
using std::cout;
using std::endl;
using std::string;
using std::ofstream;


namespace memoryAllocator {

    struct Frame {
        int id;
        int startAddress;
        int endAddress;
        string pid = "";
    };


	// helper function to get timestamp
string getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%m/%d/%Y %I:%M:%S%p", std::localtime(&now));
    return string(buffer);
}


void writeMemorySnapshot(int quantumCycle, const vector<Frame>& frames, double memPerProc, int memPerFrame) {
    string filename = "memory_stamp_" + std::to_string(quantumCycle) + ".txt";
    ofstream file(filename);
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
    int totalExternalFragBytes = (4 - activeProcesses.size()) * memPerProc;
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
    file << "----end---- = " << frames.back().endAddress << endl;

    for (int i = frames.size() - 1; i >= 0;) {
        string pid = frames[i].pid;
        int end = frames[i].endAddress;
        int j = i;
        while (j >= 0 && frames[j].pid == pid) {
            --j;
        }
        int start = frames[j + 1].startAddress;

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
        int memoryPerProcess;
        int memoryPerFrame;
        vector<Frame> frames;

        // Constructor with initialization
        MemoryAllocator(int maxOverallMemory, int memPerFrame, int memPerProc)
            : maxMemory(maxOverallMemory),
              memoryPerFrame(memPerFrame),
              memoryPerProcess(memPerProc) 
        {
            numFrames = maxMemory / memoryPerFrame;
            for (int i = 0; i < numFrames; ++i) {
                Frame f;
                f.id = i;
                f.startAddress = i * memoryPerFrame;
                f.endAddress = f.startAddress + memoryPerFrame;
                f.pid = "";
                frames.push_back(f);
            }
        }

        // Default constructor
        MemoryAllocator() {}

        // First-Fit allocation
        bool AllocateProcess(process::Process& p) {
            int numNeededFrames = memoryPerProcess / memoryPerFrame;
            int frameCounter = 0;
            int startFrameId = -1;

            // Search for contiguous free frames
            for (size_t i = 0; i < frames.size(); ++i) {
                if (frames[i].pid.empty()) {
                    frameCounter++;
                } else {
                    frameCounter = 0;
                }

                if (frameCounter == numNeededFrames) {
                    startFrameId = static_cast<int>(i) - numNeededFrames + 1;
                    break;
                }
            }

            if (startFrameId != -1) {
                for (int i = startFrameId; i < startFrameId + numNeededFrames; ++i) {
                    frames[i].pid = p.pname;
                    p.frameIds.push_back(i);
                }
                return true;
            } else {
                return false;
            }
        }

        void DeallocateProcess(process::Process& p) {
            for (int frameId : p.frameIds) {
                if (frameId >= 0 && frameId < frames.size()) {
                    frames[frameId].pid.clear();
                }
            }
            p.frameIds.clear();
        }
    };

} // namespace memoryAllocator

#endif
