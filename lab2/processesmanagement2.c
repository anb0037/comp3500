
/*****************************************************************************\
* Laboratory Exercises COMP 3500                                              *
* Author: Saad Biaz                                                           *
* Updated 6/5/2017 to distribute to students to redo Lab 1                    *
* Updated 5/9/2017 for COMP 3500 labs                                         *
* Date  : February 20, 2009                                                   *
\*****************************************************************************/

/*****************************************************************************\
*                             Global system headers                           *
\*****************************************************************************/


#include "common2.h"

/*****************************************************************************\
*                             Global data types                               *
\*****************************************************************************/

typedef enum {TAT,RT,CBT,THGT,WT,WTJQ} Metric;
typedef enum {INFINITE,OMAP,BESTFIT,WORSTFIT,PAGING} MemoryPolicy;

/*****************************************************************************\
*                             Global definitions                              *
\*****************************************************************************/
#define MAX_QUEUE_SIZE 10
#define FCFS            1
#define RR              3


#define MAXMETRICS      6



/*****************************************************************************\
*                            Global data structures                           *
\*****************************************************************************/

typedef struct FreeMemoryBlock {
	struct FreeMemoryBlock *previous;
	struct FreeMemoryBlock *next;
	Memory topOfBlock;
	Memory sizeOfBlock;
} FreeMemoryBlock;

typedef struct MemoryBlockList {
	FreeMemoryBlock *Head;
	FreeMemoryBlock *Tail;
} MemoryBlockList;

/*****************************************************************************\
*                                  Global data                                *
\*****************************************************************************/

Quantity NumberofJobs[MAXMETRICS]; // Number of Jobs for which metric was collected
Average  SumMetrics[MAXMETRICS]; // Sum for each Metrics

MemoryBlockList FreeMemoryBlocks;
const MemoryPolicy policy = OMAP; // Policy selection
const int PageSize1 = 256;
const int PageSize2 = 8192;
int NumberOfAvailablePages1;//AvailableMemory expressed as pages (PS 256)
int NumberOfAvailablePages2;//AvailableMemory expressed as pages (PS 8192)
int NumberOfRequestedPages1; //MemoryRequested expressed as pages (PS 256)
int NumberOfRequestedPages2; //MemoryRequested expressed as pages (PS 256)*/


/*****************************************************************************\
*                               Function prototypes                           *
\*****************************************************************************/

void                 ManageProcesses(void);
void                 NewJobIn(ProcessControlBlock whichProcess);
void                 BookKeeping(void);
Flag                 ManagementInitialization(void);
void                 LongtermScheduler(void);
void                 IO();
void                 CPUScheduler(Identifier whichPolicy);
ProcessControlBlock *SRTF();
void                 Dispatcher();
Flag				 removeBlock(FreeMemoryBlock *freeMemoryBlock);
void 				 compactMemory();
ProcessControlBlock *getProcessAtAddress(Memory *address);
FreeMemoryBlock     *getFreeMemoryBlockAtAddress(Memory *address);

/*****************************************************************************\
* function: main()                                                            *
* usage:    Create an artificial environment operating systems. The parent    *
*           process is the "Operating Systems" managing the processes using   *
*           the resources (CPU and Memory) of the system                      *
*******************************************************************************
* Inputs: ANSI flat C command line parameters                                 *
* Output: None                                                                *
*                                                                             *
* INITIALIZE PROGRAM ENVIRONMENT                                              *
* START CONTROL ROUTINE                                                       *
\*****************************************************************************/

int main (int argc, char **argv) {
   if (Initialization(argc,argv)){
     ManageProcesses();
   }
} /* end of main function */

/***********************************************************************\
* Input : none                                                          *
* Output: None                                                          *
* Function: Monitor Sources and process events (written by students)    *\***********************************************************************/

void ManageProcesses(void){
  ManagementInitialization();
  while (1) {
    IO();
    CPUScheduler(PolicyNumber);
    Dispatcher();
  }
}

/***********************************************************************\
* Input : none                                                          *
* Output: None                                                          *
* Function:                                                             *
*    1) if CPU Burst done, then move process on CPU to Waiting Queue    *
*         otherwise (RR) return to rReady Queue                         *
*    2) scan Waiting Queue to find processes with complete I/O          *
*           and move them to Ready Queue                                *
\***********************************************************************/
void IO() {
  ProcessControlBlock *currentProcess = DequeueProcess(RUNNINGQUEUE);
 // int NumberOfAvailablePages1 = floor(AvailableMemory/256); //AvailableMemory expressed as pages (PS 256)
 // int NumberOfAvailablePages2 = floor(AvailableMemory/8192);//AvailableMemory expressed as pages (PS 8192)
 // int NumberOfRequestedPages1; //MemoryRequested expressed as pages (PS 256)
 // int NumberOfRequestedPages2; //MemoryRequested expressed as pages (PS 256)

  if (currentProcess){
    if (currentProcess->RemainingCpuBurstTime <= 0) { // Finished current CPU Burst
      currentProcess->TimeEnterWaiting = Now(); // Record when entered the waiting queue
      EnqueueProcess(WAITINGQUEUE, currentProcess); // Move to Waiting Queue
      currentProcess->TimeIOBurstDone = Now() + currentProcess->IOBurstTime; // Record when IO completes
      currentProcess->state = WAITING;
    } else { // Must return to Ready Queue
      currentProcess->JobStartTime = Now();
      EnqueueProcess(READYQUEUE, currentProcess); // Mobe back to Ready Queue
      currentProcess->state = READY; // Update PCB state
    }
  }

  /* Scan Waiting Queue to find processes that got IOs  complete*/
  ProcessControlBlock *ProcessToMove;
  /* Scan Waiting List to find processes that got complete IOs */
  ProcessToMove = DequeueProcess(WAITINGQUEUE);
  if (ProcessToMove){
    Identifier IDFirstProcess =ProcessToMove->ProcessID;
    EnqueueProcess(WAITINGQUEUE,ProcessToMove);
    ProcessToMove = DequeueProcess(WAITINGQUEUE);
    while (ProcessToMove){
      if (Now()>=ProcessToMove->TimeIOBurstDone){
	ProcessToMove->RemainingCpuBurstTime = ProcessToMove->CpuBurstTime;
	ProcessToMove->JobStartTime = Now();
	EnqueueProcess(READYQUEUE,ProcessToMove);
      } else {
	EnqueueProcess(WAITINGQUEUE,ProcessToMove);
      }
      if (ProcessToMove->ProcessID == IDFirstProcess){
	break;
      }
      ProcessToMove =DequeueProcess(WAITINGQUEUE);
    } // while (ProcessToMove)
  } // if (ProcessToMove)
}

/***********************************************************************\
 * Input : whichPolicy (1:FCFS, 2: SRTF, and 3:RR)                      *
 * Output: None                                                         *
 * Function: Selects Process from Ready Queue and Puts it on Running Q. *
\***********************************************************************/
void CPUScheduler(Identifier whichPolicy) {
  ProcessControlBlock *selectedProcess;
  if ((whichPolicy == FCFS) || (whichPolicy == RR)) {
    selectedProcess = DequeueProcess(READYQUEUE);
  } else{ // Shortest Remaining Time First
    selectedProcess = SRTF();
  }
  if (selectedProcess) {
    selectedProcess->state = RUNNING; // Process state becomes Running
    EnqueueProcess(RUNNINGQUEUE, selectedProcess); // Put process in Running Queue
  }
}

/***********************************************************************\
 * Input : None                                                         *
 * Output: Pointer to the process with shortest remaining time (SRTF)   *
 * Function: Returns process control block with SRTF                    *
\***********************************************************************/
ProcessControlBlock *SRTF() {
  /* Select Process with Shortest Remaining Time*/
  ProcessControlBlock *selectedProcess, *currentProcess = DequeueProcess(READYQUEUE);
  selectedProcess = (ProcessControlBlock *) NULL;
  if (currentProcess){
    TimePeriod shortestRemainingTime = currentProcess->TotalJobDuration - currentProcess->TimeInCpu;
    Identifier IDFirstProcess =currentProcess->ProcessID;
    EnqueueProcess(READYQUEUE,currentProcess);
    currentProcess = DequeueProcess(READYQUEUE);
    while (currentProcess){
      if (shortestRemainingTime >= (currentProcess->TotalJobDuration - currentProcess->TimeInCpu)){
	EnqueueProcess(READYQUEUE,selectedProcess);
	selectedProcess = currentProcess;
	shortestRemainingTime = currentProcess->TotalJobDuration - currentProcess->TimeInCpu;
      } else {
	EnqueueProcess(READYQUEUE,currentProcess);
      }
      if (currentProcess->ProcessID == IDFirstProcess){
	break;
      }
      currentProcess =DequeueProcess(READYQUEUE);
    } // while (ProcessToMove)
  } // if (currentProcess)
  return(selectedProcess);
}

/***********************************************************************\
 * Input : None                                                         *
 * Output: None                                                         *
 * Function:                                                            *
 *  1)If process in Running Queue needs computation, put it on CPU      *
 *              else move process from running queue to Exit Queue      *
\***********************************************************************/
void Dispatcher() {
  double start;
  ProcessControlBlock *processOnCPU = Queues[RUNNINGQUEUE].Tail; // Pick Process on CPU
  if (!processOnCPU) { // No Process in Running Queue, i.e., on CPU
    return;
  }
  if(processOnCPU->TimeInCpu == 0.0) { // First time this process gets the CPU
    SumMetrics[RT] += Now()- processOnCPU->JobArrivalTime;
    NumberofJobs[RT]++;
    processOnCPU->StartCpuTime = Now(); // Set StartCpuTime
  }

  if (processOnCPU->TimeInCpu >= processOnCPU-> TotalJobDuration) { // Process Complete
    printf(" >>>>>Process # %d complete, %d Processes Completed So Far <<<<<<\n",
	  processOnCPU->ProcessID,NumberofJobs[THGT]);
    processOnCPU=DequeueProcess(RUNNINGQUEUE);
    EnqueueProcess(EXITQUEUE,processOnCPU);
    if (policy == OMAP) {
        AvailableMemory += processOnCPU->MemoryAllocated;
        printf(" >> deallocated %d from %d, %d AvailableMemory\n", processOnCPU->MemoryAllocated, processOnCPU->ProcessID, AvailableMemory);
        processOnCPU->MemoryAllocated = 0;
    } else if (policy == BESTFIT || policy == WORSTFIT) {
        // create a new free memory block where the process used to be
      AvailableMemory += processOnCPU->MemoryAllocated;
      processOnCPU->MemoryAllocated = 0;
    	FreeMemoryBlock *newFreeMemoryBlock;
    	newFreeMemoryBlock->topOfBlock = processOnCPU->TopOfMemory;
    	newFreeMemoryBlock->sizeOfBlock = processOnCPU->MemoryAllocated;
    	FreeMemoryBlocks.Tail->next = newFreeMemoryBlock;
    	FreeMemoryBlocks.Tail = newFreeMemoryBlock;
    } else if (policy == PAGING) {
        NumberOfAvailablePages1 += NumberOfRequestedPages1;
        NumberOfAvailablePages2 += NumberOfRequestedPages2;
    }

    NumberofJobs[THGT]++;
    NumberofJobs[TAT]++;
    NumberofJobs[WT]++;
    NumberofJobs[CBT]++;
    SumMetrics[TAT]     += Now() - processOnCPU->JobArrivalTime;
    SumMetrics[WT]      += processOnCPU->TimeInReadyQueue;


    // processOnCPU = DequeueProcess(EXITQUEUE);
    // XXX free(processOnCPU);

    // Freed memory, so we should check if we have room for another process
    LongtermScheduler();

  } else { // Process still needs computing, out it on CPU
    TimePeriod CpuBurstTime = processOnCPU->CpuBurstTime;
    processOnCPU->TimeInReadyQueue += Now() - processOnCPU->JobStartTime;
    if (PolicyNumber == RR){
      CpuBurstTime = Quantum;
      if (processOnCPU->RemainingCpuBurstTime < Quantum)
	     CpuBurstTime = processOnCPU->RemainingCpuBurstTime;
    }
    processOnCPU->RemainingCpuBurstTime -= CpuBurstTime;
    // SB_ 6/4 End Fixes RR
    TimePeriod StartExecution = Now();
    OnCPU(processOnCPU, CpuBurstTime); // SB_ 6/4 use CpuBurstTime instead of PCB-> CpuBurstTime
    processOnCPU->TimeInCpu += CpuBurstTime; // SB_ 6/4 use CpuBurstTime instead of PCB-> CpuBurstTimeu
    SumMetrics[CBT] += CpuBurstTime;
  }
}

/***********************************************************************\
* Input : None                                                          *
* Output: None                                                          *
* Function: This routine is run when a job is added to the Job Queue    *
\***********************************************************************/
void NewJobIn(ProcessControlBlock whichProcess){
  ProcessControlBlock *NewProcess;
  /* Add Job to the Job Queue */
  NewProcess = (ProcessControlBlock *) malloc(sizeof(ProcessControlBlock));
  memcpy(NewProcess,&whichProcess,sizeof(whichProcess));
  NewProcess->TimeInCpu = 0; // Fixes TUX error
  NewProcess->RemainingCpuBurstTime = NewProcess->CpuBurstTime; // SB_ 6/4 Fixes RR
  EnqueueProcess(JOBQUEUE,NewProcess);
  DisplayQueue("Job Queue in NewJobIn",JOBQUEUE);
  LongtermScheduler(); /* Job Admission  */
}


/***********************************************************************\
* Input : None                                                         *
* Output: None                                                         *
* Function:                                                            *
* 1) BookKeeping is called automatically when 250 arrived              *
* 2) Computes and display metrics: average turnaround  time, throughput*
*     average response time, average waiting time in ready queue,      *
*     and CPU Utilization                                              *
\***********************************************************************/
void BookKeeping(void){
  double end = Now(); // Total time for all processes to arrive
  Metric m;

  // Compute averages and final results
  if (NumberofJobs[TAT] > 0){
    SumMetrics[TAT] = SumMetrics[TAT]/ (Average) NumberofJobs[TAT];
  }
  if (NumberofJobs[RT] > 0){
    SumMetrics[RT] = SumMetrics[RT]/ (Average) NumberofJobs[RT];
  }
  SumMetrics[CBT] = SumMetrics[CBT]/ Now();

  if (NumberofJobs[WT] > 0){
    SumMetrics[WT] = SumMetrics[WT]/ (Average) NumberofJobs[WT];
  }

  if (NumberofJobs[WTJQ] > 0){
    SumMetrics[WTJQ] = SumMetrics[WTJQ] / (Average) NumberofJobs[WTJQ];
  }

  printf("\n********* Processes Managemenent Numbers ******************************\n");
  printf("Policy Number = %d, Quantum = %.6f   Show = %d\n", PolicyNumber, Quantum, Show);
  printf("Number of Completed Processes = %d\n", NumberofJobs[THGT]);
  printf("ATAT=%f   ART=%f  CBT = %f  T=%f AWT=%f\n  AWTJQ=%f\n",
	 SumMetrics[TAT], SumMetrics[RT], SumMetrics[CBT],
	 NumberofJobs[THGT]/Now(), SumMetrics[WT], SumMetrics[WTJQ]);

  exit(0);
}

/***********************************************************************\
* Input : None                                                          *
* Output: None                                                          *
* Function: Decides which processes should be admitted in Ready Queue   *
*           If enough memory and within multiprogramming limit,         *
*           then move Process from Job Queue to Ready Queue             *
\***********************************************************************/
void LongtermScheduler(void){
  ProcessControlBlock *currentProcess = DequeueProcess(JOBQUEUE);
  while (currentProcess) {
    if (AvailableMemory >= currentProcess->MemoryRequested) {
       currentProcess->TimeInJobQueue = Now() - currentProcess->JobArrivalTime;
       SumMetrics[WTJQ] += currentProcess->TimeInJobQueue;
       NumberofJobs[WTJQ]++;
       currentProcess->JobStartTime = Now();
       EnqueueProcess(READYQUEUE, currentProcess);
       currentProcess->state = READY;

       if (policy == OMAP) {
           AvailableMemory -= currentProcess->MemoryRequested;
           currentProcess->MemoryAllocated = currentProcess->MemoryRequested;
           printf(" >> allocated %d to %d, %d AvailableMemory\n", currentProcess->MemoryAllocated, currentProcess->ProcessID, AvailableMemory);
       } else if (policy == BESTFIT) {
           struct FreeMemoryBlock* currentMemoryBlock = FreeMemoryBlocks.Head;
           struct FreeMemoryBlock* selectedMemoryBlock;
           while (currentMemoryBlock !=  NULL) {
           		// select the minimum of the blocks that are large enough to accomodate the new process 
       	   		printf("looking for memory block\n");
           		if (currentMemoryBlock->sizeOfBlock >= currentProcess->MemoryRequested && currentMemoryBlock->sizeOfBlock <= selectedMemoryBlock->sizeOfBlock) {
           			printf("selected memory block\n");
                selectedMemoryBlock = currentMemoryBlock;
           		}
           		currentMemoryBlock = currentMemoryBlock->previous;
           }
           if (selectedMemoryBlock != NULL) { // assign that process to the selected blocks
           	  printf("assigning process to memory block\n");
           		currentProcess->TopOfMemory = selectedMemoryBlock->topOfBlock;
           		selectedMemoryBlock->topOfBlock += currentProcess->MemoryRequested;
           		selectedMemoryBlock->sizeOfBlock -= currentProcess->MemoryRequested;
           		AvailableMemory -= currentProcess->MemoryRequested;
           		currentProcess->MemoryAllocated = currentProcess->MemoryRequested;
           }
       } else if (policy == PAGING) {
      	  /*int NumberOfAvailablePages1 = floor(AvailableMemory/256); //AvailableMemory expressed as pages (PS 256)
      	  int NumberOfAvailablePages2 = floor(AvailableMemory/8192);//AvailableMemory expressed as pages (PS 8192)
      	  int NumberOfRequestedPages1; //MemoryRequested expressed as pages (PS 256)
      	  int NumberOfRequestedPages2; //MemoryRequested expressed as pages (PS 256)*/

          NumberOfRequestedPages1 = ceil(currentProcess->MemoryRequested/256);
          NumberOfRequestedPages2 = ceil(currentProcess->MemoryRequested/8192);

          NumberOfAvailablePages1 -= NumberOfRequestedPages1;
          NumberOfAvailablePages2 -= NumberOfRequestedPages2;
       }
    }
    currentProcess = DequeueProcess(JOBQUEUE);
  }
}


/***********************************************************************\
* Input : None                                                          *
* Output: TRUE if Intialization successful                              *
\***********************************************************************/
Flag ManagementInitialization(void){
  Metric m;
  int NumberOfAvailablePages1 = floor(AvailableMemory/256); //AvailableMemory expressed as pages (PS 256)
  int NumberOfAvailablePages2 = floor(AvailableMemory/8192);//AvailableMemory expressed as pages (PS 8192)
  //int NumberOfRequestedPages1; //MemoryRequested expressed as pages (PS 256)
  //const int NumberOfRequestedPages2; //MemoryRequested expressed as pages (PS 256)*/
  for (m = TAT; m < MAXMETRICS; m++){
     NumberofJobs[m] = 0;
     SumMetrics[m]   = 0.0;
  }
  // Initialize double-linked list of free memory blocks
  FreeMemoryBlock memory;
  memory.topOfBlock = 0;
  memory.sizeOfBlock = AvailableMemory;
  FreeMemoryBlocks.Head = &memory;
  FreeMemoryBlocks.Tail = &memory;
}

/***********************************************************************\
* Input : pointer to the block to remove from the list                  *
* Output: if deletion was successful                                    *
\***********************************************************************/
Flag removeBlock(FreeMemoryBlock *freeMemoryBlock) {
	if (freeMemoryBlock != NULL) { // null check on parameter
		if (freeMemoryBlock->previous != NULL) {
			if (freeMemoryBlock->next != NULL) { // handles case where freeMemoryBlock is in between 2 others in the list
				freeMemoryBlock->previous->next = freeMemoryBlock->next;
				freeMemoryBlock->next->previous = freeMemoryBlock->previous;
				return TRUE;
			} else { // handles case where freeMemoryBlock is at the back of the list
				FreeMemoryBlocks.Tail = freeMemoryBlock->previous;
				freeMemoryBlock->previous->next = NULL;
				return TRUE;
			}
		} else if (freeMemoryBlock->next != NULL) { // handles case where freeMemoryBlock is at the front of the list
			FreeMemoryBlocks.Head = freeMemoryBlock->next;
			freeMemoryBlock->next->previous = NULL;
			return TRUE;
		} else {
			return FALSE;
		}

	}
	
}

/***********************************************************************\

* Checks the space after each free memory block                         *
* If it contains a process it compacts the process                      *
* else if it contains another free memory block it combines the blocks  *
* else it does nothing                                                  *
* Input : none                                                          *
* Output: none                                                          *
\***********************************************************************/
void compactMemory() {
	FreeMemoryBlock *currentMemoryBlock = FreeMemoryBlocks.Head; // loop this until there is only one free space?
	while(currentMemoryBlock != NULL) { // iterate through all free spaces

		Memory *addressOfNextBlock = & currentMemoryBlock->topOfBlock + currentMemoryBlock->sizeOfBlock;
		ProcessControlBlock *pcb = getProcessAtAddress(addressOfNextBlock); // get the PCB located immediately after this memory block, if there is one
		if (pcb != NULL) { // if a process is located immediately after this block
			// move the pcb to the top of the free memory block
			pcb->TopOfMemory = currentMemoryBlock->topOfBlock;
			// move the top of the free memory block to the address after the process
			currentMemoryBlock->topOfBlock += pcb->MemoryAllocated;
			printf(">> Compacted process %d\n", pcb->ProcessID);
		}
		FreeMemoryBlock *fmb = getFreeMemoryBlockAtAddress(addressOfNextBlock); // get the FMB located immediately after this memory block, if there is one
		if (fmb != NULL) { // if a free memory block is located immediately after this block
			//combine the 2 memory blocks
			currentMemoryBlock->sizeOfBlock += fmb->sizeOfBlock;
			removeBlock(fmb);
			printf(">> Combined two contiguous free spaces\n");
		}
		currentMemoryBlock = currentMemoryBlock->next;
	}
}

/***********************************************************************\
* Input : address to search for a process                               *
* Output: pointer to the process if there, null if not                  *
\***********************************************************************/
ProcessControlBlock *getProcessAtAddress(Memory *address) {
	// search ready queue
	ProcessControlBlock *pcb = Queues[READYQUEUE].Head;
	while (pcb != NULL) {
		if (pcb->TopOfMemory == *address) {
			return pcb;
		}
		pcb = pcb->next;
	}
	// search running queue
	pcb = Queues[RUNNINGQUEUE].Head;
	while (pcb != NULL) {
		if (pcb->TopOfMemory == *address) {
			return pcb;
		}
		pcb = pcb->next;
	}
	// search waiting queue
	pcb = Queues[WAITINGQUEUE].Head;
	while (pcb != NULL) {
		if (pcb->TopOfMemory == *address) {
			return pcb;
		}
		pcb = pcb->next;
	}
	return NULL;

} 

/***********************************************************************\
* Input : pointer to virtual address to search for a free memory block  *
* Output: pointer to the free memory block if there, null if not        *
\***********************************************************************/
FreeMemoryBlock *getFreeMemoryBlockAtAddress(Memory *address) {
	FreeMemoryBlock *fmb = FreeMemoryBlocks.Head;
	while (fmb != NULL) {
		if (fmb->topOfBlock == *address) {
			return fmb;
		}
		fmb = fmb->next;
	}
	return NULL;
}