#pragma once
#ifndef processH
#define processH

#include <string>
#include <ctime>
#include <iostream>
#include <list>
#include <map>
#include <fstream>
#include <vector>
#include "process.h"
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>

using std::vector;
using std::map;
using std::list;
using std::string;
using std::cout;
using std::endl;





namespace process{
	//#1
	struct heap{ int i = 1; };			//Template for heap struct.
	struct stack{ int i = 1; };			//Template for stack struct.
	struct symbolTable{ int i = 1; };	//Template for symbolTable struct.
	struct subroutine{ int i = 1; };	//Template for subroutine struct.
	struct library{ int i = 1; };		//Template for  struct.
	struct instruction{ int i = 1; };	//Template for instruction struct.

	class Process{
		public:
			int pid; 					//Process ID or PID: Numeric identifier for the process
			string pname = "error";		//Process Name: (Hopefully) easier-to-understand identifier for the process (i.e. subtraction.exe)
			time_t startTime; 			//Epoch time when the process was started. time_t is from the <ctime> library.
			struct tm timestamp;		//Used to hold the time data structure used by <ctime> that's more human-readable.
			time_t arrivalTime;			//Epoch time when the process arrived
			struct tm arrivalTimeStamp;	//More human-readable time stamp
			time_t finishTime;
			struct tm finishTimeStamp;
			int core; 					//Indicates which core the process is running on. (i.e. Core 1, Core 2.)
			int coreUtil;				//Indicates the utilization of the core. Is a percentage, range is 0 to 100.
			int lineCount = -1;			//The process' number lines of code.
			int currLine = 0;			//Which line of code the CPU is currently executing.
			vector<string> log;			//Log
			list<string> commands;		//List of commands that the process has to execute.

			list<int> frameIds;			//List of frame Ids that the process uses

			void incrementLine(){ //Function for incrementing current line and the instruction pointer.
				currLine++; 	//Increment the integer counter for line
				instructions++; //Go the next instruction in the memory.
			}; 			

			void start(int coreId){
				time(&startTime); //Log when the process was started
				localtime_s(&timestamp, &startTime); //Turn epoch time to calendar time
				core = coreId;
			}

			//maybe
			//void resume(){}

			void end(){
				time(&finishTime);
				localtime_s(&finishTimeStamp, &finishTime);
				core = -1;
			}
			/*	
			void logPrint() { //change this, maybe better in console
				std::string filename = pname + ".txt";

				// Check if the file is empty
				std::ifstream infile(filename);
				bool isEmpty = infile.peek() == std::ifstream::traits_type::eof();
				infile.close();

				std::ofstream logFile(filename, std::ios::app);

				if (isEmpty) {
					logFile << "Process name: " << pname << "\n\nLogs:\n\n";
				}
			
				time_t t;
				struct tm timeinfo;
				time(&t);
				localtime_s(&timeinfo, &t);

				std::ostringstream newLog;
				newLog << "(" << std::put_time(&timeinfo, "%m/%d/%Y %I:%M:%S%p")
					<< ") Core:" << core << " \"Hello world from " << pname << "!\"\n";

				logFile << newLog.str();

				log.push_back(newLog.str());
			}

			*/
			//Process constructors.
			Process(int instructionCount, int heapSize = 100000, int stackSize = 10000){
				time(&startTime); //Log when the process was started
				localtime_s(&timestamp, &startTime); //Turn epoch time to calendar time

				lineCount = instructionCount; //Set the line count

				//Allocate memory for the instructions/main program
				instructions = (instruction* )malloc(sizeof(instruction) * instructionCount);
				if(instructions == NULL) cout << "Error allocating memory for instructions" << endl; 
					
				//Allocate memory for the heap
				pHeap = (heap* )malloc(sizeof(heap) * heapSize);
				if(pHeap == NULL) cout << "Error allocating memory for the heap" << endl; 

				//Allocate memory for the stack
				pStack = (stack* )malloc(sizeof(stack) * stackSize);
				if(pStack == NULL) cout << "Error allocating memory for stack" << endl;
			}	

			Process(string name, int lines, int id){
				time(&startTime); //Log when the process was started
				localtime_s(&timestamp, &startTime); //Turn epoch time to calendar time
				pname = name;
				pid = id;
				lineCount = lines;
			}

			Process(string name, int id) : pname(name), pid(id){
				time(&arrivalTime); //Log when the process was started
				localtime_s(&arrivalTimeStamp, &arrivalTime); //Turn epoch time to calendar time
				lineCount = rand() % (200 - 50 + 1) + 50; //picks a random linecount between 50 and 200 UPDATE TO TAKE FROM THE CONFIG INSTEAD
			}

			Process(string name, int id, int minins, int maxins) : pname(name), pid(id){
				time(&arrivalTime); //Log when the process was started
				localtime_s(&arrivalTimeStamp, &arrivalTime); //Turn epoch time to calendar time
				lineCount = rand() % (maxins - minins + 1) + minins; 
					int k = 0;
				for(int i = 0; i < lineCount; i++){
					k = rand() % 7;
					switch(k){
						case 0:
							commands.push_back("print(Hello World from " + pname + ")");
							break;
						case 1:
							commands.push_back("declare(var,value)");
							break;
						case 2:
							commands.push_back("add(var1,var2,var3)");
							break;
						case 3:
							commands.push_back("subtract(var1,var2,var3)");
							break;
						case 4:
							commands.push_back("sleep(50)");
							break;
						case 5:
							commands.push_back("for([declare(var,value)],3))");
							break;
						}
				}

			}

			Process(){
				
			}; //default Constructor

		private:
			heap* pHeap;				//Pointer for the heap
			stack* pStack;				//Pointer for the stack
			symbolTable* pSymbolTable;	//Pointer for the symbol table
			subroutine* pSubRoutine;	//Pointer for the subroutine
			library* libraries;			//Pointer for the libraries
			instruction* instructions; 	//Pointer for the lines of code/instructions of the process.
	};
}

#endif