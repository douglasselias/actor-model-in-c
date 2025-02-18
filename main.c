#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct Actor Actor;
typedef void compute_t(Actor*, void*);

struct Actor {
  // These two could be generic data
  char* mailbox[100];
  int state;

  int id;
  int write_index;
  int read_index;
  int total_messages;
  compute_t* compute;
  HANDLE write_mutex;
};

int global_actor_id = 0;
Actor* global_actor_list[100] = {0};

Actor* create_actor(compute_t* compute) {
  Actor* actor = calloc(sizeof(Actor), 1);
  /// How to do with void* ? Maybe I could pass as an argument, like create_actor(compute, sizeof(MyData) * 2 /* size of mailbox, array of two MyData (Mydata is the message type) */, sizeof(MyState));
  for(int i = 0; i < 100; i++)
    actor->mailbox[i] = calloc(sizeof(char), 50);
  actor->compute = compute;
  actor->write_mutex = CreateMutex(NULL, false, NULL);
  actor->id = global_actor_id;
  global_actor_list[actor->id] = actor;
  global_actor_id++;
  return actor;
}

// Platform dependent
DWORD run_actor(void* args) {
  Actor* actor = (Actor*)args;
  while(true) {
    while(actor->total_messages > 0 && actor->read_index != actor->write_index) {
      actor->compute(actor, (void*)actor->mailbox[actor->read_index]);
      actor->read_index = (actor->read_index + 1) % 100;
      actor->total_messages--;
    }
  }

  return 0;
}

void send_message(Actor* actor, char* message) {
  int wait_result = WaitForSingleObject(actor->write_mutex, 1000);

  switch(wait_result) {
    case WAIT_OBJECT_0: {
      strcpy(actor->mailbox[actor->write_index], message);
      actor->write_index = (actor->write_index + 1) % 100;
      actor->total_messages++;
      break;
    }
    case WAIT_TIMEOUT: {
      printf("Failed to send message to actor id %d", actor->id);
      break;
    }
  }

  ReleaseMutex(actor->write_mutex);
}

/// User defined
void compute_state_a(Actor* actor, void* message) {
  char* msg = (char*)message;
  printf("Computing state -> Actor ID: %d MSG: %s\n", actor->id, msg);
  actor->state++;
  send_message(global_actor_list[actor->id+1], "Hello next actor");
}

void compute_state_b(Actor* actor, void* message) {
  char* msg = (char*)message;
  printf("Computing state -> Actor ID: %d MSG: %s\n", actor->id, msg);
  actor->state++;
  send_message(global_actor_list[actor->id-1], "Hello previous actor");
}

int main() {
  Actor* tom_cruise = create_actor(&compute_state_a);
  Actor* jim_carrey = create_actor(&compute_state_b);

  send_message(tom_cruise, "Hello Tom.");
  
  HANDLE tom_cruise_thread = CreateThread(NULL, 0, run_actor, (void*)tom_cruise, 0, NULL);
  HANDLE jim_carrey_thread = CreateThread(NULL, 0, run_actor, (void*)jim_carrey, 0, NULL);

  // run_actor(tom_cruise);
  // run_actor(jim_carrey);

  int total_threads = 2;
  HANDLE threads[2] = {0};
  threads[0] = tom_cruise_thread;
  threads[1] = jim_carrey_thread;
  WaitForMultipleObjects(total_threads, threads, true, INFINITE);
}