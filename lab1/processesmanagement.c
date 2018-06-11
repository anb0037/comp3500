/******************************************************************************\
* Laboratory Exercises COMP 3500                                              *
* Author: Saad Biaz                                                           *
* Updated 5/22/2017 to distribute to students to do Lab 1                     *
\*****************************************************************************/

/*****************************************************************************\
*                             Global system headers                           *
\*****************************************************************************/


#include "common.h"

/*****************************************************************************\
*                             Global data types                               *
\*****************************************************************************/

typedef enum {TAT,RT,CBT,THGT,WT} Metric;


/*****************************************************************************\
*                             Global definitions                              *
\*****************************************************************************/
#define MAX_QUEUE_SIZE 10
#define FCFS            1
#define SJF             2
#define RR              3


#define MAXMETRICS      5



/*****************************************************************************\
*                            Global data structures                           *
\*****************************************************************************/



/*****************************************************************************\
*                                  Global data                                *
\*****************************************************************************/

Quantity NumberofJobs[MAXMETRICS]; // Number of Jobs for which metric was collected
Average  SumMetrics[MAXMETRICS]; // Sum for each Metrics
Quantity QueueLength[MAXQUEUES]; // length of each queue

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
ProcessControlBlock *SJF_Scheduler();
ProcessControlBlock *FCFS_Scheduler();
ProcessControlBlock *RR_Scheduler();
void                 Dispatcher();

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
* Function: Monitor Sources and process events (written by students)    *
\***********************************************************************/

void ManageProcesses(void){
  ManagementInitialization();
  while (1) {
    IO();
    CPUScheduler(PolicyNumber);
    Dispatcher();
  }
}

/* XXXXXXXXX Do Not Change IO() Routine XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/***********************************************************************\
* Input : none                                                          *
* Output: None                                                          *
* Function:                                                             *
*    1) if CPU Burst done, then move process on CPU to Waiting Queue    *
*         otherwise (for RR) return Process to Ready Queue              *
*    2) scan Waiting Queue to find processes with complete I/O          *
*           and move them to Ready Queue                                *
\***********************************************************************/
void IO() {
  ProcessControlBlock *currentProcess = DequeueProcess(RUNNINGQUEUE);
  if (currentProcess){
    if (currentProcess->RemainingCpuBurstTime <= 0) { // Finished current CPU Burst
      currentProcess->TimeEnterWaiting = Now(); // Record when entered the waiting queue
      EnqueueProcess(WAITINGQUEUE, currentProcess); // Move to Waiting Queue
      QueueLength[WAITINGQUEUE]++;
      currentProcess->TimeIOBurstDone = Now() + currentProcess->IOBurstTime; // Record when IO completes
      currentProcess->state = WAITING;
    } else { // Must return to Ready Queue
      currentProcess->JobStartTime = Now();
      EnqueueProcess(READYQUEUE, currentProcess); // Mobe back to Ready Queue
      QueueLength[READYQUEUE]++;
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
    QueueLength[WAITINGQUEUE]++;
    ProcessToMove = DequeueProcess(WAITINGQUEUE);
    while (ProcessToMove){
      if (Now()>=ProcessToMove->TimeIOBurstDone){
	ProcessToMove->RemainingCpuBurstTime = ProcessToMove->CpuBurstTime;
	ProcessToMove->JobStartTime = Now();
	EnqueueProcess(READYQUEUE,ProcessToMove);
	QueueLength[READYQUEUE]++;
      } else {
	EnqueueProcess(WAITINGQUEUE,ProcessToMove);
        QueueLength[WAITINGQUEUE]++;
      }
      if (ProcessToMove->ProcessID == IDFirstProcess){
	break;
      }
      ProcessToMove =DequeueProcess(WAITINGQUEUE);
    } // while (ProcessToMove)
  } // if (ProcessToMove)
}

/***********************************************************************\
 * Input : whichPolicy (1:FCFS, 2: SJF, and 3:RR)                      *
 * Output: None                                                         *
 * Function: Selects Process from Ready Queue and Puts it on Running Q. *
\***********************************************************************/
void CPUScheduler(Identifier whichPolicy) {
  ProcessControlBlock  *selectedProcess;
  switch(whichPolicy){
    case FCFS : selectedProcess = FCFS_Scheduler();
      break;
    case SJF : selectedProcess = SJF_Scheduler();
      break;
    case RR   : selectedProcess = RR_Scheduler();
  }
  if (selectedProcess) {
  	if(selectedProcess->TimeInReadyQueue == 0) {
  		NumberofJobs[WT]++;
  	}
    selectedProcess->TimeInReadyQueue += Now() - selectedProcess->JobStartTime;
    selectedProcess->state = RUNNING; // Process state becomes Running
    EnqueueProcess(RUNNINGQUEUE, selectedProcess); // Put process in Running Queue
    QueueLength[RUNNINGQUEUE]++;
  }
}

/***********************************************************************\
 * Input : None                                                         *
 * Output: Pointer to the process based on First Come First Serve (FCFS)*
 * Function: Returns process control block based on FCFS                *
 \***********************************************************************/
ProcessControlBlock *FCFS_Scheduler() {
  /* Select Process based on FCFS */
  ProcessControlBlock *selectedProcess = Queues[READYQUEUE].Tail;
  ProcessControlBlock *topOfRunningQueue = Queues[RUNNINGQUEUE].Tail;
  if (selectedProcess == NULL  || topOfRunningQueue != NULL) {
    return NULL;
  }
  selectedProcess = DequeueProcess(READYQUEUE);
  QueueLength[READYQUEUE]--;
  return(selectedProcess);
}



/***********************************************************************\
 * Input : None                                                         *
 * Output: Pointer to the process with shortest remaining time (SJF)   *
 * Function: Returns process control block with SJF                    *
\***********************************************************************/
ProcessControlBlock *SJF_Scheduler() {
  /* Select Process with Shortest Remaining Time*/
  ProcessControlBlock *temp = Queues[READYQUEUE].Tail;
  ProcessControlBlock *topOfRunningQueue = Queues[RUNNINGQUEUE].Tail;
  ProcessControlBlock *selectedProcess = temp;
  if (temp == NULL || topOfRunningQueue != NULL) {
	return NULL;
  }
  TimePeriod minDuration = temp->TotalJobDuration;
  int index;
  for(index = 0; index < QueueLength[READYQUEUE]; index++) {
  	temp = DequeueProcess(READYQUEUE);
	if (temp->TotalJobDuration < minDuration) {
		selectedProcess = temp;
		minDuration = temp->TotalJobDuration;
	}
	EnqueueProcess(READYQUEUE, temp);
  }
  // cycle through items in queue until selected item is at the front
  while (selectedProcess != Queues[READYQUEUE].Tail) {
     EnqueueProcess(READYQUEUE, DequeueProcess(READYQUEUE));
  }
  // remove selected process from ready queue
  DequeueProcess(READYQUEUE);
  QueueLength[READYQUEUE]--;
  return(selectedProcess);
}


/***********************************************************************\
 * Input : None                                                         *
 * Output: Pointer to the process based on Round Robin (RR)             *
 * Function: Returns process control block based on RR                  *
 \***********************************************************************/
ProcessControlBlock *RR_Scheduler() {
  /* Select Process based on RR*/
  ProcessControlBlock *selectedProcess = Queues[READYQUEUE].Tail;
  ProcessControlBlock *topOfRunningQueue = Queues[RUNNINGQUEUE].Tail;
  if (selectedProcess == NULL || topOfRunningQueue != NULL) {
      return NULL;
  }
  if (Quantum < selectedProcess->CpuBurstTime) {
      selectedProcess->CpuBurstTime = Quantum;
  }
  DequeueProcess(READYQUEUE);
  QueueLength[READYQUEUE]--;
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
  ProcessControlBlock *pcb = Queues[RUNNINGQUEUE].Tail;
  if (pcb != NULL) { 
    if (pcb->TimeInCpu >= pcb->TotalJobDuration) { // handles processes that have completed
	    printf("pid %d exited\n",pcb->ProcessID);
	    pcb->JobExitTime = Now();
	    pcb->state = DONE;
	    NumberofJobs[THGT]++;
	    NumberofJobs[TAT]++;
		SumMetrics[TAT] += temp->JobExitTime - temp->JobArrivalTime;
        DequeueProcess(RUNNINGQUEUE);
        QueueLength[RUNNINGQUEUE]--;
        EnqueueProcess(EXITQUEUE, pcb);
        QueueLength[EXITQUEUE]++;
    }
    else if (PolicyNumber == 3) { // handles RR processes that aren't complete
        if (pcb->TimeInCpu == 0) {
        	pcb->StartCpuTime = Now();
    		NumberofJobs[RT]++;
			SumMetrics[RT] += temp->StartCpuTime - temp->JobArrivalTime;
        }
        OnCPU(pcb, pcb->CpuBurstTime);
    	pcb->TimeInCpu += pcb->CpuBurstTime;
    	printf("kicking off pid %d after quantum %f total time: %f\n",pcb->ProcessID, pcb->CpuBurstTime, pcb->TimeInCpu); 
        if (pcb->TimeInCpu <= pcb->TotalJobDuration) {
        	pcb->JobStartTime = Now();
        	DequeueProcess(RUNNINGQUEUE);
        	EnqueueProcess(READYQUEUE, pcb);
        	QueueLength[RUNNINGQUEUE]--;
        	QueueLength[READYQUEUE]++;
        }
    }
    else { // handles non-RR processes that still need computation
    	printf("adding %f to pid %d\n", pcb->CpuBurstTime, pcb->ProcessID);
    	if (pcb->TimeInCpu == 0) {
    		pcb->StartCpuTime = Now();
    		NumberofJobs[RT]++;
			SumMetrics[RT] += temp->StartCpuTime - temp->JobArrivalTime;
    	}
  		OnCPU(pcb, pcb->CpuBurstTime);
		pcb->TimeInCpu += pcb->CpuBurstTime;
    }
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
  QueueLength[JOBQUEUE]++;
  NewProcess->JobArrivalTime = Now();
  DisplayQueue("Job Queue in NewJobIn",JOBQUEUE);
  LongtermScheduler(); /* Job Admission  */
}

/**********************************************************************\
* Input: None                                                          *
* Output: None                                                         *
* Function:                                                            *
* 1) BookKeeping is called automatically when 250 arrived              *
* 2) Computes and display metrics: average turnaround  time, throughput*
*     average response time, average waiting time in ready queue,      *
*     and CPU Utilization                                              *
\***********************************************************************/
void BookKeeping(void){
  DisplayQueue("Job Queue", JOBQUEUE);
  DisplayQueue("Ready Queue: ", READYQUEUE);
  DisplayQueue("Running Queue: ", RUNNINGQUEUE);
  DisplayQueue("Waiting Queue: ", WAITINGQUEUE);
  DisplayQueue("Exit Queue", EXITQUEUE);
  double end = Now(); // Total time for all processes to arrive
  Metric m;
  // scan and record exit queue
  ProcessControlBlock *temp;
  int i;
  for(i = 0; i < QueueLength[EXITQUEUE]; i++) {
	 temp = Queues[EXITQUEUE].Tail;
     SumMetrics[CBT] += temp->TimeInCpu;
     SumMetrics[WT] += temp->TimeInReadyQueue;
     EnqueueProcess(EXITQUEUE, DequeueProcess(EXITQUEUE));
  }
  for(i = 0; i < QueueLength[WAITINGQUEUE]; i++) {
	 temp = Queues[WAITINGQUEUE].Tail;
     SumMetrics[CBT] += temp->TimeInCpu;
     SumMetrics[WT] += temp->TimeInReadyQueue;
     EnqueueProcess(WAITINGQUEUE, DequeueProcess(WAITINGQUEUE));
  }
  for(i = 0; i < QueueLength[RUNNINGQUEUE]; i++) {
	 temp = Queues[RUNNINGQUEUE].Tail;
     SumMetrics[CBT] += temp->TimeInCpu;
     SumMetrics[WT] += temp->TimeInReadyQueue;
     EnqueueProcess(RUNNINGQUEUE, DequeueProcess(RUNNINGQUEUE));
  }
  for(i = 0; i < QueueLength[READYQUEUE]; i++) {
	 temp = Queues[READYQUEUE].Tail;
     SumMetrics[CBT] += temp->TimeInCpu;
     SumMetrics[WT] += temp->TimeInReadyQueue;
     EnqueueProcess(READYQUEUE, DequeueProcess(READYQUEUE));
  }
  if (NumberofJobs[THGT] != 0) {
	SumMetrics[TAT] /= QueueLength[EXITQUEUE];
	SumMetrics[RT] /= NumberofJobs[RT];
	SumMetrics[WT] /= NumberofJobs[WT];
	SumMetrics[CBT] /= end;
  }

  printf("\n********* Processes Managemenent Numbers ******************************\n");
  printf("Policy Number = %d, Quantum = %.6f   Show = %d\n", PolicyNumber, Quantum, Show);
  printf("Number of Completed Processes = %d\n", NumberofJobs[THGT]);
  printf("ATAT=%f   ART=%f  CBT = %f  T=%f AWT=%f\n",
	 SumMetrics[TAT], SumMetrics[RT], SumMetrics[CBT],
	 NumberofJobs[THGT]/Now(), SumMetrics[WT]);

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
    QueueLength[JOBQUEUE]--;
    currentProcess->TimeInJobQueue = Now() - currentProcess->JobArrivalTime; // Set TimeInJobQueue
    currentProcess->JobStartTime = Now(); // Set JobStartTime
    EnqueueProcess(READYQUEUE,currentProcess); // Place process in Ready Queue
    QueueLength[READYQUEUE]++;
    currentProcess->state = READY; // Update process state
    currentProcess = DequeueProcess(JOBQUEUE);
  }
}


/***********************************************************************\
* Input : None                                                          *
* Output: TRUE if Intialization successful                              *
\***********************************************************************/
Flag ManagementInitialization(void){
  Queue n;
  for(n = JOBQUEUE; n < MAXQUEUES; n++) {
     QueueLength[n] = 0;
  }
  Metric m;
  for (m = TAT; m < MAXMETRICS; m++){
     NumberofJobs[m] = 0;
     SumMetrics[m]   = 0.0;
  }
  return TRUE;
}
