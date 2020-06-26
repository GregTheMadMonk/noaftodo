#!/bin/sh

echo -e "Command mode commands:\n$(grep -i 'command \"' src/noaftodo_cmd.cpp | sed 's/.*\/\/ command \"/  :/g' | sed 's/\".*- /\t\t- /g')" > doc/cmd.doc.gen
echo -e "Command-line arguments:\n$(grep -i 'argument \"' src/noaftodo.cpp | sed 's/.*\/\/ argument \"/  /g' | sed 's/\".*- /\t\t- /g')" > doc/arg.doc.gen

echo -e "NOAFtodo - a TODO manager No-One-Asked-For.\n\n$(cat doc/arg.doc.gen)\n\n$(cat doc/cmd.doc.gen)\n\n$(cat doc/troubleshooting.doc)" > doc/doc.gen
