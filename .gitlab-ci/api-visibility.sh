#!/bin/bash

# Check that private headers aren't included in public ones.
if grep "include.*private.h" $(ls src/*.h | grep -v "private.h");
then
  echo "Private headers shouldn't be included in public ones."
  exit 1
fi

# Check that adwaita.h contains all the public headers.
for header in $(ls src | grep "\.h$" | grep -v "private.h" | grep -v adwaita.h);
do
  if ! grep -q "$(basename $header)" src/adwaita.h;
  then
    echo "The public header" $(basename $header) "should be included in adwaita.h."
    exit 1
  fi
done
