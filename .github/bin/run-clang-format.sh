#!/bin/bash

FORMAT_OUT=${TMPDIR:-/tmp}/clang-format-diff.out

# Run on all files.
find src/ -name "*.h" -o -name "*.cc" | xargs -P2 clang-format-11 -i

# Check if we got any diff
git diff > ${FORMAT_OUT}

if [ -s ${FORMAT_OUT} ]; then
   echo "Style not matching; please run .github/bin/run-clang-format.sh"
   cat ${FORMAT_OUT}
   exit 1
fi

exit 0
