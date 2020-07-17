#!/bin/sh

printf "Command mode commands:\n%s" "$(grep -i 'command \"' src/noaftodo_cmd.cpp | sort | sed 's/.*\/\/ command \"/\t:/g;s/\".*- /\n\t\t/g')" > doc/cmd.doc.gen
printf "Command-line arguments:\n%s" "$(grep -i 'argument \"' src/noaftodo.cpp | sort | sed 's/.*\/\/ argument \"/\t/g;s/\".*- /\n\t\t/g')" > doc/arg.doc.gen

printf "#include <string>
std::string HELP = \"%s\";
std::string CMDS_HELP = \"%s\";
std::string TCONF = \"%s\";" \
"$(sed 's/\\/\\\\/g' doc/arg.doc.gen | sed 's/\t/\\t/g' | sed ':a;N;$!ba;s/\n/\\n/g' | sed 's/\"/\\"/g')" \
"$(sed 's/\*/\\\*/g' doc/cmd.doc.gen | sed 's/\\/\\\\/g' | sed 's/\t/\\t/g' | sed '0~2s/^/**/g;0~2s/$/**/g' | sed ':a;N;$!ba;s/\n/\\n/g' | sed 's/\"/\\"/g')" \
"$(sed 's/\\/\\\\/g' noaftodo.conf.template | sed ':a;N;$!ba;s/\n/\\n/g' | sed 's/\"/\\"/g')" \
> src/noaftodo_embed.cpp

LIST_V=$(grep ' LIST_V = ' src/noaftodo.h | sed 's/.*LIST_V = //g;s/\;//g')
CONF_V=$(grep ' CONF_V = ' src/noaftodo.h | sed 's/.*CONF_V = //g;s/\;//g')
MINOR_V=$(grep ' MINOR_V = ' src/noaftodo.h | sed 's/.*MINOR_V = //g;s/\;//g')

printf ".TH NOAFTODO 1 \"July 2020\" \"%s.%s.%s\" \"NOAFtodo man page\"
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
%s
.SH COMMAND MODE
.SS COMMANDS
%s
.SH TROUBLESHOOTING
%s" \
"$LIST_V" \
"$CONF_V" \
"$MINOR_V" \
"$(sed '1d;s/\t//g;s/-/\\-/g;0~2s/^/\.HP\n/g;0~2s/$/\n\.IP/g' doc/arg.doc.gen)" \
"$(sed '1d;s/\t//g;s/-/\\-/g;0~2s/^/\.HP\n.B\n/g;0~2s/$/\n\.IP/g' doc/cmd.doc.gen)" \
"$(sed '1d;s/\t//g;2s/.*/\.SS CONFIG VERSION MISMATCH/g;4s/.*/\.SS LIST VERSION MISMATCH/g' doc/troubleshooting.doc)" > doc/noaftodo.man
