#include <iostream>
#include <iomanip>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <list>
#include <fstream> //for file operations

#include "console.h" //For the console class
#include "process.h"

using std::list;
using std::cout;
using std::cin;
using process::Process;
using console::Console;
using console::MainConsole;
using std::mutex;
using std::chrono::milliseconds;
using std::vector;
using std::thread;
using std::ref;


Config configSetup() {
    Config config;
    FILE* file = fopen("config.txt", "r");

    if (!file) {
        perror("Error opening config.txt");
        exit(EXIT_FAILURE);
    }

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        line[strcspn(line, "\r\n")] = 0;

        char key[64] = {0};
        char value[64] = {0};

        if (sscanf(line, "%s %63[^\n]", key, value) == 2) {
            // Strip trailing newlines/spaces from value
            value[strcspn(value, "\r\n")] = 0;

            // Debug print
            std::cout << "Key: [" << key << "], Value: [" << value << "]" << std::endl;

            if (strcmp(key, "num-cpu") == 0) {
                config.num_cpu = atoi(value);
            } else if (strcmp(key, "scheduler") == 0) {
                // Strip surrounding quotes if they exist
                size_t len = strlen(value);
                if (value[0] == '"' && value[len - 1] == '"') {
                    value[len - 1] = '\0'; // remove trailing quote
                    strncpy(config.scheduler, value + 1, sizeof(config.scheduler) - 1);
                } else {
                    strncpy(config.scheduler, value, sizeof(config.scheduler) - 1);
                }
                config.scheduler[sizeof(config.scheduler) - 1] = '\0';
            } else if (strcmp(key, "quantum-cycles") == 0) {
                config.quantum_cycles = atoi(value);
            } else if (strcmp(key, "batch-process-freq") == 0) {
                config.batch_process_freq = atoi(value);
            } else if (strcmp(key, "min-ins") == 0) {
                config.min_ins = atoi(value);
            } else if (strcmp(key, "max-ins") == 0) {
                config.max_ins = atoi(value);
            } else if (strcmp(key, "delays-per-exec") == 0) {
                config.delay_per_exec = atoi(value);
            } else if (strcmp(key, "max-overall-mem") == 0) {
                config.max_overall_mem = atoi(value);
            } else if (strcmp(key, "mem-per-frame") == 0) {
                config.mem_per_frame = atoi(value);
            } else if (strcmp(key, "min-mem-per-proc") == 0) {
                config.mem_per_proc = atoi(value);
            } else if (strcmp(key, "max-mem-per-proc") == 0) {
                config.mem_per_proc = atoi(value);
            } else {
                std::cerr << "Unknown config key: " << key << std::endl;
            }
        } else {
            std::cerr << "Malformed line in config.txt: " << line << std::endl;
        }
    }

    fclose(file);
    return config;
}


 void startScreen (){
	cout << "Hello User! Type \"initialize\" to start.\n" << endl;
	string input = "";
	while(strcmp(input.c_str(), "initialize") != 0){
		cout << ">root/>"; //prompt
		std::getline(cin, input);
	}
 }

int main(){
	startScreen(); //Start the screen to prompt the user to initialize the program


	std::thread sched;
	//file reading should be done here
	Config config = configSetup(); //This should read the config file and set the values accordingly
    MainConsole mainConsole(config.num_cpu, config.scheduler, config.quantum_cycles, 
                            config.batch_process_freq, config.min_ins, 
                            config.max_ins, config.delay_per_exec,
                            config.max_overall_mem, config.mem_per_frame, config.mem_per_proc);
	//MainConsole mainConsole(NUM_CPU, SCHEDULER, QUANTUM_CYCLES, BATCH_PROCESS_FREQ, MIN_INS, MAX_INS, DELAY_PER_EXEC);
	Console* console = &mainConsole; //holds the current active console, initialized to main Menu as it's the root
	Console* temp = NULL;
	
	string input; //input variable
	string path = ">root/>";

    bool uiRunning = true;
	
	//cores are started in mainConsole in construction
	

    // Start scheduler
	int processes = 0;


	if (strcmp(config.scheduler,"fcfs" )== 0) {
		cout << "Using FCFS scheduler." << endl;
		sched = thread(&MainConsole::FCFSscheduler, &mainConsole, ref(processes));

	} else if (strcmp(config.scheduler,"rr" )== 0) {
		cout << "Using Round Robin scheduler." << endl;
		sched = thread(&MainConsole::rrscheduler, &mainConsole, ref (processes));
	} else {
		std :: cerr << "Unknown scheduler in config.txt: " << config.scheduler << endl;
	}

	while(!(console->exit)){ //========This should be a thread by itself
		if(console->mainConsole)
			cout << path;
		else
			cout << path << console->process.pname << "/>";

		std::getline(cin, input);
		console->handleInput(input);
		if(console->handoff != NULL){
			temp = console->handoff;
			console->handoff = NULL; //switch the handoff back to null
			console = temp; //switch the current console to the handoff value
			console->clear();
		}
		else if(console->exit && !(console->mainConsole)){
			console->exit = false; //set that console's exit to false or it never let us back in
			console = &mainConsole; //set us back to main console
			console->clear();
		}
		std::this_thread::sleep_for(milliseconds(1)); 
	}

	// Wait for scheduler and cores to finish
	sched.join();
    for (auto& t : mainConsole.cores) t.join();

    return 0;
}

// Implementation for continuous process generation
void MainConsole::startProcessGenerator(int i, string s, int mem) {
    if (generatingProcesses) {
        std::cout << "Process generator already running.\n";
        return;
    }
    generatingProcesses = true;
    processGeneratorThread = std::thread(&MainConsole::processGeneratorLoop, this, i, s, mem);
    std::cout << "[scheduler -start] Process generator started.\n";
}

void MainConsole::stopProcessGenerator() {
    if (!generatingProcesses) {
        std::cout << "Process generator is not running.\n";
        return;
    }
    generatingProcesses = false;
    if (processGeneratorThread.joinable()) {
        processGeneratorThread.join();
    }
    std::cout << "[scheduler -stop] Process generator stopped.\n";
}

void MainConsole::processGeneratorLoop(int i, string s, int mem) {
    int numLoops = 0;
    while (generatingProcesses && (numLoops < i || i == 0)) {
        Console console;
        consoleMade++;
        if(i != 0)
            console.process = Process(s, consoleMade, minIns, maxIns);
        else
            console.process = Process("process_" + std::to_string(consoleMade), consoleMade, minIns, maxIns);
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            processQueue.push_back(console);
            //if(i != 0) this->handoff = &processQueue.back();
        }
        cv.notify_one();
        if(i != 0) numLoops++;
        //if(numLoops >= i && i != 0){
        //    generatingProcesses = false;
        //}

        std::this_thread::sleep_for(milliseconds(batchProcessFreq));
    }
}