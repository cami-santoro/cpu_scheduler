#include "fake_process.h"
#include "linked_list.h"
#pragma once


typedef struct {
  ListItem list;
  int pid;
  ListHead events;
  int quantum;//ogni processo ha il proprio quanto
} FakePCB;

struct FakeOS;
struct FakeCPU;
typedef void (*ScheduleFn)(struct FakeOS* os, struct FakeCPU* cpu);

typedef struct FakeCPU{ //RAPPRESENTA UNA CPU
  int id;
  int id_process;
  FakePCB* running;
  ScheduleFn schedule_fn;
  int quantum;
} FakeCPU;

typedef struct FakeOS{
  FakePCB* running;
  ListHead ready;
  ListHead waiting;
  int timer;
  ScheduleFn schedule_fn;
  ListHead processes;
  int ncpu;
  FakeCPU *cpu;
} FakeOS;



void FakeCPU_init(FakeCPU* cpu);
void FakeOS_init(FakeOS* os, int ncpu);
void FakeOS_simStep(FakeOS* os);
void FakeOS_destroy(FakeOS* os);
