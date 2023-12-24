#!/bin/bash


mode=${1:-n}

case $mode in
    n) printf "Running simulation.\n"
       ./main run 1280 720 60;;
    r) printf "Rendering simulation..\n"
       read -p "Enter duration: " duration
       duration=${duration:-10}
       ./main render 1280 720 60 $duration;;
    b) printf "Running and rendering simulation.\n"
       ./main both 1280 720 60;;
esac    

