#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "fake_os.h"


void prediction(FakePCB* pcb, FakeOS* os) {
  int last_burst=pcb->last_burst;
  float last_prediction=pcb->last_prediction;
  pcb->burst_prediction=os->decay_coefficient*last_burst + (1-os->decay_coefficient)*last_prediction;
}

void FakeOS_init(FakeOS* os) {
  List_init(&os->running); //os->running=0;
  List_init(&os->ready);
  List_init(&os->waiting);
  List_init(&os->processes);
  os->timer=0;
  os->schedule_fn=0;
}

void FakeOS_createProcess(FakeOS* os, FakeProcess* p) {
  // sanity check
  assert(p->arrival_time==os->timer && "time mismatch in creation");
  // we check that in the list of PCBs there is no
  // pcb having the same pid
  FakePCB* run = (FakePCB*)os->running.first;
  assert( (! (os->running.first) || run->pid!=p->pid) && "pid taken");

  ListItem* aux=os->ready.first;
  while(aux){
    FakePCB* pcb=(FakePCB*)aux;
    assert(pcb->pid!=p->pid && "pid taken");
    aux=aux->next;
  }

  aux=os->waiting.first;
  while(aux){
    FakePCB* pcb=(FakePCB*)aux;
    assert(pcb->pid!=p->pid && "pid taken");
    aux=aux->next;
  }

  // all fine, no such pcb exists
  FakePCB* new_pcb=(FakePCB*) malloc(sizeof(FakePCB));
  new_pcb->list.next=new_pcb->list.prev=0;
  new_pcb->pid=p->pid;
  new_pcb->events=p->events;
  new_pcb->burst_prediction=0;
  new_pcb->last_burst=0;
  new_pcb->arrival_time=p->arrival_time;
  new_pcb->last_prediction=0;

  assert(new_pcb->events.first && "process without events");

  // depending on the type of the first event
  // we put the process either in ready or in waiting
  ProcessEvent* e=(ProcessEvent*)new_pcb->events.first;
  switch(e->type){ //Here it choose where the new process have to be
  case CPU:
    List_pushBack(&os->ready, (ListItem*) new_pcb);
    break;
  case IO:
    List_pushBack(&os->waiting, (ListItem*) new_pcb);
    break;
  default:
    assert(0 && "illegal resource");
    ;
  }
}




void FakeOS_simStep(FakeOS* os){
  
  printf("************** TIME: %08d **************\n", os->timer);

  //scan process waiting to be started
  //and create all processes starting now
  ListItem* aux=os->processes.first; //select the process loaded in the processes list in main
  while (aux){
    FakeProcess* proc=(FakeProcess*)aux;
    FakeProcess* new_process=0;
    if (proc->arrival_time==os->timer){
      new_process=proc;
    }
    aux=aux->next;
    if (new_process) { //it creates the process brought before
      printf("\tcreate pid:%d\n", new_process->pid);
      new_process=(FakeProcess*)List_detach(&os->processes, (ListItem*)new_process);
      FakeOS_createProcess(os, new_process);
      free(new_process); //we have put it in ready or waiting 
    }
  }

  // scan waiting list, and put in ready all items whose event terminates
  aux=os->waiting.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;
    ProcessEvent* e=(ProcessEvent*) pcb->events.first;
    printf("\twaiting pid: %d\n", pcb->pid);
    assert(e->type==IO);
    e->duration--;
    printf("\t\tremaining time:%d\n",e->duration);
    if (e->duration==0){
      printf("\t\tend IO burst\n");
      List_popFront(&pcb->events);
      free(e);
      List_detach(&os->waiting, (ListItem*)pcb);
      if (! pcb->events.first) {
        // kill process
        printf("\t\tend process\n");
        free(pcb);
      } else {
        //handle next event
        e=(ProcessEvent*) pcb->events.first;
        switch (e->type){
        case CPU:
          printf("\t\tmove to ready\n");
          List_pushBack(&os->ready, (ListItem*) pcb);
          break;
        case IO:
          printf("\t\tmove to waiting\n");
          List_pushBack(&os->waiting, (ListItem*) pcb);
          break;
        }
      }
    }
  }

  

  // decrement the duration of running
  // if event over, destroy event
  // and reschedule process
  // if last event, destroy running
  aux=os->running.first;
  while(aux) {
    FakePCB* pcb = (FakePCB*)aux;
    aux=aux->next;
    ProcessEvent* e=(ProcessEvent*) pcb->events.first;
    printf("\trunning pid: %d\n", pcb->pid);
    assert(e->type==CPU);
    e->duration--;
    printf("\t\tremaining time:%d\n",e->duration);
    if (e->duration==0){
      printf("\t\tend CPU burst\n");
      List_popFront(&pcb->events);
      free(e);
      if (! pcb->events.first) {
        printf("\t\tend process\n");
        assert(List_find(&os->running, (ListItem*) pcb));
        pcb = (FakePCB*)List_detach(&os->running, (ListItem*) pcb);
        free(pcb); // kill process
      } else {
        e=(ProcessEvent*) pcb->events.first;
        switch (e->type){  
        case CPU:
          printf("\t\tmove to ready\n");
          if (List_find(&os->running, (ListItem*) pcb))
            pcb = (FakePCB*)List_detach(&os->running, (ListItem*) pcb);
          pcb->last_burst=os->timer-pcb->arrival_time; //now I know how much the last burst was long
          prediction(pcb, os);// so now I can calculate how the prediction
          pcb->last_prediction=pcb->burst_prediction; //here I the new value of prediction as the last prediction registered
          List_pushBack(&os->ready, (ListItem*) pcb);
          break;
        case IO:
          printf("\t\tmove to waiting\n");
          if (List_find(&os->running, (ListItem*) pcb))
          pcb = (FakePCB*)List_detach(&os->running, (ListItem*) pcb);
          pcb->last_burst=os->timer-pcb->arrival_time; //now I know how much the last burst was long
          prediction(pcb, os);// so now I can calculate how the prediction
          pcb->last_prediction=pcb->burst_prediction; //here I the new value of prediction as the last prediction registered
          List_pushBack(&os->waiting, (ListItem*) pcb);
          break;
        }
        printf("\t\tLast burst of process %d: %d\n", pcb->pid, pcb->last_burst);
        printf("\t\tPrediction of process %d: %f\n\n", pcb->pid, pcb->burst_prediction);
      }
    }
  }

  // call schedule, if defined
  if (os->schedule_fn && List_size(&os->running)<os->num_cpus){
    (*os->schedule_fn)(os, os->schedule_args); 
  }

  // if running not defined and ready queue not empty
  // put the first in ready to run
  

  ++os->timer;

}

void FakeOS_destroy(FakeOS* os) {
    // Destroy the running processes
    while (!List_empty(&os->running)) {
        ListItem* item = List_popFront(&os->running);
        FakePCB* process = (FakePCB*)item;
        free(process);
    }

    // Destroy the ready processes
    while (!List_empty(&os->ready)) {
        ListItem* item = List_popFront(&os->ready);
        FakePCB* process = (FakePCB*)item;
        free(process);
    }

    // Destroy the waiting processes
    while (!List_empty(&os->waiting)) {
        ListItem* item = List_popFront(&os->waiting);
        FakePCB* process = (FakePCB*)item;
        free(process);
    }

    // Destroy the processes list
    while (!List_empty(&os->processes)) {
        ListItem* item = List_popFront(&os->processes);
        FakePCB* process = (FakePCB*)item;
        free(process);
    }
}
