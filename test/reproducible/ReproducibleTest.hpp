#pragma once

#include <random>
#include <vector>

class Process;

extern std::vector<Process*> processes;
extern uint32_t size;
extern bool verbose;
extern std::set<Process*> runnable_processes;
extern int primary;
extern int max_proposals;
extern int num_proposals;
extern std::set<Process*> notify_processes;
extern uint32_t replicas;
