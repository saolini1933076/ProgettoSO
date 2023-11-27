#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "fake_os.h"
#include "fake_process.h"
#include "linked_list.h"

#define MAX_CPUS 4

FakeOS os[MAX_CPUS];

typedef struct {
  double a;
  int initial_quantum;
} SchedSJFArgs;

void schedSJF(FakeOS* os, void* args_) {
  SchedSJFArgs* args = (SchedSJFArgs*)args_;

  FakePCB* shortest_pcb = NULL;
  for (int i = 0; i < MAX_CPUS; ++i) {
    if (os[i].ready.first) {
      FakePCB* pcb = (FakePCB*)os[i].ready.first;
      if (!shortest_pcb || ((ProcessEvent*)pcb->events.first)->duration < ((ProcessEvent*)shortest_pcb->events.first)->duration) {
        shortest_pcb = pcb;
      }
    }
  }

  if (!shortest_pcb) return;

  int selected_cpu = -1;
  for (int i = 0; i < MAX_CPUS; ++i) {
    if (!os[i].running) {
      selected_cpu = i;
      break;
    }
  }

  if (selected_cpu == -1) {
    return;
  }

  FakePCB* pcb = shortest_pcb;
  os[selected_cpu].running = pcb;

  ProcessEvent* e = (ProcessEvent*)pcb->events.first;
  assert(e->type == CPU);
  int nq=args->a * e->duration + (1 - args->a) * args->initial_quantum;
  int new_quantum = round(nq);

  if (new_quantum > e->duration) {
    new_quantum = e->duration;
  }

  ProcessEvent* qe = (ProcessEvent*)malloc(sizeof(ProcessEvent));
  qe->list.prev = qe->list.next = NULL;
  qe->type = CPU;
  qe->duration = new_quantum;
  List_pushFront(&pcb->events, (ListItem*)qe);
}

void cleanup() {
  for (int i = 0; i < MAX_CPUS; ++i) {
    while (os[i].processes.first) {
      FakeProcess* process = (FakeProcess*)List_popFront(&os[i].processes);
      List_init(&process->events);
      free(process);
    }

    while (os[i].ready.first) {
      FakePCB* pcb = (FakePCB*)List_popFront(&os[i].ready);
      List_init(&pcb->events);
      free(pcb);
    }

    while (os[i].waiting.first) {
      FakePCB* pcb = (FakePCB*)List_popFront(&os[i].waiting);
      List_init(&pcb->events);
      free(pcb);
    }

    if (os[i].running) {
      List_init(&os[i].running->events);
      free(os[i].running);
    }
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <num_cpus> <process_file1> <process_file2> ...\n", argv[0]);
    return -1;
  }

  int num_cpus = atoi(argv[1]);
  if (num_cpus <= 0 || num_cpus > MAX_CPUS) {
    fprintf(stderr, "Number of CPUs must be between 1 and %d\n", MAX_CPUS);
    return -1;
  }

  for (int i = 0; i < num_cpus; ++i) {
    FakeOS_init(&os[i], i);
  }

  SchedSJFArgs sjf_args;
  sjf_args.a = 0.5;
  sjf_args.initial_quantum = 5;

  for (int i = 0; i < num_cpus; ++i) {
    os[i].schedule_args = &sjf_args;
    os[i].schedule_fn = schedSJF;
  }

  for (int i = 2; i < argc; ++i) {
    FakeProcess new_process;
    int num_events = FakeProcess_load(&new_process, argv[i]);
    printf("Caricamento [%s], pid: %d, eventi: %d\n", argv[i], new_process.pid, num_events);
    if (num_events > 0) {
      FakePCB* new_pcb = (FakePCB*)malloc(sizeof(FakePCB));
      new_pcb->list.prev = new_pcb->list.next = NULL;
      new_pcb->pid = new_process.pid;
      new_pcb->events = new_process.events;
      List_pushBack(&os[(i - 2) % num_cpus].ready, (ListItem*)new_pcb);
    }
  }

  int all_idle = 0;
  while (!all_idle) {
    printf("************** TEMPO: %08d **************\n", os[0].timer);
    all_idle = 1;
    for (int i = 0; i < num_cpus; ++i) {
      FakeOS_simStep(&os[i]);
      if (os[i].running || os[i].ready.first || os[i].waiting.first || os[i].processes.first) {
        all_idle = 0;
      }
    }
  }

  cleanup(); // Libera la memoria allocata dinamicamente

  return 0;
}
