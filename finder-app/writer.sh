#!/bin/sh

# Parameter checks
if [ $# -lt 2 ]; then
  echo "Error: Missing required arguments."
  echo "Usage: $0 <write_file> <write_string>"
  exit 1
fi

write_file=$1
write_string=$2

# Extract the directory path from the full file path
dest_dir=$(dirname "$write_file")

# Create the directory path if it doesn't exist
if ! mkdir -p "$dest_dir" 2>/dev/null; then
  echo "Error: Could not create directory path '$dest_dir'."
  exit 1
fi

# Write the string to the file, overwriting existing content
if ! echo "$write_string" >"$write_file" 2>/dev/null; then
  echo "Error: File '$write_file' could not be created or written to."
  exit 1
fi

exit 0
