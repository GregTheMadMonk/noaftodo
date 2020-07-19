#!/bin/sh

# update manpage on wiki
printf "
> * [Home page](https://github.com/GregTheMadMonk/noaftodo/wiki)
> * [Guide](https://github.com/GregTheMadMonk/noaftodo/wiki/Guide) (**manpage**)
\`\`\`
%s
\`\`\`" \
"$(roff2text doc/noaftodo.man)" | sed 's/.//g' > ../noaftodo.wiki/NOAFtodo-manpage.md
