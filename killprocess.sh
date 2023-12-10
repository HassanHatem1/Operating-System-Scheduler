#!/bin/bash

# Find all processes with names containing ".out" using sudo
pids=$(sudo pgrep ".out")

# Check if any PIDs were found
if [ -n "$pids" ]; then
    echo "Killing processes with names containing '.out'"
    
    # Loop through each PID and terminate the process
    while IFS= read -r pid; do
        sudo kill -9 "$pid";
        echo "Process with PID $pid killed."
    done <<< "$pids"
    
    echo "All processes killed successfully."
else
    echo "No processes found with names containing '.out'"
fi