#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

static NthQueue *nth_fcfs1ReadyQueue;
  
static void nth_fcfs1SetReady(nThread th) {
  CHECK_CRITICAL("nth_fcfs1SetReady")
  
  if (th->status==READY || th->status==RUN)
    nFatalError("nth_fcfs1Ready", "The thread was already in READY status\n");

  th->status= READY;
  nth_putBack(nth_fcfs1ReadyQueue, th);
}

static void nth_fcfs1Suspend(State waitState) {
  CHECK_CRITICAL("nth_fcfs1Suspend")

  nThread th= nSelf();
  // if (th->status==READY)
  //   nth_delQueue(nth_fcfs1ReadyQueue, th);
  // else
  assert(th->status==RUN);

  th->status= waitState;
}

static void nth_fcfs1Schedule(void) {
  CHECK_CRITICAL("nth_fcfs1Schedule")
  
  nThread thisTh= nSelf();
  if (thisTh!=NULL) {
    if (thisTh->status==RUN)
      return;
    if (thisTh->status==READY) {
      nth_delQueue(nth_fcfs1ReadyQueue, thisTh);
      thisTh->status= RUN;
      return;
    }
  }

  nThread nextTh= nth_getFront(nth_fcfs1ReadyQueue);
  while (nextTh==NULL) {
    // No thread to execute, only a signal can wake up a thread,
    // but signals are disabled because this is a critical section,
    // so while waiting, they must be enabled
    nth_corePark();
    nextTh= nth_getFront(nth_fcfs1ReadyQueue);
  }

  // The context change: give this core to nextTh
  // it will take a while to return from here
  // Meanwhile thread nextTh and others are being executed
  
  nth_changeContext(thisTh, nextTh);
  
  // Some time later, at return the scheduler gave back onother core
  // to thisTh, but most probably coreId() != prevCoreId

  DBG(
    if (nth_selfCritical()!=thisTh)
      nFatalError("nth_fcfs1Schedule", "Not the right thread\n");
  );

  thisTh->status= RUN;
}

static void nth_fcfs1Stop(void) {
  CHECK_CRITICAL("nth_fcfs1Stop")
  
  nth_destroyQueue(nth_fcfs1ReadyQueue);
}

static Scheduler nth_fcfs1Scheduler= { .schedule = nth_fcfs1Schedule,
                               .setReady = nth_fcfs1SetReady,
                               .suspend = nth_fcfs1Suspend,
                               .stop = nth_fcfs1Stop };
void setFcfs1Scheduling() {
  START_CRITICAL

  if (nth_verbose)
    printf("Info: setting single-core FCFS scheduling\n");

  if (nth_totalCores!=1)
    nFatalError("setFcfs1Scheduling",
                "This FCFS scheduler only accepts a single core\n");

  nth_fcfs1ReadyQueue= nth_makeQueue();
  nth_setScheduler(nth_fcfs1Scheduler);
  MapIterator *iter= getMapIterator(nth_threadSet);
  void *ptr;
  while (mapNext(iter, &ptr, &ptr)) {
    nThread th= ptr;
    if (th->status==READY)
      nth_putBack(nth_fcfs1ReadyQueue, th);
  }
  destroyMapIterator(iter);
  
  END_CRITICAL
}

int isFcfs1Scheduling(void) {
  return nth_scheduler.schedule==nth_fcfs1Scheduler.schedule;
}
