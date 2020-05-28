#!/bin/sh

echo -e "Command mode commands:\n$(grep -i 'command \"' src/noaftodo_cmd.cpp | sed 's/.*\/\/ command \"/  /g' | sed 's/\".*- /\t- /g')" > doc/cmd.doc.gen

echo -e "NOAFtodo - a TODO manager No-One-Asked-For.\n\n$(cat doc/cmd.doc.gen)" > doc/doc.gen
