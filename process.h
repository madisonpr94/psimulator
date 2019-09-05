/*
COSC 3360
Madison Pratt
Due 2018-02-23

This assignment simulates a process scheduling system
by responding to various "resource requests" provided
via standard input.
*/

#ifndef PROCESS_H_GUARD
#define PROCESS_H_GUARD

#include <stdint.h>
#include <queue>

typedef enum RequestType
{
    REQUEST_CPU,
    REQUEST_SSD,
    REQUEST_INPUT
} RequestType;

// Small struct to represent various requests a process can make
struct Request
{
    RequestType type;
    uint32_t duration;
};

typedef enum ProcessState
{
    STATE_NOT_ARRIVED,
    STATE_READY,
    STATE_RUNNING,
    STATE_BLOCKED,
    STATE_TERMINATED,
    STATE_DEAD // Prevents the process from appearing in the display table
} ProcessState;

// This structure represents processes to be simulated, along with their requests
struct Process
{
    // Assigns a new ID and sets the default state
    Process();

    // Ensures the request queue is properly cleaned up
    ~Process();

    // Constructs and attaches a request to the queue
    void addRequest(RequestType type, uint32_t duration);

    // Returns a pointer to the Request at the head of the queue
    Request* next() const;

    // Deletes the Request at the head of the queue
    void advance();

    // Returns true if a process has no further requests
    bool finished() const;

    uint32_t id;
    uint32_t arrival_time;
    ProcessState state;
    std::queue<Request*> request_queue;

private:
    static uint32_t next_id;
};

#endif