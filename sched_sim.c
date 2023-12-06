#include "fake_os.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define DECAY_COEFFICIENT_POSITION 1
#define SCHEDULING_POLICY_POSITION 2
#define QUANTUM_POSITION 3
#define PROCESS_FILES_START_POSITION 4

FakeOS os;

typedef struct { 
  int quantum;
} SchedArgs;

FakePCB* selectFakePCB(ListHead* listHead) {
    if (List_empty(listHead)) {
        return NULL;
    }

    ListItem* currentListItem = listHead->first;
    FakePCB* minBurstPCB = (FakePCB*)currentListItem;

    while (currentListItem != NULL) {
        FakePCB* pcb = (FakePCB*)currentListItem;
        if (pcb->burst_prediction < minBurstPCB->burst_prediction) {
            minBurstPCB = pcb;
        }
        currentListItem = currentListItem->next;
    }
    return minBurstPCB;
}

void customSchedSJF(FakeOS* fakeOS, void* arguments_) {
    SchedArgs* customArgs = (SchedArgs*)arguments_;

    // Look for the first process in ready
    // If none, return

    if (List_empty(&fakeOS->ready))
        return;

    while (!List_empty(&fakeOS->ready) && List_size(&fakeOS->running) < fakeOS->num_cpus) {
        FakePCB* selectedPCB = selectFakePCB(&fakeOS->ready);
        FakePCB* verification = (FakePCB*)List_detach(&fakeOS->ready, (ListItem*)selectedPCB);
        if (!verification) exit(-1);
        selectedPCB->arrival_time = fakeOS->timer;
        List_pushFront(&fakeOS->running, (ListItem*)selectedPCB);

        assert(selectedPCB->events.first);
        ProcessEvent* event = (ProcessEvent*)selectedPCB->events.first;
        assert(event->type == CPU);

        // Look at the first event
        // If duration > quantum
        // Push front in the list of events a CPU event of duration quantum
        // Alter the duration of the old event subtracting quantum
        if (event->duration > customArgs->quantum) {
            ProcessEvent* quantumEvent = (ProcessEvent*)malloc(sizeof(ProcessEvent));
            quantumEvent->list.prev = quantumEvent->list.next = 0;
            quantumEvent->type = CPU;
            quantumEvent->duration = customArgs->quantum;
            event->duration -= customArgs->quantum;
            List_pushFront(&selectedPCB->events, (ListItem*)quantumEvent);
        }
    }
}

int main(int argc, char** argv) {
    FakeOS customOS;
    FakeOS_init(&customOS);
    SchedArgs customSchedArgs;

    // Default quantum for preemption
    customSchedArgs.quantum = 10;  // Default quantum value

    if (argc <= PROCESS_FILES_START_POSITION) {
        printf("You must provide at least one process file.\n");
        exit(-1);
    }

    assert(atof(argv[DECAY_COEFFICIENT_POSITION]) != 0 && atof(argv[DECAY_COEFFICIENT_POSITION]) <= 1 &&
           "You have to digit a number between 0 and 1");
    customOS.decay_coefficient = atof(argv[DECAY_COEFFICIENT_POSITION]);

    customOS.num_cpus = /* Get the number of available CPUs in the system */ 4; // Change accordingly

    customOS.schedule_args = &customSchedArgs;
    customOS.schedule_fn = customSchedSJF;

    if (argc > QUANTUM_POSITION) {
        assert(atoi(argv[QUANTUM_POSITION]) != 0 && *argv[QUANTUM_POSITION] != '0' &&
               "You must insert a positive integer value for the quantum");
        customSchedArgs.quantum = atoi(argv[QUANTUM_POSITION]);
    }

    for (int i = PROCESS_FILES_START_POSITION; i < argc; ++i) {
        FakeProcess newFakeProcess;
        int numberOfEvents = FakeProcess_load(&newFakeProcess, argv[i]);
        if (numberOfEvents) {
            FakeProcess* newFakeProcessPtr = (FakeProcess*)malloc(sizeof(FakeProcess));
            *newFakeProcessPtr = newFakeProcess;
            List_pushBack(&customOS.processes, (ListItem*)newFakeProcessPtr);
        }
    }
    printf("num processes in queue %d\n", customOS.processes.size);
    while (customOS.running.first || customOS.ready.first || customOS.waiting.first || customOS.processes.first) {
        FakeOS_simStep(&customOS);
    }
    FakeOS_destroy(&customOS);
}
