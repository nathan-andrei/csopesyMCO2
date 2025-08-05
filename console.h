#pragma once
#ifndef consoleH
#define consoleH

#include <string>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <list>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <regex>
#include <atomic>

#include "process.h" //This is for the process class
#include "memoryAllocator.h" //This is for the memory allocator class
#include "frame.h"
#include "stats.h"
#include "cpuCore.h"



using std::left;
using std::right;
using std::setw;
using std::vector;
using std::mutex;
using std::list;
using std::string;
using std::cout;
using std::endl;
using process::Process;
using std::condition_variable;
using std::chrono::milliseconds;
using std::thread;
using std::ref;
using std::regex;
using std::regex_replace;
using std::atomic;


inline void printVMStat(memoryAllocator::MemoryAllocator &allocator) {
    VMStat stats{};
    stats.totalMemory = allocator.maxMemory;

    // Count used frames
    uint64_t usedFrames = 0;
    for (auto &f : allocator.frames) {
        if (!f.pid.empty()) usedFrames++;
    }
    stats.usedMemory = usedFrames * allocator.memoryPerFrame;
    stats.freeMemory = stats.totalMemory - stats.usedMemory;

    // Placeholders
    stats.idleCpuTicks = 0;
    stats.activeCpuTicks = 0;
    stats.totalCpuTicks = 0;
    stats.pagedIn = 0;
    stats.pagedOut = 0;

    // Print like vmstat -s
    std::cout << stats.totalMemory / 1024 << " K total memory\n";
    std::cout << stats.usedMemory / 1024  << " K used memory\n";
    std::cout << stats.freeMemory / 1024  << " K free memory\n";
    std::cout << stats.idleCpuTicks       << " idle cpu ticks\n";
    std::cout << stats.activeCpuTicks     << " active cpu ticks\n";
    std::cout << stats.totalCpuTicks      << " total cpu ticks\n";
    std::cout << stats.pagedIn            << " pages paged in\n";
    std::cout << stats.pagedOut           << " pages paged out\n";
}

namespace console{
    class Console{
        public:
            Process process;          //The process associated with this console.
            bool exit = false;
            bool mainConsole = false;   //just a flag that this is the main console.      
            Console* handoff = NULL;    //something that tells the main program what console to switch to

            virtual void drawHeader(){
                char time[30];
                strftime(time, sizeof(time), "%m/%d/%Y, %I:%M:%S %p", &process.timestamp);
                cout << "      __   __   __   __   __   __   __" << endl;
                cout << "---|__|-|__|-|__|-|__|-|__|-|__|-|__|---" << endl;
                cout << process.pname << endl;
                cout << process.pid << endl;
                cout << "Time created: " << time << endl;
                cout << "You are in the console for " << process.pname << endl;
                cout << "====================================================================" << endl;
                cout << left << setw(30) << "Total lines of code: " << setw(15) << right << process.lineCount << endl;
                cout << left << setw(30) << "Current line: " << setw(15) << right << process.currLine << endl <<endl;
                printLog();
            };

            void printSMI(){
                cout << "Process name: " << process.pname << endl;
                cout << "Process id: " << process.pid << endl;
                printLog();
                if(process.currLine >= process.lineCount){
                    cout << "Finished!" << endl;
                }

                cout << endl << endl;
            }
            void printLog(){
                if(!process.log.empty()){
                    for(string s : process.log){
                        cout << s;
                    }
                }       
                cout << endl << endl;
            }
            virtual void printProcesses(){cout << "Type \"help\" or \"?\" for a list of commands." << endl << endl;};
            
            virtual void handleProcessCalls(string s);

            inline void handleInput(string s);

            Console(): process(){}; //For creating main console

            Console(Process p): process(std::move(p)){} //For consoles for processes       

            void clear(){
                system("cls");
                drawHeader();
                printProcesses();
            }
        private:
            void cmdHelp(){
                cout << "For more information on a specific command, type it out (i.e. type only 'screen' and enter)" << endl;
                cout << left << setw(15) << "clear" << setw(10) << "" << "Clear the screen while leaving the header." << endl;
                cout << left << setw(15) << "exit" << setw(10) << "" << "Exit the program." << endl;
                cout << left << setw(15) << "process-smi" << setw(10) << "" << "Prints the information of the process and the logs." << endl;
                cout << left << setw(15) << "PRINT" << setw(10) << "" << "Displays an output message to the console." << endl;
                cout << left << setw(15) << "DECLARE" << setw(10) << "" << "Declare a uint16 variable." << endl;
                cout << left << setw(15) << "ADD" << setw(10) << "" << "Add two values or variables and store the sum in a variable." << endl;
                cout << left << setw(15) << "SUBTRACT" << setw(10) << "" << "Subtract two values or variables and store the difference in a variable." << endl;
                cout << left << setw(15) << "SLEEP" << setw(10) << "" << "Causes this associated process to sleep for the amount of provided CPU ticks. If this processes is running, stop using any CPU cores." << endl;
                cout << left << setw(15) << "FOR" << setw(10) << "" << "Performs a for-loop, given a set/array of instructions. Can be nested up to three times." << endl;
            }
            void cmdPrintHelp(){
                cout << "usage: [PRINT|print]([<message>] [<message> +<var>])" << endl;
                cout << "Options:" << endl;
                cout << "\t" << "message" << "\t\t\t" << "Message to print" << endl;
                cout << "\t" << "var" << "\t\t\t" << "Variable value to append to the message" << endl;
            }
            void cmdDeclareHelp(){
                cout << "usage: [DECLARE|declare] (<var>, <value>)" << endl;
                cout << "Options:" << endl;
                cout << "\t" << "var" << "\t\t\t" << "Name for the new variable to declare." << endl;
                cout << "\t" << "value" << "\t\t\t" << "Value of the new variable." << endl;
            }
            void cmdAddHelp(){
                cout << "usage: [ADD|add] (<var1>, [<var2>] [value], [<var3>] [value] )" << endl;
                cout << "Options:" << endl;
                cout << "\t" << "var1" << "\t\t\t" << "Variable to store sum in." << endl;
                cout << "\t" << "var2" << "\t\t\t" << "1st value to use. Can be a literal." << endl;
                cout << "\t" << "var3" << "\t\t\t" << "2nd value to use. Can be a literal." << endl;
            }
            void cmdSubtractHelp(){
                cout << "usage: [SUBTRACT|subtract] (<var1>, [<var2>] [value], [<var3>] [value] )" << endl;
                cout << "Options:" << endl;
                cout << "\t" << "var1" << "\t\t\t" << "Variable to store difference in." << endl;
                cout << "\t" << "var2" << "\t\t\t" << "1st value to use. Can be a literal." << endl;
                cout << "\t" << "var3" << "\t\t\t" << "2nd value to use. Can be a literal." << endl;
            }
            void cmdSleepHelp(){
                cout << "usage: [SLEEP|sleep] (<ticks>)" << endl;
                cout << "Options:" << endl;
                cout << "\t" << "ticks" << "\t\t\t" << "Amount of ticks the process will sleep." << endl;
            }
            void cmdForHelp(){
                cout << "usage: [FOR|for] ([<instructions>], <loop count>)" << endl;
                cout << "Options:" << endl;
                cout << "\t" << "instructions" << "\t\t\t" << "Set of instructions to run. For loops can be nested up three times." << endl;
                cout << "\t" << "loop count" << "\t\t\t" << "Amount of time to run the instructions." << endl;
            }


    };

    class MainConsole : public Console{
        public:
            // Queues and flags
            list<Console> processQueue;
            vector<Console> runningProcesses;
            vector<Console> finishedProcesses;
            vector<cpucore::cpuCore> cores;

            mutex queueMutex;
            mutex processStatusMutex;
            condition_variable cv;

            int numCPU;
            string scheduler;
            int quantumCycles;
            int batchProcessFreq;
            int minIns;
            int maxIns;
            int delayPerExec;

            int maxOverallMem;
            int memPerFrame;
            int minMemPerProc;
            int maxMemPerProc;

            int numFrames;
            
            bool running = true;
            void handleProcessCalls(string s);
            void printProcesses();
            void _printProcesses(list<Console> list);
            void _printProcesses(vector<Console> list, bool withCore);
            void printProcessesToFile();

            void printProcessSMI();
            
            int consoleMade = 0;

            std::atomic<bool> generatingProcesses{false};
            std::thread processGeneratorThread;
            void startProcessGenerator(int i = 0, string s = "", int mem = 16);
            void stopProcessGenerator();
            void processGeneratorLoop(int i = 0, string s = "", int mem = 16);

            memoryAllocator::MemoryAllocator memManager;
            void drawHeader(){
                cout << "      ______   ______   ______   ______   ______   ______   ______" << endl;
                cout << "---|______|-|______|-|______|-|______|-|______|-|______|-|______|---" << endl;
                cout << "   ______   ______     ___   _______  ________   ______  ____  ____" << endl;
                cout << " .' ___  |.' ____ \\  .'   `.|_   __ \\|_   __  |.' ____ \\|_  _||_  _|" << endl;
                cout << "/ .'   \\_|| (___ \\_|/  .-.  \\ | |__) | | |_ \\_|| (___ \\_| \\ \\  / /" << endl;
                cout << "| |         _.____`. | |   | | |  ___/  |  _| _   _.____`.   \\ \\/ /" << endl;
                cout << "\\ `.___.'\\| \\____) |\\  `-'  /_| |_    _| |__/ || \\____) |  _|  |_" << endl;
                cout << " `.____ .' \\______.' `.___.'|_____|  |________| \\______.' |______|" << endl << endl;
                cout << "Hello, welcome, to CSOPESY command line!" << endl;
                cout << "Created by: Asnan, Pilea, De la Torre" << endl << endl << endl;
            };

            MainConsole(){
                mainConsole = true;
                
                drawHeader();
                printProcesses();
            }

            MainConsole(int nCpu, string sched, int qc, int bpf, int min, int max, int delay,int maxMem, int memPerFrame, int minmemPerProc, int maxmemPerProc) : numCPU(nCpu), scheduler(sched), quantumCycles(qc), batchProcessFreq(bpf), minIns(min), maxIns(max), delayPerExec(delay), memManager(maxMem, memPerFrame), minMemPerProc(minmemPerProc), maxMemPerProc(maxmemPerProc) {
                mainConsole = true;
                // Start CPU cores
                
                for (int i = 0; i < numCPU; ++i) {
                    cores.emplace_back(i, delayPerExec);
                }            
                drawHeader();
                printProcesses();
            }
            

            // Scheduler that adds n consoles with processes -- THIS DOES NOT SUPPORT HAVING MORE PROCESSES ADDED
            void FCFSscheduler(int numProcess){   

                //Wait until all processes are scheduled and completed
                while(true){ //possible error condition
                    //std::lock_guard<mutex> lock(queueMutex);
                    //if(processQueue.empty()) break;
                }

                cv.notify_all();
            }
        
        int quantumCounter = 0; //Counter for quantum cycles

        void rrscheduler(int numProcess) {  
            quantumCounter = 0;
                while(running){
                    //check context switch per core
                    for(int i = 0; i < numCPU; i++){
                        if(!cores[i].running){ //if core is not running, context switch
                            if(cores[i].processFinished()){ //Make a function that raises a flag if the currLine >= instruction count
                                memManager.DeallocateProcess(cores[i].processToRun->process); //deallocate the process memory
                                finishedProcesses.push_back(*cores[i].processToRun); //return value rather than the pointer;

                                for (auto it = runningProcesses.begin(); it != runningProcesses.end(); ) {
                                    if (it->process.pid == cores[i].processToRun->process.pid) {
                                        it = runningProcesses.erase(it); // returns the next valid iterator
                                    } else {
                                        ++it;
                                    }
                                }
                            } 
                            
                        //After updating the queues, get the next console
                        // - Check processqueue (back) for a new console
                        // - allocate the memory
                        if(!processQueue.empty()){
                            Console* processToRun = &processQueue.back(); //get the last console in the queue and store the pointer so we don't have to look for it again (e.x. Console* processToRun);
                            processQueue.pop_back(); //remove the last console in the queue
                            cout << "allocating!!" << endl;
                            memManager.AllocateProcess(processToRun->process);
                            runningProcesses.push_back(*processToRun);
                            cores[i].startProcess(processToRun); // - give it to the core                 
                        }
                        
                    }

                    //check context switch by quantumSlice
                    if(quantumCounter >= quantumCycles){
                        for(int i = 0; i < numCPU; i++){
                            for (auto it = runningProcesses.begin(); it != runningProcesses.end(); ) {
                                    if (it->process.pid == cores[i].processToRun->process.pid) {
                                        it = runningProcesses.erase(it); // returns the next valid iterator
                                    } else {
                                        ++it;
                                    }
                            }
                            processQueue.push_back(*cores[i].processToRun); //return value rather than the pointer; Idk if this should push_back or front
                            cores[i].stop(); //stop the core
                            if(!processQueue.empty()){
                                Console* processToRun = &processQueue.back(); //get the last console in the queue and store the pointer so we don't have to look for it again (e.x. Console* processToRun);
                                processQueue.pop_back(); //remove the last console in the queue
                                cout << "allocating new!!" << endl;
                                memManager.AllocateProcess(processToRun->process);
                                runningProcesses.push_back(*processToRun);
                                cores[i].startProcess(processToRun); // - give it to the core        

                            }
                        }
                        quantumCounter = 0; //reset the quantum counter
                    }
                    quantumCounter++;
                    std::this_thread::sleep_for(milliseconds(delayPerExec)); 
                }
        

            //     // Take snapshot
            //     //memoryAllocator::writeMemorySnapshot(quantumCounter, memManager.frames, memManager.memoryPerFrame);

            //     // Exit condition: nothing in queue and memory is empty
            //     {
            //         std::lock_guard<std::mutex> lock(queueMutex);
            //         bool memoryEmpty = std::all_of(memManager.frames.begin(), memManager.frames.end(), [](const Frame& f) {
            //             return f.pid.empty();
            //         });

            //         if (processQueue.empty() && memoryEmpty) break;
            //     }

            //     std::this_thread::sleep_for(milliseconds(1));
            // }

            cv.notify_all();
            }
        }
                    
    
        private:
            void cmdHelp(){
                cout << "For more information on a specific command, type it out (i.e. type only 'screen' and enter)" << endl;
                cout << setw(15) << "clear" << setw(10) << "" << "Clear the screen while leaving the header." << endl;
                cout << setw(15) << "exit" << setw(10) << "" << "Exit the program." << endl;
                cout << setw(15) << "initialize" << setw(10) << "Used to initialize the program with parameters specified in config.txt" << "" << endl;
                cout << setw(15) << "screen" << setw(10) << "" << "Start or open a console for a process." << endl;
                cout << setw(15) << "scheduler-test" << setw(10) << "" << "Every x cpu ticks (defined in config.txt), a new process is generated and put in the ready Queue." << endl;
                cout << setw(15) << "scheduler-stop" << setw(10) << "" << "Stops the scheduler-test command." << endl;
                cout << setw(15) << "report-util" << setw(10) << "" << "Prints a summary of CPU utilization and processes to a text file." << endl;
            }

            void cmdScreenHelp(){
                cout << "usage: screen [-ls] [-s <process name> <process memory size>] [-r <process name>]" << endl;
                cout << "Options:" << endl;
                cout << "\t" << "-ls" << "\t\t\t" << "List all the processes" << endl;
                cout << "\t" << "-s" << "\t\t\t" << "Start a new process with the process name and memory size (must be in 2^n format. [2^6, 2^16]])" << endl;
                cout << "\t" << "-r" << "\t\t\t" << "Redraw/resume session of a process" << endl;
            }
            void addNewProcess(string name){ //func for adding a new process - Only called when doing screen -s UNUSED DUE TO BUGGY: doesn't get caught by scheduler
                /*
                int newPID = consoleMade + 1;
                consoleMade += 1;
                
                processQueue.push_back(Console(Process(name, newPID, minIns, maxIns))); //add a new console to the end of the console list with the associated process.
                return &processQueue.back(); //return the created console
                */
                /*
                consoleMade++;
                {
                    processQueue.push_back(Console(Process(name, consoleMade, minIns, maxIns)));
                }
                cv.notify_one();
                */
            }
            Console* searchList(string name){ //used to search through the console list
                for(Console &c : processQueue){
                    if(c.process.pname == name) return &c; //If it finds a matching process name, return the address of the console
                }
                for(Console &c : runningProcesses){
                    if(c.process.pname == name) return &c; //If it finds a matching process name, return the address of the console
                }
                for(Console &c : finishedProcesses){
                    if(c.process.pname == name) return &c; //If it finds a matching process name, return the address of the console
                }
                cout << "[SearchList] could not find process name \"" << name << "\"" << endl;
                return NULL;
            }
    };

    void MainConsole::_printProcesses(list<Console> list){ //just a helper function for printing processes
        char time[30];
        char timeS[30];
        char timeF[30];
        int percentage;
        cout << left << setw(4) << "PID";
        cout << left << "\t" << setw(20) << "Name";
        cout << left << "\t" << setw(30) << "Time arrived";
        cout << left << "\t" << setw(30) << "Time started";
        cout << left << "\t" << setw(30) << "Time finished";
        cout << left << "\t" << setw(15) << "Current Line";
        cout << left << "\t" << setw(15) << "Total Lines";
        cout << left << "\t" << setw(17) << "Progress" << endl;

        if(!list.empty()){
            for(Console& c : list){
                strftime(time, sizeof(time), "%m/%d/%Y, %I:%M:%S %p", &c.process.arrivalTimeStamp);
                strftime(timeS, sizeof(timeS), "%m/%d/%Y, %I:%M:%S %p", &c.process.timestamp);
                strftime(timeF, sizeof(timeF), "%m/%d/%Y, %I:%M:%S %p", &c.process.finishTimeStamp);
                percentage = ((double)c.process.currLine / (double)c.process.lineCount) * 100.00;

                cout << left << setw(4) << c.process.pid;
                cout << left << "\t" << setw(20) << c.process.pname;
                cout << left << "\t" << setw(30) << time;
                cout << left << "\t" << setw(30) << timeS;
                cout << left << "\t" << setw(30) << timeF;
                cout << left << "\t" << setw(15) << c.process.currLine;
                cout << left << "\t" << setw(15) << c.process.lineCount;
                cout << right << "\t" << setw(3) << percentage;
                cout << left << "% [";
                for(int i = 0; i < percentage / 10; i++){
                    cout << "#";
                }
                for(int i = percentage / 10; i < 9; i++){
                    cout << "-";
                }   
                cout << "]" << endl;
            }
        }
        else{
            cout << "No processes to be listed." << endl;
        }
        cout << endl << endl;
    }

    void MainConsole::_printProcesses(vector<Console> list, bool withCore = false){
        char time[30];
        char timeS[30];
        char timeF[30];
        int percentage;
        cout << left << setw(4) << "PID";
        cout << left << "\t" << setw(20) << "Name";
        cout << left << "\t" << setw(30) << "Time arrived";
        cout << left << "\t" << setw(30) << "Time started";
        if(!withCore)
            cout << left << "\t" << setw(30) << "Time finished";
        else
            cout << left << "\t" << setw(8) << "Core";
        cout << left << "\t" << setw(15) << "Current Line";
        cout << left << "\t" << setw(15) << "Total Lines";
        cout << left << "\t" << setw(17) << "Progress";
        if(withCore)
            cout << left << "\t" << setw(15) << "Memory Utilization" << endl;
        else
            cout << endl;

        if(!list.empty()){
            for(Console& c : list){
                strftime(time, sizeof(time), "%m/%d/%Y, %I:%M:%S %p", &c.process.arrivalTimeStamp);
                strftime(timeS, sizeof(timeS), "%m/%d/%Y, %I:%M:%S %p", &c.process.timestamp);
                strftime(timeF, sizeof(timeF), "%m/%d/%Y, %I:%M:%S %p", &c.process.finishTimeStamp);
                percentage = ((double)c.process.currLine / (double)c.process.lineCount) * 100.00;

                cout << left << setw(4) << c.process.pid;
                cout << left << "\t" << setw(20) << c.process.pname;
                cout << left << "\t" << setw(30) << time;
                cout << left << "\t" << setw(30) << timeS;
                if(!withCore)
                    cout << left << "\t" << setw(30) << timeF;
                else
                    cout << left << "\t" << setw(8) << c.process.core;
                cout << left << "\t" << setw(15) << c.process.currLine;
                cout << left << "\t" << setw(15) << c.process.lineCount;
                cout << right << "\t" << setw(3) << percentage;
                cout << left << "% [";
                for(int i = 0; i < percentage / 10; i++){
                    cout << "#";
                }
                for(int i = percentage / 10; i < 9; i++){
                    cout << "-";
                }   
                cout << "]     ";

                if(withCore)
                    cout << left << setw(15) << c.process.getMemorySize() * memPerFrame << endl;
                else 
                    cout << endl;
            }
        }
        else{
            cout << "No processes to be listed." << endl;
        }
        cout << endl << endl;
    }

    void MainConsole::printProcesses(){
        char refreshTime[30];
        time_t refresh;
        struct tm timestamp;
        time(&refresh); //Log when the process was started
        localtime_s(&timestamp, &refresh); //Turn epoch time to calendar time

        int numCores = cores.size();
        int usedCores = runningProcesses.size(); //This should be a safe assumption that each running process represents a core being used

        cout << "CPU Utilization: " << ((double)usedCores/(double)numCores) * 100.00  << "%" << endl;
        cout << "Cores used: " << usedCores << endl;
        cout << "Cores available: " << numCores - usedCores << endl;

        strftime(refreshTime, sizeof(refreshTime), "%m/%d/%Y, %I:%M:%S %p", &timestamp);
        cout << "List generated on: " << refreshTime << endl;
        cout << "===========================================================" << endl;
        cout << "Processes ready" << endl;
        _printProcesses(processQueue);

        cout << "Running processes" << endl;
        _printProcesses(runningProcesses, true);

        cout << "Finished processes" << endl;
        _printProcesses(finishedProcesses);
        cout << "===========================================================" << endl;

        cout << endl;

        cout << "Type \"help\" or \"?\" for a list of commands." << endl << endl;
    }

    void MainConsole::printProcessesToFile(){
        std::string filename = "csopesy-log.txt";

        // Check if the file is empty
        std::ifstream infile(filename);
        bool isEmpty = infile.peek() == std::ifstream::traits_type::eof();
        infile.close();

        std::ofstream  logFile(filename); //no append, clear the file everytime.

        std::ostringstream newLog;

        //i'm sorry for my sins but i'm just gonna copy whatever printProcesses already does except have it print to oss
        char refreshTime[30];
        time_t refresh;
        struct tm timestamp;
        time(&refresh); //Log when the process was started
        localtime_s(&timestamp, &refresh); //Turn epoch time to calendar time

        char time[30];
        char timeS[30];
        char timeF[30];
        int percentage;

        int numCores = cores.size();
        int usedCores = runningProcesses.size(); //This should be a safe assumption that each running process represents a core being used

        newLog << "CPU Utilization: " << ((double)usedCores/(double)numCores) * 100.00  << "%" << "\n";
        newLog << "Cores used: " << numCores << endl;
        newLog << "Cores available: " << usedCores << endl;

        strftime(refreshTime, sizeof(refreshTime), "%m/%d/%Y, %I:%M:%S %p", &timestamp);
        newLog << "List generated on: " << refreshTime << "\n";
        newLog << "===========================================================" << "\n";
        newLog << "Processes ready" << "\n";
        newLog << left << setw(4) << "PID";
        newLog << left << "\t" << setw(20) << "Name";
        newLog << left << "\t" << setw(30) << "Time arrived";
        newLog << left << "\t" << setw(30) << "Time started";
        newLog << left << "\t" << setw(30) << "Time finished";
        newLog << left << "\t" << setw(15) << "Current Line";
        newLog << left << "\t" << setw(15) << "Total Lines";
        newLog << left << "\t" << setw(17) << "Progress" << "\n";
        if(!processQueue.empty()){
            for(Console& c : processQueue){
                strftime(time, sizeof(time), "%m/%d/%Y, %I:%M:%S %p", &c.process.arrivalTimeStamp);
                strftime(timeS, sizeof(timeS), "%m/%d/%Y, %I:%M:%S %p", &c.process.timestamp);
                strftime(timeF, sizeof(timeF), "%m/%d/%Y, %I:%M:%S %p", &c.process.finishTimeStamp);
                percentage = ((double)c.process.currLine / (double)c.process.lineCount) * 100.00;
                newLog << left << setw(4) << c.process.pid;
                newLog << left << "\t" << setw(20) << c.process.pname;
                newLog << left << "\t" << setw(30) << time;
                newLog << left << "\t" << setw(30) << timeS;
                newLog << left << "\t" << setw(30) << timeF;
                newLog << left << "\t" << setw(15) << c.process.currLine;
                newLog << left << "\t" << setw(15) << c.process.lineCount;
                newLog << right << "\t" << setw(3) << percentage;
                newLog << left << "% [";
                for(int i = 0; i < percentage / 10; i++){
                    newLog << "#";
                }
                for(int i = percentage / 10; i < 9; i++){
                    newLog << "-";
                }   
                newLog << "]" << "\n";
            }
        }
        else{
            newLog << "No processes to be listed." << "\n";
        }
        newLog << "\n" << "\n";

        newLog << "Running processes" << "\n";
        newLog << left << setw(4) << "PID";
        newLog << left << "\t" << setw(20) << "Name";
        newLog << left << "\t" << setw(30) << "Time arrived";
        newLog << left << "\t" << setw(30) << "Time started";
        newLog << left << "\t" << setw(8)  << "Core";
        newLog << left << "\t" << setw(15) << "Current Line";
        newLog << left << "\t" << setw(15) << "Total Lines";
        newLog << left << "\t" << setw(17) << "Progress" << "\n";
        if(!runningProcesses.empty()){
            for(Console& c : runningProcesses){
                strftime(time, sizeof(time), "%m/%d/%Y, %I:%M:%S %p", &c.process.arrivalTimeStamp);
                strftime(timeS, sizeof(timeS), "%m/%d/%Y, %I:%M:%S %p", &c.process.timestamp);
                strftime(timeF, sizeof(timeF), "%m/%d/%Y, %I:%M:%S %p", &c.process.finishTimeStamp);
                percentage = ((double)c.process.currLine / (double)c.process.lineCount) * 100.00;
                newLog << left << setw(4) << c.process.pid;
                newLog << left << "\t" << setw(20) << c.process.pname;
                newLog << left << "\t" << setw(30) << time;
                newLog << left << "\t" << setw(30) << timeS;
                newLog << left << "\t" << setw(8) << c.process.core;
                newLog << left << "\t" << setw(15) << c.process.currLine;
                newLog << left << "\t" << setw(15) << c.process.lineCount;
                newLog << right << "\t" << setw(3) << percentage;
                newLog << left << "% [";
                for(int i = 0; i < percentage / 10; i++){
                    newLog << "#";
                }
                for(int i = percentage / 10; i < 9; i++){
                    newLog << "-";
                }   
                newLog << "]" << "\n";
            }
        }
        else{
            newLog << "No processes to be listed." << "\n";
        }
        newLog << "\n" << "\n";
        newLog << "Finished processes" << "\n";
        newLog << left << setw(4) << "PID";
        newLog << left << "\t" << setw(20) << "Name";
        newLog << left << "\t" << setw(30) << "Time arrived";
        newLog << left << "\t" << setw(30) << "Time started";
        newLog << left << "\t" << setw(30) << "Time finished";
        newLog << left << "\t" << setw(15) << "Current Line";
        newLog << left << "\t" << setw(15) << "Total Lines";
        newLog << left << "\t" << setw(17) << "Progress" << "\n";
        if(!finishedProcesses.empty()){
            for(Console& c : finishedProcesses){
                strftime(time, sizeof(time), "%m/%d/%Y, %I:%M:%S %p", &c.process.arrivalTimeStamp);
                strftime(timeS, sizeof(timeS), "%m/%d/%Y, %I:%M:%S %p", &c.process.timestamp);
                strftime(timeF, sizeof(timeF), "%m/%d/%Y, %I:%M:%S %p", &c.process.finishTimeStamp);
                percentage = ((double)c.process.currLine / (double)c.process.lineCount) * 100.00;
                newLog << left << setw(4) << c.process.pid;
                newLog << left << "\t" << setw(20) << c.process.pname;
                newLog << left << "\t" << setw(30) << time;
                newLog << left << "\t" << setw(30) << timeS;
                newLog << left << "\t" << setw(30) << timeF;
                newLog << left << "\t" << setw(15) << c.process.currLine;
                newLog << left << "\t" << setw(15) << c.process.lineCount;
                newLog << right << "\t" << setw(3) << percentage;
                newLog << left << "% [";
                for(int i = 0; i < percentage / 10; i++){
                    newLog << "#";
                }
                for(int i = percentage / 10; i < 9; i++){
                    newLog << "-";
                }   
                newLog << "]" << "\n";
            }
        }
        else{
            newLog << "No processes to be listed." << "\n";
        }
        newLog << "\n" << "\n";
        newLog << "===========================================================" << "\n";

        logFile << newLog.str();
        cout << "Report generated!" << endl;

    }

    void MainConsole::printProcessSMI() {
        // Clear screen
        system("cls");

        // Print CSOPESY header
        drawHeader();

        // CPU Utilization
        int numCores = cores.size();
        int usedCores = runningProcesses.size();
        double cpuUtil = (numCores > 0) ? ((double)usedCores / numCores) * 100.0 : 0;

        // Memory stats
        uint64_t totalMemBytes = memManager.maxMemory;
        uint64_t usedFrames = 0;
        for (auto &f : memManager.frames) {
            if (!f.pid.empty()) usedFrames++;
        }
        uint64_t usedMemBytes = usedFrames * memManager.memoryPerFrame;
        double memUtilPercent = (totalMemBytes > 0) ? ((double)usedMemBytes / totalMemBytes) * 100.0 : 0;

        // Convert to MiB
        double usedMiB = usedMemBytes / 1024.0 / 1024.0;
        double totalMiB = totalMemBytes / 1024.0 / 1024.0;

        // Print overview
        std::cout << "| PROCESS-SMI V01.00 Driver Version: 01.00 |\n";
        std::cout << "-------------------------------------------\n";
        std::cout << "CPU-Util: " << cpuUtil << "%\n";
        std::cout << "Memory Usage: " << usedMiB << "MiB / " << totalMiB << "MiB\n";
        std::cout << "Memory Util: " << memUtilPercent << "%\n\n";

        // List running processes
        std::cout << "Running processes and memory usage:\n";
        for (auto &procConsole : runningProcesses) {
            double procMemMiB = (procConsole.process.getMemorySize() * memPerFrame) / 1024.0 / 1024.0;
            std::cout << procConsole.process.pname << "\t" << procMemMiB << "MiB\n";
        }
    }

    void Console::handleInput(string s){
        //handle system calls first, then hand over the string to processcalls
        std::ostringstream output;

        s.erase(std::remove(s.begin(), s.end(), '\n'), s.cend());
        s.erase(std::remove(s.begin(), s.end(), '\r'), s.cend());

        char command[64] = {0}, arg1[64] = {0}, arg2[64] = {0}, arg3[64] = {0};

        if(s.find("PRINT") != string::npos || s.find("print") != string::npos){     //PRINT
            regex checkValid("(?:PRINT|print)\\s{0,1}\\((.*?)\\)");
            regex checkSolo("(.*)(?:print|PRINT)(.*)");
            if(std::regex_match(s, checkValid)){;
                char logTimeOut[30];
                time_t logTime;
                struct tm logTimeStamp;

                time(&logTime); 
                localtime_s(&logTimeStamp, &logTime);
                output << "(" << std::put_time(&logTimeStamp, "%m/%d/%Y %I:%M:%S%p") << ") Core:" << process.core << " " << regex_replace(s, checkValid, "$1") << endl;
                process.log.push_back(output.str());
            }
            else if(std::regex_match(s, checkSolo)){
                cmdPrintHelp();
            }
        }
        else if(sscanf(s.c_str(), "%s %s %s %s", command, arg1, arg2, arg3) == 4){
            if(strcmp(command, "add") == 0 || strcmp(command, "ADD") == 0){
                process.AddVars(arg1, arg2, arg3);
            }
            else if(strcmp(command, "subtract") == 0 || strcmp(command, "SUBTRACT") == 0){
                process.SubtractVar(arg1, arg2, arg3);
            }
            else   
                handleProcessCalls(s); //IS HERE BECAUSE IT CAPTURES SCREEN -S <PROC_NAME> <mem_size> and other 3 token commands
        }
        else if(sscanf(s.c_str(), "%s %s %s %s", command, arg1, arg2, arg3) == 3){
            if(strcmp(command, "declare") == 0 || strcmp(command, "DECLARE") == 0){
                process.AddToTableUsingIdentifier(arg1, arg2);
            }
            else if(strcmp(command, "read") == 0 || strcmp(command, "READ") == 0){
                process.ReadFromAddress(arg1, arg2);
            }
            else if(strcmp(command, "write") == 0 || strcmp(command, "WRITE") == 0){
                process.WriteToAddress(arg1, arg2);
            }
            else   
                handleProcessCalls(s); //IS HERE BECAUSE IT CAPTURES SCREEN -S <PROC_NAME> and other 3 token commands
        }
        /*
        if(sscanf("%s %s %s %s", command, arg1, arg2, arg3) && !valid3Arg){
            if(strcmp(command, "add") == 0 || strcmp(command, "ADD")){
                process.AddVars(arg1, arg2, arg3);
            }
            else if(strcmp(command, "add") == 0 || strcmp(command, "ADD")){
                process.AddVars(arg1, arg2, arg3);
            }
        }*/
        
        else if(s.find("SLEEP") != string::npos || s.find("sleep") != string::npos){         //SLEEP
            regex checkValid("(?:SLEEP|sleep)\\s{0,1}\\((.*?)\\)");
            regex checkSolo("(.*)(?:SLEEP|sleep)(.*)");
            if(std::regex_match(s, checkValid)){;
                //idk do something like sleep(std::stoi(regex_match(s, checkValid, "$1")));
            }
            else if(std::regex_match(s, checkSolo)){
                cmdSleepHelp();
            }
        }
        else if(s.find("FOR") != string::npos || s.find("for") != string::npos){         //FOR
            //I DON'T KNOW AHUHU
            //cmdForHelp();
        }
        else{
            handleProcessCalls(s);
        }
    }

    //HANDLING INPUT FOR THE BASE CONSOLE CLASS
    void Console::handleProcessCalls(string s){
        string placeholder = " command recognized. Doing something.";
        string invalid = "\" is not a recognized command";

        char* tokenizer = strtok(&s[0], " ");
        list<string> tokens;

        while(tokenizer != NULL){
            tokens.push_back(tokenizer);
            tokenizer = strtok(NULL, " "); // increment the tokenizer
        }
        
        if(!tokens.empty()){
            if(tokens.front() == "help" || tokens.front() == "?"){
                cmdHelp();
            }

            else if(tokens.front() == "exit"){
                exit = true;
            }

            else if(tokens.front() == "clear"){
                system("cls");
                drawHeader();
                printProcesses();
            }   

            else if(tokens.front() == "process-smi"){
                printSMI(); 
            }
            else{
                cout << "\"" << tokens.front() << invalid << endl;
            }
        }
    }

    //HANDLING INPUT FOR THE MAIN CONSOLE
    void MainConsole::handleProcessCalls(string s){
        string placeholder = " command recognized. Doing something.";
        string invalid = "\" is not a recognized command";

        char* tokenizer = strtok(&s[0], " ");
        list<string> tokens;

        while(tokenizer != NULL){
            tokens.push_back(tokenizer);
            tokenizer = strtok(NULL, " "); // increment the tokenizer
        }
        if(!tokens.empty()){
            if(tokens.front() == "help" || tokens.front() == "?"){
                cmdHelp();
            }
            else if (strcmp(tokens.front().c_str(), "scheduler-start") == 0) {
                stopProcessGenerator();
                startProcessGenerator();
                std::cout << "Process generation started.\n";
            } 

            else if (strcmp(tokens.front().c_str(), "scheduler-stop") == 0) {
                stopProcessGenerator();
                std::cout << "Process generation stopped.\n";
            }
            
            else if(tokens.front() == "exit"){
                exit = true;
            }
            else if(tokens.front() == "clear"){
                clear();
            }
            else if(tokens.front() == "initialize"){
                cout << s << placeholder << endl;
            }
            else if(tokens.front() == "screen"){
                //check for the flags
                tokens.pop_front(); //iterate to the next token, which is the flag (-s and -r)
                if(!tokens.empty() && (tokens.front() == "-s" || tokens.front() == "-r" || tokens.front() == "-ls")){
                    if(tokens.front() == "-s"){
                        tokens.pop_front(); //iterate to the process name;
                        if(!tokens.empty()){ //check if the string is null
                            //std::lock_guard<std::mutex> lock(queueMutex);
                            //addNewProcess(tokens.front());
                            //handoff = &processQueue.back();
                            //cout << handoff;
                            //clear();
                            string name = tokens.front();
                            if(!tokens.empty()){
                                tokens.pop_front(); //iterate to mem size
                                stopProcessGenerator();  //jic
                                startProcessGenerator(1, name, std::stoi(tokens.front()));
                            }
                            else
                                cmdScreenHelp();
                        }
                        else 
                            cmdScreenHelp();
                    }
                    else if(tokens.front() == "-r"){
                        tokens.pop_front(); //iterate to the process name;
                        if(!tokens.empty()){ //check if the string is null
                            handoff = searchList(tokens.front());
                            if(handoff) clear(); //If it finds something, clear the screen. If not, keep the screen.
                        }
                        else 
                            cmdScreenHelp();
                    }
                    else if(tokens.front() == "-ls"){
                        printProcesses();
                    }
                }
                else
                    cmdScreenHelp();
            }
            else if(tokens.front() == "report-util"){
                printProcessesToFile();
            }

            else if(tokens.front() == "vmstat") {
                printVMStat(memManager);
            }
            else if(tokens.front() == "process-smi"){ 
                printProcessSMI();
            }
            else{
                cout << "\"" << tokens.front() << invalid << endl;
            }
        }
    }
}


typedef struct {
    int num_cpu;
    char scheduler[10];
    int quantum_cycles;
    int batch_process_freq;
    int min_ins;
    int max_ins;
    int delay_per_exec;
    int max_overall_mem;
    int mem_per_frame;
    int min_mem_per_proc;
    int max_mem_per_proc;
} Config;

#endif