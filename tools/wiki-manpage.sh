#!/bin/sh

# update manpage on wiki
printf "\`\`\`
%s
\`\`\`" \
"$(roff2text doc/noaftodo.man)" | sed 's/.//g' > ../noaftodo.wiki/NOAFtodo-manpage.md
