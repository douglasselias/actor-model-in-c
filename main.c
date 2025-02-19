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
  u8* state;

  compute_t* compute;
  HANDLE thread;
};

u64 global_actor_id = 0;
Actor* global_actor_list[100] = {0};
DWORD run_actor(void* args);

Actor* create_actor(u64 state_size, u64 mailbox_capacity, compute_t* compute) {
  Actor* actor = calloc(sizeof(Actor), 1);
  actor->state = calloc(state_size, 1);
  actor->mailbox_capacity = mailbox_capacity;
  for(u64 i = 0; i < mailbox_capacity; i++)
    actor->mailbox[i] = calloc(sizeof(char), 50);
  actor->compute = compute;
  actor->id = global_actor_id;
  global_actor_list[actor->id] = actor;
  global_actor_id++;

  actor->thread = CreateThread(NULL, 0, run_actor, (void*)actor, CREATE_SUSPENDED, NULL);
  return actor;
}

void init_actor(Actor* actor) {
  ResumeThread(actor->thread);
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

void send_message(Actor* actor, u8* message, u64 message_size) {
  u64 write_index = InterlockedCompareExchange64(&actor->write_index, (actor->write_index + 1) % actor->mailbox_capacity, actor->write_index);
  memcpy(actor->mailbox[write_index], message, message_size);
  InterlockedIncrement64(&(actor->total_messages));
}

/// User defined
void compute_state_a(Actor* actor, void* message_arg) {
  char* message_received = (char*)message_arg;
  printf("Computing state -> Actor ID: %lld MSG: %s\n", actor->id, message_received);
  (*(u64*)actor->state)++;
  char* message = "Hello next actor";
  send_message(global_actor_list[actor->id+1], (u8*)message, strlen(message));
}

void compute_state_b(Actor* actor, void* message_arg) {
  char* message_received = (char*)message_arg;
  printf("Computing state -> Actor ID: %lld MSG: %s\n", actor->id, message_received);
  (*(u64*)actor->state)++;
  char* message = "Hello previous actor";
  send_message(global_actor_list[actor->id-1], (u8*)message, strlen(message));
}

int main() {
  Actor* tom_cruise = create_actor(sizeof(u64), 100, compute_state_a);
  Actor* jim_carrey = create_actor(sizeof(u64), 100, compute_state_b);
  
  char* message = "Hello Tom.";
  send_message(tom_cruise, (u8*)message, strlen(message));

  init_actor(tom_cruise);
  init_actor(jim_carrey);

  s32 total_threads = 2;
  HANDLE threads[2] = {0};
  threads[0] = tom_cruise->thread;
  threads[1] = jim_carrey->thread;
  WaitForMultipleObjects(total_threads, threads, true, INFINITE);
  // printf("State %lld\n", *(u64*)tom_cruise->state);
}