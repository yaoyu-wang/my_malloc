#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>

typedef struct node_t
{
  intptr_t start;
  intptr_t end;
  struct node_t* next;
  struct node_t* prev;
  bool isFree;
} node_t;

typedef struct node_t node;

node* head = NULL;
unsigned long seg = 0;
unsigned long freeSeg = 0;

void init(){
  if (head == NULL){
    head = (node*)sbrk(sizeof(node));
    head -> start = (intptr_t)sbrk(0);
    head -> end = (intptr_t)sbrk(0);
    head -> next = NULL;
    head -> prev = NULL;
    head -> isFree = false;
  }
}

void bothFree (void *ptr){
  node* freed = (node*)((intptr_t)ptr - sizeof(node) - 1);
  freed -> isFree = true;
  freeSeg += (freed -> end - freed -> start) + 1 + sizeof(node);
  node* n = freed -> next;
  if (n != NULL){
    if (n -> isFree){
      freed -> next = n -> next;
      freed -> end = n -> end;
    }
  }
  node* cur = freed -> prev;
  if (cur -> isFree){
    cur -> next = freed -> next;
    cur -> end = freed -> end;
  }
}

void *ff_malloc (size_t size){
  init();
  node* cur = head;
  while(cur -> next != NULL){
    size_t curSize = cur -> end - cur -> start + 1;
    if (cur -> isFree && curSize >= size){
      if (curSize - size < sizeof(node) + 1){
        cur -> isFree = false;
        freeSeg -= (curSize + sizeof(node));
        return (void*)cur -> start;
      }
      node* n = (node*)(cur -> start + size);
      if (cur -> next != NULL){
        cur -> next -> prev = n;
      }
      n -> end = cur -> end;
      cur -> end = cur -> start + size - 1;
      cur -> isFree = false;
      n -> start = (intptr_t)n + sizeof(node) + 1;
      n -> isFree = true;
      n -> next = cur -> next;
      n -> prev = cur;
      cur -> next = n;
      freeSeg -= (size + sizeof(node));
      return (void*)cur -> start;
    }
    cur = cur -> next;
  }
  node* newBlk = (node*)sbrk(size + sizeof(node));
  newBlk -> start = (intptr_t)sbrk(0) - size + 1;
  newBlk -> end = (intptr_t)sbrk(0);
  newBlk -> isFree = false;
  newBlk -> next = NULL;
  newBlk -> prev = cur;
  cur -> next = newBlk;
  seg += (size + sizeof(node));
  return (void*)newBlk -> start;
}

void ff_free (void *ptr){
  bothFree(ptr);
}

void *bf_malloc (size_t size){
  init();
  int minFree = INT_MAX;
  node* res = head;
  node* cur = head;
  while(cur -> next != NULL){
    size_t curSize = cur -> end - cur -> start + 1;
    if (cur -> isFree
        && curSize >= size
        && curSize < minFree){
      minFree = curSize;
      res = cur;
    }
    cur = cur -> next;
  }
  if (res == head){
    node* newBlk = (node*)sbrk(size + sizeof(node));
    newBlk -> start = (intptr_t)sbrk(0) - size + 1;
    newBlk -> end = (intptr_t)sbrk(0);
    newBlk -> isFree = false;
    newBlk -> next = NULL;
    cur -> next = newBlk;
    newBlk -> prev = cur;
    seg += (size + sizeof(node));
    return (void*)newBlk -> start;
  }
  else if (minFree - size < sizeof(node)){
    res -> isFree = false;
    freeSeg -= (minFree + sizeof(node));
    return (void*)res -> start;
  }
  else{
    node* n = (node*)(res -> start + size);
    if (res -> next != NULL){
      res -> next -> prev = n;
    }
    n -> end = res -> end;
    res -> end = res -> start + size - 1;
    res -> isFree = false;
    n -> start = res -> end + 1 + sizeof(node) + 1;
    n -> isFree = true;
    n -> next = res -> next;
    res -> next = n;
    n -> prev = res;
    freeSeg -= (size + sizeof(node));
    return (void*)res -> start;
  }
}

void bf_free (void *ptr){
  bothFree(ptr);
}

unsigned long get_data_segment_size(){
  return seg;
}

unsigned long get_data_segment_free_space_size(){
  return freeSeg;
}
