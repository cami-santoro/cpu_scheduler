#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "fake_os.h"

FakeOS os;

typedef struct {
  int quantum;
} SchedRRArgs;

void schedRR(FakeOS* os, FakeCPU* cpu){
  //deve prendere i processi in os->ready e distribuirli in base al burst minore alle varie cpu
  // per organizzare la lista di ready si prende per ognuno dei processi presenti il quanto
  //e li ordina per quantum crescente

  //prima di assegnare un evento del quanto ad un cpu (ed eliminarlo dalla coda degli eventi del processo)
  //aggiorna il quanto di quel processo con la formula 1/2(tempoeff +1/2 quantum vecchio)

  //SchedRRArgs* args=(SchedRRArgs*)&cpu->quantum;

  // look for the first process in ready
  // if none, return
  if (! os->ready.first){
    return;
  }

    //ORDINO PER QUANTO CRESCENTE LA LISTA DI READY  
    ListItem* aux=os->ready.first;
    struct ListItem *node=NULL, *temp = NULL;
    ListItem* tempvar;//temp variable to store node data
    node = aux;
    //temp = node;//temp node to hold node data and next link
    while(node)
    {
        temp=node; 
        FakePCB* temporaneo= (FakePCB*) temp;
        FakePCB* temporaneo_prossimo= (FakePCB*) temp->next;
        while (temp->next !=NULL)//travel till the second last element 
        {
           if(temporaneo->quantum > temporaneo_prossimo->quantum)// compare the data of the nodes 
            {
              tempvar = temp;
              temp = temp->next;// swap the data
              temp->next= tempvar;
            }
         temp = temp->next;    // move to the next element 
        }
        node = node->next;    // move to the next node
    }
    //SBAGLIATO, LA FUNZIONE DEVE LAVORARE SOLO SULLA PRIMA CPU
      if(cpu->running==NULL){ //alloco un'istanza di ProcessEvent
          cpu->running=(ProcessEvent*) malloc(sizeof(ProcessEvent));
          FakePCB* processo=(FakePCB*) os->ready.first;
          ProcessEvent* e=(ProcessEvent*) processo->events.first;
          if(e->type==CPU){
          cpu->running=e; //tiriamo fuori l'evento
          cpu->id_process=e->process_pid;
          cpu->quantum=processo->quantum;
          int new_quantum=(int)0.5*(processo->quantum)+(int)0.5*(e->duration);
          *(&processo->quantum)= new_quantum;// aggiorno il quanto per la volta successiva
          printf("Quanto vecchio: %d\n, Durata Evento: %d\n, Nuovo quanto: %d\n ", processo->quantum, e->duration, new_quantum);
          List_pushBack(&os->ready, (ListItem*)processo);
          }
          else{
            List_pushBack(&os->waiting, (ListItem*) processo);
          }

      }
    
  /*
  FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
  os->running=pcb;
  
  assert(pcb->events.first);
  ProcessEvent* e = (ProcessEvent*)pcb->events.first;
  assert(e->type==CPU);

  // look at the first event
  // if duration>quantum
  // push front in the list of event a CPU event of duration quantum
  // alter the duration of the old event subtracting quantum
  if (e->duration>args->quantum) {
    ProcessEvent* qe=(ProcessEvent*)malloc(sizeof(ProcessEvent));
    qe->list.prev=qe->list.next=0;
    qe->type=CPU;
    qe->duration=args->quantum;
    e->duration-=args->quantum;
    List_pushFront(&pcb->events, (ListItem*)qe);
  
  }
  */
};

int main(int argc, char** argv) {
  //IL PRIMO ARGOMENTO DA PASSARE E' IL NUMERO DI CPU
  char* char_cpu= argv[1];
  char cpu_int=char_cpu[0];
  int ncpu = cpu_int - '0';
  printf("Numero di cpu scelti: %d\n", ncpu);
  //INIZIALIZZIAMO L'OS CON NCPU CPU
  FakeOS_init(&os, ncpu);
  //INIZIALIZZO LE ISTANZE DI CPU
  for(int i=0; i<ncpu; i++){
    FakeCPU_init(&os.cpu[i]);
    os.cpu[i].id=i+1;
    os.cpu[i].schedule_fn=schedRR;
    printf("Creazione cpu %d\n", os.cpu[i].id);
  }
  //IL QUANTO ADESSO E' UN ATTRIBUTO DEL PROCESSO

  //SchedRRArgs srr_args;
  //srr_args.quantum=5;
  //os.schedule_args=&srr_args;
  os.schedule_fn=schedRR;
  
  for (int i=2; i<argc; ++i){
    FakeProcess new_process;
    int num_events=FakeProcess_load(&new_process, argv[i]);
    printf("loading [%s], pid: %d, events:%d, quantum:%d\n",
           argv[i], new_process.pid, num_events, new_process.quantum);
    if (num_events) {
      FakeProcess* new_process_ptr=(FakeProcess*)malloc(sizeof(FakeProcess));
      *new_process_ptr=new_process;
      List_pushBack(&os.processes, (ListItem*)new_process_ptr);
    }
  }
  printf("num processes in queue %d\n", os.processes.size);
  while(os.running
        || os.ready.first
        || os.waiting.first
        || os.processes.first){
    FakeOS_simStep(&os);
  }
}
