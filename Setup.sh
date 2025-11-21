#!/bin/bash
set -e

# Ensure Homebrew exists
if ! command -v brew >/dev/null 2>&1; then
    echo "Homebrew not found. Install it from https://brew.sh/"
    exit 1
fi

# Install system libraries required by OSMnx
brew install geos proj spatialindex

# Ensure Python exists
if ! command -v python3 >/dev/null 2>&1; then
    echo "Python3 not found."
    exit 1
fi

# Install Python dependencies
python3 -m pip install --upgrade pip
python3 -m pip install osmnx pyyaml

# Build project
mkdir -p build
cmake -S . -B build
cmake --build build
cd build
make
