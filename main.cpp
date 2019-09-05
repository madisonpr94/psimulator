/*
COSC 3360
Madison Pratt
Due 2018-02-23

This assignment simulates a process scheduling system
by responding to various "resource requests" provided
via standard input.
*/

#include <stdint.h>
#include <stdlib.h>

#include <iomanip>
#include <iostream>
#include <queue>

#include "event.h"
#include "process.h"

// This class processes input and maintains the simulation state
class Simulation
{
public:
    // Sets the initial state of the simulation to zero
    Simulation()
    {
        cores_total = 0;
        cores_available = 0;
        disk_in_use = false;
        input_in_use = false;

        completed_processes = 0;
        ssd_accesses = 0;
        ssd_active_time = 0;
        ssd_queue_time = 0;
        cpu_total_time = 0;

        current_timestep = 0;
    }

    // Ensures that all Processes are cleaned up
    ~Simulation()
    {
        // Delete up allocated processes
        for(std::vector<Process*>::iterator it = process_table.begin(); it != process_table.end(); it++)
        {
            delete *it;
        }
    }

    // Get input for the simulation from an input stream (in this case, cin)
    void processInput(std::istream& stream)
    {
        char tag[32];
        int numeric;
        Process* current = NULL; // The process which "owns" any requests we encounter

        while(stream >> tag >> numeric) {
            std::string str_tag(tag);

            if(str_tag == "NEW")
            {
                // Create the process
                current = new Process;
                current->arrival_time = numeric;
                process_table.push_back(current);

                // Create the arrival event
                Event* e = new Event;
                e->type = EVENT_ARRIVAL;
                e->timestep = numeric;
                e->process = current;
                event_queue.push(e);
            }

            else if(str_tag == "NCORES")
            {
                cores_total = numeric;
            }

            // All remaining (valid) inputs require an existing process
            else if(current == NULL)
            {
                // So if we don't have one, something's gone wrong and we cannot continue
                std::cerr << "Malformed input - A request was given with no owning process" << std::endl;
                exit(EXIT_FAILURE);
            }

            else if(str_tag == "CORE")
            {
                current->addRequest(REQUEST_CPU, numeric);
            }

            else if(str_tag == "SSD")
            {
                current->addRequest(REQUEST_SSD, numeric);
            }

            else if(str_tag == "INPUT")
            {
                current->addRequest(REQUEST_INPUT, numeric);
            }

            else
            {
                std::cerr << "Malformed input - Unrecognized request \"" << tag << "\"" << std::endl;
                exit(EXIT_FAILURE);
            }

        }

        if(cores_total == 0) {
            std::cerr << "Malformed input - could not identify NCORES" << std::endl;
            exit(EXIT_FAILURE);
        }
        cores_available = cores_total;
    }

    // Executes the event loop
    void runSimulation()
    {
        while(event_queue.size() > 0)
        {
            // Grab the next event
            Event* next_event = event_queue.top();
            event_queue.pop();

            current_timestep = next_event->timestep;

            switch(next_event->type)
            {
                case EVENT_ARRIVAL:
                    handleProcessArrival(next_event);
                    break;
                case EVENT_CORE_FREE:
                    handleCoreFreed(next_event);
                    break;
                case EVENT_DISK_FREE:
                    handleDiskFreed(next_event);
                    break;
                case EVENT_INPUT_FREE:
                    handleInputFreed(next_event);
                    break;
                default:
                    break;
            }

            // We are done with this event, free the memory
            delete next_event;
        }

        printSummary();
    }

private:
    // Helper method to create a new event and add it to the simulation event queue
    void createEvent(Process* p, EventType t)
    {
        Event* new_event = new Event;
        new_event->process = p;
        new_event->type = t;
        new_event->timestep = current_timestep + p->next()->duration;
        event_queue.push(new_event);
    }

    // If a free core is available, schedules the process on that core
    // Otherwise, the process is added to the ready queue
    void scheduleCore(Process* p)
    {
        // Ensure the process's next request is for CPU
        if(p->next()->type != REQUEST_CPU)
        {
            std::cerr << "[debug] CPU scheduling error (request queue mismatch!)" << std::endl;
        }

        // Determine core availability
        if(cores_available > 0)
        {
            cores_available--;
            p->state = STATE_RUNNING;
            
            // Create the event which occurs when this core is freed again
            createEvent(p, EVENT_CORE_FREE);

            // Track how long the core is used for statistics later
            cpu_total_time += p->next()->duration;
        }

        else
        {
            ready_queue.push(p);
            p->state = STATE_READY;
        }
    }

    // Creates an event for disk usage, or places the process in the queue if busy
    void scheduleDisk(Process* p)
    {
        // Ensure the process is requesting the disk
        if(p->next()->type != REQUEST_SSD)
        {
            std::cerr << "[debug] Disk scheduling error (request queue mismatch!)" << std::endl;
        }

        if(!disk_in_use)
        {
            disk_in_use = true;
            // Schedule event to free disk
            createEvent(p, EVENT_DISK_FREE);

            // Track access information for statistics
            ssd_accesses++;
            ssd_active_time += p->next()->duration;
        }
        else
        {
            // Calculate the total time spent in the disk queue
            ssd_queue_time -= current_timestep;
            disk_queue.push(p);
        }
        p->state = STATE_BLOCKED;
    }

    // Creates an event for input usage, or places the process in the queue if busy
    void scheduleInput(Process* p)
    {
        // Ensure the process is requesting input
        if(p->next()->type != REQUEST_INPUT)
        {
            std::cerr << "[debug] Input scheduling error (request queue mismatch!)" << std::endl;
        }

        if(!input_in_use)
        {
            input_in_use = true;
            
            // Schedule event to free input
            createEvent(p, EVENT_INPUT_FREE);
        }
        else
        {
            input_queue.push(p);
        }
        p->state = STATE_BLOCKED;
    }

    // Handles the arrival event for a new process
    void handleProcessArrival(Event* e)
    {
        std::cout << "Process " << e->process->id << " starts at time " << e->process->arrival_time << " ms" << std::endl;
        
        printProcesses();

        // This is unlikely to ever be needed, but protects us against processes with zero requests arriving
        if(!e->process->finished())
        {
            scheduleCore(e->process); // The first request is always the CPU
        }
        else
        {
            // Process is already done (no requests), terminate it
            e->process->state = STATE_TERMINATED;
            printTermination(e->process);
        }
    }

    // Callback for a core becoming available
    void handleCoreFreed(Event* e)
    {
        cores_available++;
        e->process->advance();

        // If the ready queue has processes waiting, assign them the free core immediately
        if(ready_queue.size() > 0)
        {
            scheduleCore(ready_queue.front());
            ready_queue.pop();
        }

        // If the process isn't finished, schedule the next request
        if(!e->process->finished())
        {
            if(e->process->next()->type == REQUEST_INPUT)
            {
                scheduleInput(e->process);
            }
            else if(e->process->next()->type == REQUEST_SSD)
            {
                scheduleDisk(e->process);
            }
            else
            {
                std::cerr << "Process " << e->process->id << " made an illegal request" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            // No remaining requests, terminate the process
            e->process->state = STATE_TERMINATED;
            printTermination(e->process);
        }
    }

    // Callback for disk free event
    void handleDiskFreed(Event* e)
    {
        disk_in_use = false;
        e->process->advance();

        // Check the disk queue
        if(disk_queue.size() > 0)
        {
            // Calculate the total time spent in the disk queue
            ssd_queue_time += current_timestep;
            scheduleDisk(disk_queue.front());
            disk_queue.pop();
        }

        if(!e->process->finished())
        {
            scheduleCore(e->process);
        }
        else
        {
            // No remaining requests, terminate the process
            e->process->state = STATE_TERMINATED;
            printTermination(e->process);
        }
    }

    // Callback for input free event
    void handleInputFreed(Event* e)
    {
        input_in_use = false;
        e->process->advance();

        // Check the input queue
        if(input_queue.size() > 0)
        {
            scheduleInput(input_queue.front());
            input_queue.pop();
        }

        if(!e->process->finished())
        {
            scheduleCore(e->process);
        }
        else
        {
            // No remaining requests, terminate the process
            e->process->state = STATE_TERMINATED;
            printTermination(e->process);
        }
    }

    // Lists all arrived processes and their states
    void printProcesses()
    {
        for(std::vector<Process*>::iterator it = process_table.begin(); it != process_table.end(); it++)
        {
            switch((*it)->state)
            {
                case STATE_RUNNING:
                    std::cout << "Process " << (*it)->id << " is RUNNING" << std::endl;
                    break;
                case STATE_READY:
                    std::cout << "Process " << (*it)->id << " is READY" << std::endl;
                    break;
                case STATE_BLOCKED:
                    std::cout << "Process " << (*it)->id << " is BLOCKED" << std::endl;
                    break;
                case STATE_TERMINATED:
                    std::cout << "Process " << (*it)->id << " is TERMINATED" << std::endl;
                    break;
                case STATE_DEAD:
                case STATE_NOT_ARRIVED: // Ignore processes which haven't arrived yet
                default:
                    break;
            }
        }
        std::cout << std::endl;
    }

    // Prints to standard output that a process has terminated, and flags that process to not be mentioned again
    void printTermination(Process* p)
    {
        std::cout << "Process " << p->id << " terminates at time " << current_timestep << " ms" << std::endl;
        printProcesses();
        p->state = STATE_DEAD;
        completed_processes++;
    }

    // Prints the summary of the simulation's final statistics
    void printSummary()
    {
        std::cout << std::fixed << std::setprecision(2) << "SUMMARY:" << std::endl
            << "Number of processes that completed: " << completed_processes << std::endl
            << "Total number of SSD accesses: " << ssd_accesses << std::endl
            << "Average SSD access time: " << ((float)(ssd_queue_time + ssd_active_time) / ssd_accesses) << " ms" << std::endl
            << "Total elapsed time: " << current_timestep << " ms" << std::endl
            << "Core utilization: " << ((float)cpu_total_time * 100 / current_timestep) << " percent" << std::endl
            << "SSD utilization: " << ((float)ssd_active_time * 100 / current_timestep) << " percent" << std::endl;
    }

    uint32_t current_timestep;

    uint32_t cores_total;
    uint32_t cores_available;
    bool disk_in_use;
    bool input_in_use;

    std::vector<Process*> process_table;

    std::priority_queue<Event*, std::vector<Event*>, Event> event_queue;

    std::queue<Process*> ready_queue;
    std::queue<Process*> disk_queue;
    std::queue<Process*> input_queue;

    // Statistics information
    uint32_t completed_processes;
    
    uint32_t ssd_accesses;
    int32_t ssd_queue_time;
    uint32_t ssd_active_time;

    uint32_t cpu_total_time;
};

int main(int argc, char* argv[])
{
    Simulation s;
    s.processInput(std::cin);
    s.runSimulation();
}