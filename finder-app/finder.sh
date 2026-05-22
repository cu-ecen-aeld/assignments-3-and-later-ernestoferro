#!/bin/sh

# Parameter checks
if [ $# -lt 2 ]; then
  echo "Error: Missing required arguments."
  echo "Usage: $0 <files_dir> <search_string>"
  exit 1
fi

files_dir=$1
search_string=$2

# Check if files_dir is actually a directory
if [ ! -d "$files_dir" ]; then
  echo "Error: Directory '$files_dir' does not exist or is not a directory."
  exit 1
fi

# Calculate total number of files
count_of_files=$(find "$files_dir" -type f | wc -l)

# Calculate total number of matching lines
count_of_matching_lines=$(grep -r "$search_string" "$files_dir" 2>/dev/null | wc -l)

# Print the final result
echo "The number of files are $count_of_files and the number of matching lines are $count_of_matching_lines"

exit 0
