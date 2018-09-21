#!/bin/sh
# privileged mode is required for ptrace
docker run --privileged -w /cs3210/assigns/assign1 -v $(pwd)/:/cs3210 -it cs3210 /bin/bash 
