#!/bin/bash

c_files=$(find . -name "*.c" -type f)

for file in $c_files
do
    cp "$file" "${file}.orig"
done

