#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "fake_os.h"

void FakeOS_init(FakeOS* os, int ncpu) {
  os->running=0;
  List_init(&os->ready);
  List_init(&os->waiting);
  List_init(&os->processes);
  os->timer=0;
  os->schedule_fn=0;
  os->cpu=malloc(ncpu * sizeof(FakeCPU));
  os->ncpu=ncpu;
}
void FakeCPU_init(FakeCPU* cpu) {
  cpu->id=0;
  cpu->running=NULL;//(ProcessEvent*) malloc(sizeof(ProcessEvent)); deve essere inizializzato dallo scheduler
  cpu->quantum=0;
  cpu->schedule_fn=0;
  cpu->id_process=-1; //deve essere inzializzato con l'id del processo di cui gli è stato assegnato un burst
}

void FakeOS_createProcess(FakeOS* os, FakeProcess* p) {
  // sanity check
  assert(p->arrival_time==os->timer && "time mismatch in creation");
  // we check that in the list of PCBs there is no
  // pcb having the same pid
  assert( (!os->running || os->running->pid!=p->pid) && "pid taken");

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
  new_pcb->quantum=p->quantum;

  assert(new_pcb->events.first && "process without events");

  // depending on the type of the first event
  // we put the process either in ready or in waiting
  ProcessEvent* e=(ProcessEvent*)new_pcb->events.first;
  switch(e->type){
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
  ListItem* aux=os->processes.first;
  while (aux){
    FakeProcess* proc=(FakeProcess*)aux;
    FakeProcess* new_process=0;
    if (proc->arrival_time==os->timer){
      new_process=proc;
    }
    aux=aux->next;
    if (new_process) {
      printf("\tcreate pid:%d\n", new_process->pid);
      new_process=(FakeProcess*)List_detach(&os->processes, (ListItem*)new_process);
      FakeOS_createProcess(os, new_process);
      free(new_process);
    }
  }

  // scan waiting list, and put in ready all items whose event terminates

  //IO RIMANE INVARIATO
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
      printf("\t\tend burst\n");
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

  //RUNNING CAMBIA  
  //originale
  /*
    FakePCB* running=os->running;
  printf("\trunning pid: %d\n", running?running->pid:-1);
  if (running) {
    ProcessEvent* e=(ProcessEvent*) running->events.first;
    assert(e->type==CPU);
    e->duration--;
    printf("\t\tremaining time:%d\n",e->duration);
    if (e->duration==0){
      printf("\t\tend burst\n");
      List_popFront(&running->events);
      free(e);
      if (! running->events.first) {
        printf("\t\tend process\n");
        free(running); // kill process
      } else {
        e=(ProcessEvent*) running->events.first;
        switch (e->type){
        case CPU:
          printf("\t\tmove to ready\n");
          List_pushBack(&os->ready, (ListItem*) running);
          break;
        case IO:
          printf("\t\tmove to waiting\n");
          List_pushBack(&os->waiting, (ListItem*) running);
          break;
        }
      }
      os->running = 0;
    }
  }
  */

  //MODIFICATO
  //FakePCB* running=os->running;
  for(int i=0;i<os->ncpu;i++){
    if(os->cpu[i].running!=NULL){
    ProcessEvent* running= (ProcessEvent*) os->cpu[i].running;
    printf("\t CPU %d running pid: %d\n", os->cpu[i].id,os->cpu[i].id_process);
    if (running==NULL) {
    assert(running->type==CPU);
    running->duration--;
    printf("\t\tremaining time:%d\n",running->duration);
    if (running->duration==0){
      printf("\t\tend burst\n");
      os->cpu[i].running=NULL;
      free(running);
    }
  }
    }
  else{
    printf("\t CPU %d running pid: %d\n", os->cpu[i].id,-1);
   (*os->cpu[i].schedule_fn) (os, &os->cpu[i]);
  }
     

  //if ( &os->cpu[i].running==NULL && os->ready.first) {
//    (*os->cpu[i].schedule_fn) (os->cpu[i]);
  //}

  }
  


  // call schedule, if defined ORIGINALE
  //if (os->schedule_fn && ! os->running){
  //  (*os->schedule_fn)(os, os->schedule_args); 
  //}



  ++os->timer;

}

void FakeOS_destroy(FakeOS* os) {
}