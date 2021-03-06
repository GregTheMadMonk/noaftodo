#!/bin/sh

if [ "$1" = "" ]; then
	echo "Directory unspecified"
	exit 1
fi
DIR="$1"

echo "Generating docs and embedded code for $DIR"

mkdir -p gen/doc
mkdir -p gen/src

printf "Command mode commands:\n%s" "$(grep -i '@command \"' $(find $DIR/src/*) | sort | sed 's/.*@command \"/\t:/g;s/\".*- /\n\t\t/g')" > gen/doc/cmd.doc.gen
printf "Console variables:\n%s" "$(grep -i '@cvar \"' $(find $DIR/src/*) | sort | sed 's/.*@cvar \"/\t%/g;s/\".*- /%\n\t\t/g')" > gen/doc/cvar.doc.gen
printf "Command-line arguments:\n%s" "$(grep -i '@argument \"' $DIR/src/noaftodo.cpp | sort | sed 's/.*argument \"/\t/g;s/\".*- /\n\t\t/g')" > gen/doc/arg.doc.gen
printf "%s" "$(grep -i '@LISTVIEW column \"' $DIR/src/def.cpp | sort | sed 's/.*@LISTVIEW column \"/\t/g;s/\".*- /\n\t\t/g')" > gen/doc/cols.lview.doc.gen
printf "%s" "$(grep -i '@NORMAL column \"' $DIR/src/def.cpp | sort | sed 's/.*@NORMAL column \"/\t/g;s/\".*- /\n\t\t/g')" > gen/doc/cols.norm.doc.gen
printf "%s" "$(grep -i '@status field \"' $DIR/src/def.cpp | sort | sed 's/.*@status field \"/\t/g;s/\".*- /\n\t\t/g')" > gen/doc/fields.status.doc.gen

printf "#include <string>
std::string HELP = \"%s\";
std::string CMDS_HELP = \"%s\";
std::string CVARS_HELP = \"%s\";
std::string TCONF = \"%s\";" \
"$(sed 's/\\/\\\\/g' gen/doc/arg.doc.gen | sed 's/\t/\\t/g' | sed ':a;N;$!ba;s/\n/\\n/g' | sed 's/\"/\\"/g')" \
"$(sed 's/\*/\\\*/g' gen/doc/cmd.doc.gen | sed 's/\\/\\\\/g' | sed 's/\t/\\t/g' | sed '0~2s/^/**/g;0~2s/$/**/g' | sed ':a;N;$!ba;s/\n/\\n/g' | sed 's/\"/\\"/g')" \
"$(sed 's/\*/\\\*/g' gen/doc/cvar.doc.gen | sed 's/\\/\\\\/g' | sed 's/\t/\\t/g' | sed '0~2s/^/**/g;0~2s/$/**/g' | sed ':a;N;$!ba;s/\n/\\n/g' | sed 's/\"/\\"/g')" \
"$(sed 's/\\/\\\\/g' $DIR/noaftodo.conf.template | sed ':a;N;$!ba;s/\n/\\n/g' | sed 's/\"/\\"/g')" \
> gen/src/embed.cpp

VERSION=$(grep "noaftodo VERSION" $DIR/CMakeLists.txt | sed 's/^.*VERSION //g;s/)$//g')

printf ".TH NOAFTODO 1 \"$(date +%B\ %Y)\" \"%s\" \"NOAFtodo man page\"
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
.SS CONSOLE VARIABLES
%s
.SH NORMAL MODE COLUMNS
NORMAL mode columns are specified in \"norm.cols.all\" and \"norm.cols\" cvars (for when all lists/only one list is viewed correspondingly). You can also set some NORMAL mode columns to display their values with \"det.cols\".
%s
.SH LISTVIEW MODE COLUMNS
LISTVIEW mode columns are set in \"det.cols\" cvar.
%s
.SH STATUS FIELDS
Status fields are specified in \"norm.status_fields\" and \"livi.status_fields\" cvars for NORMAL and LISTVIEW correspondingly.
%s
.SH TROUBLESHOOTING
%s
.SH EPILOGUE
.I
Apology for bad english and probably not very useful manpage.
.I
<3 Y'all! Peace!" \
"$VERSION" \
"$(sed '1d;s/\t//g;s/-/\\-/g;0~2s/^/.HP\n/g;0~2s/$/\n.IP/g' gen/doc/arg.doc.gen)" \
"$(sed '1d;s/\t//g;s/-/\\-/g;0~2s/^/.HP\n.B\n/g;0~2s/$/\n.IP/g' gen/doc/cmd.doc.gen)" \
"$(sed '1d;s/\t//g;s/-/\\-/g;0~2s/^/.HP\n.B\n/g;0~2s/$/\n.IP/g' gen/doc/cvar.doc.gen)" \
"$(sed 's/\t//g;s/-/\\-/g;1~2s/^/.HP\n.B\n/g;1~2s/$/\n.IP/g' gen/doc/cols.norm.doc.gen)" \
"$(sed 's/\t//g;s/-/\\-/g;1~2s/^/.HP\n.B\n/g;1~2s/$/\n.IP/g' gen/doc/cols.lview.doc.gen)" \
"$(sed 's/\t//g;s/-/\\-/g;1~2s/^/.HP\n.B\n/g;1~2s/$/\n.IP/g' gen/doc/fields.status.doc.gen)" \
"$(sed '1d;s/\t//g;2s/.*/\.SS CONFIG VERSION MISMATCH/g;4s/.*/\.SS LIST VERSION MISMATCH/g' $DIR/doc/troubleshooting.doc)" > gen/doc/noaftodo.man

gzip -c gen/doc/noaftodo.man > gen/doc/noaftodo.man.gz
