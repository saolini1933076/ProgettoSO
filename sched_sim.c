#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdint.h> 
#include <stddef.h>
#include "fake_os.h"
#include "fake_process.h"
#include "linked_list.h"

#define MAX_CPUS 4 //Configurazione del numero massimo di CPU
FakeOS os[MAX_CPUS]; //Array di simulazioni per diverse CPU

typedef struct {
  double a; //Coefficiente per la previsione quantistica
  int initial_quantum; //Quanto iniziale per il processo
} SchedSJFArgs;

void schedSJF(FakeOS* os, void* args_){
  SchedSJFArgs* args=(SchedSJFArgs*)args_;
  //Scanziona le CPU e trova la CPU con il prossimo processo più breve pronto
  FakePCB* shortest_pcb=NULL;
  for (int i=0; i<MAX_CPUS; ++i){
    if(os[i].ready.first){
      FakePCB* pcb= (FakePCB*)os[i].ready.first;
      if(!shortest_pcb || ((ProcessEvent*)pcb->events.first)->duration<((ProcessEvent*)shortest_pcb->events.first)->duration){
        shortest_pcb=pcb;
      }
    }
  }

  //Se non ci sono processi in ready, esci
  if(!shortest_pcb) return;

  //Prendi la CPU che ospiterà il prossimo processo più breve
  int selected_cpu = -1;
  for(int i=0; i<MAX_CPUS; ++i){
    if(!os[i].running){
      selected_cpu = i;
      break;
    }
  }

  //Se non ci sono CPU libere, non fare nulla
  if(selected_cpu==-1){
    return;
  }
  FakePCB* pcb= shortest_pcb;
  os[selected_cpu].running=pcb;

  ProcessEvent* e= (ProcessEvent*) pcb->events.first;
  assert(e->type==CPU);

  //Calcola la nuova durata quantistica basata sulla formula
   int new_quantu = args->a * e->duration + (1 - args->a) * args->initial_quantum;
   int new_quantum=round(new_quantu);// Se la durata quantistica è superiore alla durata residua del burst, usala
  if (new_quantum > e->duration) {
    new_quantum = e->duration;
  }

  // Aggiungi un nuovo evento CPU con la nuova durata quantistica
  if (pcb->events.first) {
    ProcessEvent* qe = (ProcessEvent*)malloc(sizeof(ProcessEvent));
    qe->list.prev = qe->list.next = NULL;
    qe->type = CPU;
    qe->duration = new_quantum;
    List_pushFront(&pcb->events, (ListItem*)qe);
  }
}

int main(int argc, char** argv) {
  for (int i = 0; i < MAX_CPUS; ++i) {
    FakeOS_init(&os[i]);
  }

  SchedSJFArgs sjf_args;
  sjf_args.a = 0.5;  // Coefficient for quantum prediction
  sjf_args.initial_quantum = 5;  // Initial quantum
  for (int i = 0; i < MAX_CPUS; ++i) {
    os[i].schedule_args = &sjf_args;
    os[i].schedule_fn = schedSJF;
  }

  // Create an array to keep track of the current process index for each CPU
  int cpu_process_index[MAX_CPUS] = {0};

  // Load and distribute processes to CPUs
  for (int i = 1; i < argc; ++i) {
    FakeProcess new_process;
    int num_events = FakeProcess_load(&new_process, argv[i]);
    printf("Loading [%s], pid: %d, events: %d\n", argv[i], new_process.pid, num_events);
    if (num_events > 0) {
      FakeProcess* new_process_ptr = (FakeProcess*)malloc(sizeof(FakeProcess));
      *new_process_ptr = new_process;
      int cpu_index = i % MAX_CPUS;  // Distribute processes to CPUs in a round-robin manner
      List_pushBack(&os[cpu_index].processes, (ListItem*)new_process_ptr);
    }
  }

  int all_idle = 0;
  int time = 0;

  while (!all_idle) {
    printf("************** TIME: %08d **************\n", time);
    all_idle = 1;

    for (int i = 0; i < MAX_CPUS; ++i) {
      if (os[i].running || os[i].ready.first || os[i].waiting.first || os[i].processes.first) {
        all_idle = 0;

        // Check if the current CPU has finished its process
        if (!os[i].running) {
          // Find the next process for this CPU in a round-robin manner
          int cpu_index = i;
          FakeProcess* next_process = NULL;
          while (!next_process) {
            next_process = (FakeProcess*)(uintptr_t)List_getByIndex(&os[cpu_index].processes, cpu_process_index[cpu_index]);

            int size= List_size(&os[cpu_index].processes);
            cpu_process_index[cpu_index] = (cpu_process_index[cpu_index] + 1) % size;
            if (next_process && next_process->arrival_time > time) {
              // Process hasn't arrived yet, skip it for now
              next_process = NULL;
            }
          }

          // Create a PCB for the process and add it to the ready queue
          if (next_process) {
            FakePCB* new_pcb = (FakePCB*)malloc(sizeof(FakePCB));
            new_pcb->pid = next_process->pid;
            new_pcb->events = next_process->events;
            List_pushBack(&os[cpu_index].ready, (ListItem*)new_pcb);
          }
        }

        FakeOS_simStep(&os[i]);
      }
    }

    time++;
  }

  return 0;
}
