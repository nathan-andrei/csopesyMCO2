#pragma once
#ifndef cpuCoreH
#define cpuCoreH
  
#include "console.h"

using std::thread;

namespace cpucore {
class cpuCore {
    public:
    int coreId;
    console::Console* processToRun = nullptr; // Pointer to the process currently running on this core
    bool running = false; // Flag to control the core's operation
    int delayPerExec = 0; // Delay in milliseconds for each execution step

    thread worker;

    cpuCore(int id, int execDelay) : coreId(id), delayPerExec(execDelay) {}

    void startProcess(console::Console* console){
        running = true;
        processToRun = console; // Set the process to run on this core
        worker = thread(&cpuCore::processLoop, this, console);
    }

    void stop() {
        running = false;
        processToRun = nullptr; // Clear the process pointer
        if (worker.joinable()) {
            worker.join();
        }
    }

    void processLoop(console::Console* console) {
        while (running && !console->process.commands.empty()) { 
            try{
                console->handleInput(console->process.commands.front());
                console->process.commands.pop_front();
                console->process.currLine += 1;
            }catch(std::exception e){
                cout << "error with somethign?? " << e.what() << endl;
            };
            
            std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExec));
        }

        console->process.end();
        running = false;
    }

    bool processFinished(){
        return processToRun->process.commands.empty();
    }
};
}


#endif