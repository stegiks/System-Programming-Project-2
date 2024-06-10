#!/bin/bash

# ! This file should be ran with the source command 
# ! so the changes are kept in the shell that runs the script
# ! example of usage: $ source append_to_path_var.sh

# Get the absolute path to the bin directory
BIN_DIR="${PWD}/bin"

if [ -d "$BIN_DIR" ]; then
    # Add the bin directory to the PATH
    export PATH="$PATH:$BIN_DIR"
    echo "Bin directory added to PATH: $BIN_DIR"
else
    echo "Bin directory not found: $BIN_DIR"
fi