#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
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
  int num_cpus = MAX_CPUS;  // Numero di CPU predefinito

  if (argc < 3) {
    fprintf(stderr, "Utilizzo: %s <numero_cpu> <file_processi>...\n", argv[0]);
    return 1;
  }

  num_cpus = atoi(argv[1]);  // Legge il numero di CPU dalla linea di comando
  if (num_cpus < 1 || num_cpus > MAX_CPUS) {
    fprintf(stderr, "Numero di CPU non valido. Deve essere compreso tra 1 e %d.\n", MAX_CPUS);
    return 1;
  }

  for (int i = 0; i < num_cpus; ++i) {
    FakeOS_init(&os[i]);
  }

  SchedSJFArgs sjf_args;
  sjf_args.a = 0.5;  // Coefficiente per la previsione quantistica
  sjf_args.initial_quantum = 5;  // Quantum iniziale
  for (int i = 0; i < num_cpus; ++i) {
    os[i].schedule_args = &sjf_args;
    os[i].schedule_fn = schedSJF;
  }

  for (int i = 2; i < argc; ++i) {
  FakeProcess new_process;
  int num_events = FakeProcess_load(&new_process, argv[i]);
  printf("Caricamento [%s], pid: %d, eventi: %d\n", argv[i], new_process.pid, num_events);
  if (num_events > 0) {
    FakeProcess* new_process_ptr = (FakeProcess*)malloc(sizeof(FakeProcess));
    *new_process_ptr = new_process;
    new_process_ptr->events.first = NULL;
    new_process_ptr->events.last = NULL;

    for (ListItem* event_item = new_process.events.first; event_item; event_item = event_item->next) {
      ProcessEvent* event = (ProcessEvent*)event_item;
      ProcessEvent* new_event = (ProcessEvent*)malloc(sizeof(ProcessEvent));
      *new_event = *event;
      List_pushBack(&new_process_ptr->events, (ListItem*)new_event);
    }

    int cpu_index = (i - 2) % num_cpus;
    List_pushBack(&os[cpu_index].processes, (ListItem*)new_process_ptr);
    
    // Aggiungi un evento fittizio per il processo se non ne ha
    if (!new_process_ptr->events.first) {
      ProcessEvent* dummy_event = (ProcessEvent*)malloc(sizeof(ProcessEvent));
      dummy_event->list.prev = dummy_event->list.next = NULL;
      dummy_event->type = CPU;
      dummy_event->duration = 1; // Durata minima, solo per evitare il "process without events"
      List_pushBack(&new_process_ptr->events, (ListItem*)dummy_event);
    }
  }
}

  for (int time = 0;; ++time) {
    printf("************** TEMPO: %08d **************\n", time);
    int any_active = 0;

    for (int i = 0; i < num_cpus; ++i) {
      FakeOS_simStep(&os[i]);

      if (os[i].running || os[i].ready.first || os[i].waiting.first || os[i].processes.first) {
        any_active = 1;
      }
    }

    if (!any_active)
      break;
  }

  // Deallocazione della memoria degli eventi processi
  for (int i = 0; i < num_cpus; ++i) {
    for (ListItem* process_item = os[i].processes.first; process_item; process_item = process_item->next) {
      FakeProcess* process = (FakeProcess*)process_item;
      while (process->events.first) {
        ProcessEvent* event = (ProcessEvent*)List_popFront(&process->events);
        free(event);
      }
      free(process);
    }
  }

  return 0;
}
