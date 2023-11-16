#include "fake_process.h"
#include "linked_list.h"
#pragma once

struct FakeOS;  // Dichiarazione forward di FakeOS

typedef struct {
  ListItem list;
  int pid;
  ListHead events;
} FakePCB;

typedef struct FakeOS {
  FakePCB* running;
  ListHead ready;
  ListHead waiting;
  int timer;
  void (*schedule_fn)(struct FakeOS* os, void* args);
  void* schedule_args;
  ListHead processes;
  int num_cpus;  
  struct FakeOS** cpus;
} FakeOS;

void FakeOS_init(FakeOS* os, int num_cpus);  // Rimuovi 'struct' dai parametri
void FakeOS_simStep(FakeOS* os);
void FakeOS_destroy(FakeOS* os);
