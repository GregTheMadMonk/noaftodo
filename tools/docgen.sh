#!/bin/sh

printf "Command mode commands:\n$(grep -i 'command \"' src/noaftodo_cmd.cpp | sort | sed 's/.*\/\/ command \"/\t:/g;s/\".*- /\n\t\t/g')" > doc/cmd.doc.gen
printf "Command-line arguments:\n$(grep -i 'argument \"' src/noaftodo.cpp | sort | sed 's/.*\/\/ argument \"/\t/g;s/\".*- /\n\t\t/g')" > doc/arg.doc.gen

echo "#include <string>
std::string HELP = \"$(sed 's/\\/\\\\/g' doc/arg.doc.gen | sed 's/\t/\\t/g' | sed ':a;N;$!ba;s/\n/\\n/g' | sed 's/\"/\\"/g')\";
std::string CMDS_HELP = \"$(sed 's/\*/\\\*/g' doc/cmd.doc.gen | sed 's/\\/\\\\/g' | sed 's/\t/\\t/g' | sed '0~2s/^/**/g;0~2s/$/**/g' | sed ':a;N;$!ba;s/\n/\\n/g' | sed 's/\"/\\"/g')\";
std::string TCONF = \"$(sed 's/\\/\\\\/g' noaftodo.conf.template | sed ':a;N;$!ba;s/\n/\\n/g' | sed 's/\"/\\"/g')\";" > src/noaftodo_embed.cpp

LIST_V=$(grep ' LIST_V = ' src/noaftodo.h | sed 's/.*LIST_V = //g;s/\;//g')
CONF_V=$(grep ' CONF_V = ' src/noaftodo.h | sed 's/.*CONF_V = //g;s/\;//g')
MINOR_V=$(grep ' MINOR_V = ' src/noaftodo.h | sed 's/.*MINOR_V = //g;s/\;//g')

printf ".TH NOAFTODO 1 \"July 2020\" \"$LIST_V.$CONF_V.$MINOR_V\" \"NOAFtodo man page\"
.SH NAME
NOAFtodo
A TODO manager No-One-Asked-For.
.SH SYNOPSIS
noaftodo [OPTIONS]

noaftodo -d [OPTIONS]

naoftodo -r

noaftodo -h
.SH DESCRIPTION
NOAFtodo is a simple ncurses TODO-manager. Also includes a deamon for tracking task dues and completion.
.SH OPTIONS
$(sed '1d;s/\t//g;s/-/\\-/g;0~2s/^/\.HP\n/g;0~2s/$/\n\.IP/g' doc/arg.doc.gen)
.SH COMMAND MODE
.SS COMMANDS
$(sed '1d;s/\t//g;s/-/\\-/g;0~2s/^/\.HP\n.B\n/g;0~2s/$/\n\.IP/g' doc/cmd.doc.gen)
.SH TROUBLESHOOTING
$(sed '1d;s/\t//g;2s/.*/\.SS CONFIG VERSION MISMATCH/g;4s/.*/\.SS LIST VERSION MISMATCH/g' doc/troubleshooting.doc)
" > doc/noaftodo.man
