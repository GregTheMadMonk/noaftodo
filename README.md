# NOAFtodo
A TODO-manager No One Asked For. Written in C++, with ncurses, love and absolutely no clue why.

![NOAFtodo workflow](workflow.gif)

### Features
* minimalisic interface written with ncurses
* multiple lists between which tasks can be moved
* primitive TODO list management: add and remove tasks with dues, marking tasks as completed, filtering failed, completed, upcomig and uncategorized tasks
* a daemon that can work in background, track tasks dues and completion and is able to execute custom commands on certain events (like sending notifications)

Default list is created as **~/.noaftodo-list** and delault config is copied to **~/.config/noaftodo.conf**.

### Building
Run `make`.

On Solaris 11, run `gmake`.

### How to add a task?
You can easily add a task with
`:a <due> <title> <description>`.
Some might find due format confusing though, so here it is explained:

You specify due as `[a]YYYYyMMmDDdHHhμμ` it means:
* `YYYYy` - year YYYY
* `MMm` - MM'th month
* `DDd` - DD'th day
* `HHh` - hour of day (24-hour format)
* `μμ` - μμ minutes
* `a` at the beginning is optional and indicates that instead of reading the input as due, it should be added to current time and then counted as task due.
You can set only one property or several of them. Unset properties will be reset to their current values, or zero if `a` is specified.

Examples:
* `a15h` - task due is in 15 hours from now
* `15h00` - task due is today at 3:00 pm. Note that is `00` is not specified in the end the minutes for due will be set to teir current value and might not be 00

Hope it makes sense.

### Task categories
All tasks belong in one of the following categories and can be filtered by them in normal mode:
* Completed (marked completed manually)
* Failed (due has passed)
* (up)Coming (due is in less than 24 hours)
* Uncategorized - all other

### Binding keys
Command responsible for binding keys is `:bind <key> <command> <mode> <autoexec>`.
Exampes of it being used can be found in template config.

`mode` specifies modes that allow this bind. See **src/noaftodo_cui.h** for integer values for modes. For bind to be executable in multiple modes, `mode` should be set to logical OR of their values: 9 = 0b1001 = 0b1000 | 0b0001 = CUI_MODE_DETAILS | CUI_MODE_NORMAl - bind will be available in normal and description modes.

If `autoexec` is set, command executes immediately, otherwise the bind will take you to command mode with `command` pre-typed

### Some of default shortcuts (can be modified in config):
* ? - :? - shows help
* q or \<esc\> - :q - exit program
* up arrow or k - :up - navigate up the list
* down arrow or j - :down - navigate down the list
* a - :a a (no autoexec) - adds entry (relative time)
* A - :a (no autoexec) - adds entry
* \<enter\> - :details - show full information about entry
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
