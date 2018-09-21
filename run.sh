#!/bin/sh
# privileged mode is required for ptrace
docker run --privileged -w /assigns/assign1 -v $(pwd)/assigns:/assigns -it cs3210 /bin/bash 
