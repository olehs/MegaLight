#include <TaskScheduler.h>

Scheduler runner;

Task t1(1, TASK_FOREVER, &buttonLoop, &runner);
Task t2(1, TASK_FOREVER, &relayLoop, &runner);
Task t3(1, TASK_FOREVER, &webLoop, &runner);
Task t4(1, TASK_FOREVER, &externalLoop, &runner); 

void buttonLoop() {
  jpList.allButtons()->check();
  buttonEM.processAllEvents();
}

void relayLoop() {
  relayEM.processAllEvents();
  jpList.allRelays()->check();
}

#define WBSIZE 1024
void webLoop()
{
  char webBuffer[WBSIZE];
  int buflen = WBSIZE;
  webserver.processConnection(webBuffer, &buflen);
}

void externalLoop()
{
  externalEM.processEvent();
}

void setupTasks()
{
  t1.enable();
  t2.enable();
  t3.enable();
  t4.enable();
  
  runner.init();
  
  runner.addTask(t1);
  runner.addTask(t2);
  runner.addTask(t3);
  runner.addTask(t4);
}

