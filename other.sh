#!/bin/bash

sudo sysctl kernel.sched_rr_timeslice_ms=4

gcc -pthread Assignment_3_Code_Template.c -o aout

sudo trace-cmd record -p nop -F ./aout OTHER