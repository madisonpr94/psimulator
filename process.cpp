/*
COSC 3360
Madison Pratt
Due 2018-02-23

This assignment simulates a process scheduling system
by responding to various "resource requests" provided
via standard input.
*/

#include "process.h"

// Assigns a new ID and sets the default state
Process::Process()
{
    state = STATE_NOT_ARRIVED;
    id = next_id++;
}

// Ensures the request queue is properly cleaned up
Process::~Process()
{
    while(request_queue.size() > 0)
    {
        delete request_queue.front();
        request_queue.pop();
    }
}

// Constructs and attaches a request to the queue
void Process::addRequest(RequestType type, uint32_t duration)
{
    Request* r = new Request;
    r->type = type;
    r->duration = duration;
    request_queue.push(r);
}

// Returns a pointer to the Request at the head of the queue
Request* Process::next() const
{
    return request_queue.front();
}

// Deletes the Request at the head of the queue
void Process::advance()
{
    if(request_queue.size() > 0)
    {
        Request* r = request_queue.front();
        request_queue.pop();
        delete r;
    }
}

// Returns true if a process has no further requests
bool Process::finished() const
{
    return request_queue.size() == 0;
}

uint32_t Process::next_id = 0;