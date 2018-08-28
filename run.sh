#!/bin/sh
# privileged mode is required for ptrace
docker run --privileged -w /labs/lab1 -v $(pwd)/labs:/labs -it cs3210 /bin/bash 
