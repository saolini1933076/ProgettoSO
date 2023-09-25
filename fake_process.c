#include <stdio.h>
#include <stdlib.h>
#include "fake_process.h"
#include "linked_list.h"

#define LINE_LENGTH 1024

FakeProcess* FakeProcess_findByPID(int pid, ListHead* process_list) {
  ListItem* aux = process_list->first;
  while (aux) {
    FakeProcess* proc = (FakeProcess*)aux;
    if (proc->pid == pid) {
      return proc;
    }
    aux = aux->next;
  }
  return NULL;
}

int FakeProcess_load(FakeProcess* p, const char* filename) {
  FILE* f = fopen(filename, "r");
  if (!f) {
    return -1;
  }

  char* buffer = NULL;
  size_t line_length = 0;
  p->pid = -1;
  p->arrival_time = -1;
  List_init(&p->events);

  while (getline(&buffer, &line_length, f) > 0) {
    // Process the input line
    if (sscanf(buffer, "PROCESS %d %d", &p->pid, &p->arrival_time) == 2) {
      // Verifica che il PID sia unico prima di procedere
      if (FakeProcess_findByPID(p->pid, &p->events) != NULL) {
        printf("Process with PID %d already exists, skipping.\n", p->pid);
        free(buffer);
        fclose(f);
        return -1;
      }
    } else if (sscanf(buffer, "CPU_BURST %d", &p->arrival_time) == 1) {
      // Creazione di un nuovo evento di tipo CPU burst
      ProcessEvent* e = (ProcessEvent*)malloc(sizeof(ProcessEvent));
      e->list.prev = e->list.next = NULL;
      e->type = CPU;
      e->duration = p->arrival_time;
      List_pushBack(&p->events, (ListItem*)e);
    } else if (sscanf(buffer, "IO_BURST %d", &p->arrival_time) == 1) {
      // Creazione di un nuovo evento di tipo IO burst
      ProcessEvent* e = (ProcessEvent*)malloc(sizeof(ProcessEvent));
      e->list.prev = e->list.next = NULL;
      e->type = IO;
      e->duration = p->arrival_time;
      List_pushBack(&p->events, (ListItem*)e);
    }
  }

  if (buffer) {
    free(buffer);
  }

  fclose(f);
  return List_size(&p->events);
}

int FakeProcess_save(const FakeProcess* p, const char* filename){
  FILE* f=fopen(filename, "w");
  if (! f)
    return -1;
  fprintf(f, "PROCESS %d %d\n", p->pid, p->arrival_time);
  ListItem* aux=p->events.first;
  int num_events;
  while(aux) {
    ProcessEvent* e=(ProcessEvent*) aux;
    switch(e->type){
    case CPU:
      fprintf(f, "CPU_BURST %d\n", e->duration);
      ++ num_events;
      break;
    case IO:
      fprintf(f, "IO_BURST %d\n", e->duration);
      ++ num_events;
      break;
    default:;
    }
    aux=aux->next;
  }
  fclose(f);
  return num_events;
}
  
