#!/bin/bash

# Set the current year as a variable
current_year=$(date +"%Y")

# Find all files in the sys/ directory and its subdirectories
find sys/ -type f -print0 | while read -d $'\0' file
do
  # Replace the old copyright notice with the new one
  sed -i "s/\(Copyright (c) \)[0-9]\{4\}\(.*\)/\1${current_year}\2/g" "${file}"
done
