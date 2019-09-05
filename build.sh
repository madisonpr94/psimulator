#!/bin/bash
echo 'Building project...'
`g++ main.cpp process.cpp -o psimulator 2> err.txt`
if [[ $? -eq 0 ]]; then
  echo 'Build complete'
  rm err.txt
else
  echo 'Build failed - error details saved in err.txt'
fi
