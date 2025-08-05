#pragma once
#ifndef cpuCoreH
#define cpuCoreH

#include "console.h"



using console::Console;

class cpuCore {
    public:
    int coreId;
    Console* processToRun = nullptr; // Pointer to the process currently running on this core
    bool running = false; // Flag to control the core's operation
    int delayPerExec = 0; // Delay in milliseconds for each execution step

    cpuCore(int id, int execDelay) : coreId(id), delayPerExec(execDelay) {}

    void startProcess(Console* console){
        running = true;
        processLoop(console); // Start the process loop in a separate thread
    }

    void stop() {
        running = false;
    }

    void processLoop(Console* console) {
        processToRun = console; // Set the process to run on this core
        while (running) { 
            if(!console->process.commands.empty()) {
                try{
                    console->handleInput(console->process.commands.front());
                    console->process.commands.pop_front();
                }catch(std::exception e){
                    cout << "error with somethign?? " << e.what() << endl;
                };
            }
            console->process.currLine += 1;

            std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExec));
        

            {
                console->process.end();
                stop();
            }
        }
    }
};

#endif