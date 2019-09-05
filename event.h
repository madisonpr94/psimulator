/*
COSC 3360
Madison Pratt
Due 2018-02-23

This assignment simulates a process scheduling system
by responding to various "resource requests" provided
via standard input.
*/

#ifndef EVENT_H_GUARD
#define EVENT_H_GUARD

#include <stdint.h>

#include "process.h"

typedef enum EventType {
    EVENT_ARRIVAL,
    EVENT_CORE_FREE,
    EVENT_DISK_FREE,
    EVENT_INPUT_FREE
} EventType;

// Represents an event in the event-driven simulation
// Includes a comparator method which is used by the STL priority_queue
struct Event
{
    EventType type;
    Process* process; // Which process this event relates to
    uint32_t timestep; // When this event occurs

    bool operator()(Event* a, Event* b)
    {
        return a->timestep > b->timestep;
    }
};

#endif