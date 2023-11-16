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
  // Scansiona le CPU e trova la CPU con il prossimo processo più breve pronto
  FakePCB* shortest_pcb = NULL;
  for (int i = 0; i < MAX_CPUS; ++i) {
    if (os[i].ready.first) {
      FakePCB* pcb = (FakePCB*)os[i].ready.first;
      if (!shortest_pcb || ((ProcessEvent*)pcb->events.first)->duration < ((ProcessEvent*)shortest_pcb->events.first)->duration) {
        shortest_pcb = pcb;
      }
    }
  }

  // Se non ci sono processi in ready, esci
  if (!shortest_pcb) return;

  // Prendi la CPU che ospiterà il prossimo processo più breve
  int selected_cpu = -1;
  for (int i = 0; i < MAX_CPUS; ++i) {
    if (!os[i].running) {
      selected_cpu = i;
      break;
    }
  }

  // Se non ci sono CPU libere, non fare nulla
  if (selected_cpu == -1) {
    return;
  }
  FakePCB* pcb = shortest_pcb;
  os[selected_cpu].running = pcb;

  ProcessEvent* e = (ProcessEvent*)pcb->events.first;
  assert(e->type == CPU);

  // Calcola la nuova durata quantistica basata sulla formula
  int new_quantum = round(args->a * e->duration + (1 - args->a) * args->initial_quantum);
  // Se la durata quantistica è superiore alla durata residua del burst, usala
  if (new_quantum > e->duration) {
    new_quantum = e->duration;
  }

  // Aggiungi un nuovo evento CPU con la nuova durata quantistica
  ProcessEvent* qe = (ProcessEvent*)malloc(sizeof(ProcessEvent));
  qe->list.prev = qe->list.next = NULL;
  qe->type = CPU;
  qe->duration = new_quantum;
  List_pushFront(&pcb->events, (ListItem*)qe);
}

int main(int argc, char** argv) {
  int num_cpus = MAX_CPUS;

  for (int i = 0; i < num_cpus; ++i) {
    FakeOS_init(&os[i], num_cpus);
  }

  SchedSJFArgs sjf_args;
  sjf_args.a = 0.5;
  sjf_args.initial_quantum = 5;

  for (int i = 0; i < num_cpus; ++i) {
    os[i].schedule_args = &sjf_args;
    os[i].schedule_fn = schedSJF;
  }

  for (int i = 1; i < argc; ++i) {
    FakeProcess new_process;
    int num_events = FakeProcess_load(&new_process, argv[i]);
    printf("Caricamento [%s], pid: %d, eventi: %d\n", argv[i], new_process.pid, num_events);
    if (num_events > 0) {
      FakeProcess* new_process_ptr = (FakeProcess*)malloc(sizeof(FakeProcess));
      *new_process_ptr = new_process;
      int cpu_index = (i - 1) % num_cpus;  // Assegna i processi in modo ciclico alle CPU
      List_pushBack(&os[cpu_index].processes, (ListItem*)new_process_ptr);
    }
  }

  for (int time = 0;; ++time) {
    printf("************** TEMPO: %08d **************\n", time);
    for (int i = 0; i < num_cpus; ++i) {
      FakeOS_simStep(&os[i]);
    }

    // Esci se tutte le CPU sono inattive e non ci sono processi in attesa
    int all_idle = 1;
    for (int i = 0; i < num_cpus; ++i) {
      if (os[i].running || os[i].ready.first || os[i].waiting.first || os[i].processes.first) {
        all_idle = 0;
        break;
      }
    }
    if (all_idle)
      break;
  }

  return 0;
}
