# NOAFtodo
A TODO-manager No One Asked For. Written in C++, with ncurses, notification support, love and absolutely no clue why.

![A screenshot](screenshot.png)

### Features
* minimalisic interface written with ncurses
* primitive TODO list management: add and remove tasks with dues, marking tasks as completed, filtering failed, completed, upcomig and uncategorized tasks.
* a daemon that works and background, tracks tasks dues and completion and sends notifications via libnotify

### Default shortcuts:
* ? - :? - shows help (sometimes outdated :) )
* q or \<esc\> - :q - exit program
* up arrow or k - :up - navigate up the list
* down arrow or j - :down - navigate down the list
* a - :a a (no autoexec) - adds entry (relative time)
* A - :a (no autoexec) - adds entry
* \<space\> - :c - toggles entry completion
* d - :d (no autoexec) - deletes entry
* gg - :g 0
* [input number]g - :g [input number] - goes to the entry with index [input number], or closest if hidden
* Gg - :g [last item index]
* ll - :list all
* [input number]l - :list [input number] - switches to view only the list with index [input number]
* U - :vtoggle uncat - toggles uncategorized entries visibility
* F - :vtoggle failed - toggles failed entries visibility
* C - :vtoggle coming - and so on
* V - :vtoggle complete - and so forth

Not really much more to say. _Yet?_
