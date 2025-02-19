#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef int32_t  s32;
typedef int64_t  s64;

typedef uint8_t  u8;
typedef uint64_t u64;

typedef struct Actor Actor;
typedef void compute_t(Actor*, void*);

struct Actor {
  u64 id;
  volatile s64 total_messages;
  volatile s64 write_index;
  u64 read_index;

  char* mailbox[100];
  u64 mailbox_capacity;
  u64 state;

  compute_t* compute;
};

u64 global_actor_id = 0;
Actor* global_actor_list[100] = {0};

Actor* create_actor(u64 mailbox_capacity, compute_t* compute) {
  Actor* actor = calloc(sizeof(Actor), 1);
  actor->mailbox_capacity = mailbox_capacity;
  for(u64 i = 0; i < mailbox_capacity; i++)
    actor->mailbox[i] = calloc(sizeof(char), 50);
  actor->compute = compute;
  actor->id = global_actor_id;
  global_actor_list[actor->id] = actor;
  global_actor_id++;
  return actor;
}

DWORD run_actor(void* args) {
  Actor* actor = (Actor*)args;
  while(true) {
    while(actor->total_messages > 0 && actor->read_index != (u64)actor->write_index) {
      actor->compute(actor, (void*)actor->mailbox[actor->read_index]);
      actor->read_index = (actor->read_index + 1) % actor->mailbox_capacity;
      actor->total_messages--;
    }
  }

  return 0;
}

void send_message(Actor* actor, char* message) {
  u64 write_index = InterlockedCompareExchange64(&actor->write_index, (actor->write_index + 1) % actor->mailbox_capacity, actor->write_index);
  strcpy(actor->mailbox[write_index], message);
  InterlockedIncrement64(&(actor->total_messages));
}

/// User defined
void compute_state_a(Actor* actor, void* message) {
  char* msg = (char*)message;
  printf("Computing state -> Actor ID: %lld MSG: %s\n", actor->id, msg);
  actor->state++;
  send_message(global_actor_list[actor->id+1], "Hello next actor");
}

void compute_state_b(Actor* actor, void* message) {
  char* msg = (char*)message;
  printf("Computing state -> Actor ID: %lld MSG: %s\n", actor->id, msg);
  actor->state++;
  send_message(global_actor_list[actor->id-1], "Hello previous actor");
}

int main() {
  Actor* tom_cruise = create_actor(100, &compute_state_a);
  Actor* jim_carrey = create_actor(100, &compute_state_b);

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