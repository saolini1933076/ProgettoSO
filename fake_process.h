#pragma once
#include "linked_list.h"

struct FakeOS;

typedef enum {CPU = 0, IO = 1} ResourceType;

typedef struct {
  ListItem list;
  ResourceType type;
  int duration;
} ProcessEvent;

typedef struct {
  ListItem list;
  int pid;
  int arrival_time;
  ListHead events;
} FakeProcess;

int FakeProcess_load(FakeProcess* p, const char* filename);

int FakeProcess_save(const FakeProcess* p, const char* filename);
