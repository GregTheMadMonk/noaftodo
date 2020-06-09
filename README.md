# NOAFtodo
A TODO-manager No One Asked For. Written in C++, with ncurses, love and absolutely no clue why.

<img src="workflow.gif" width="576" height="324" alt="NOAFtodo workflow"></img>

### Current and future features
- [x] minimalisic interface written with ncurses
- [x] multiple lists between which tasks can be moved
- [x] primitive TODO list management: add and remove tasks with or without dues, mark tasks as completed, basic task editing
- [x] task filtering: by flag (failed, upcoming, completed, nodue, uncategoorized), by list (tag), regex serach titles and descriptions
- [x] a daemon that can work in background, track tasks dues and completion and is able to execute custom commands on certain events (like sending notifications)
- [x] per-task events on top of global events, which allow you to do some tricky things (creaing recurring tasks or setting up something like an alarm clock)
- [x] bugs
- [ ] option to configure program with source-code only (being considered, thinking of a way to do this and provide new functionality on top of just using a config file)
- [ ] \(not a feature really\) documentation

Default list is created as **~/.noaftodo-list** and delault config is copied to **~/.config/noaftodo.conf**.

Also running without creating a config is supported with `noaftodo -c default`, as default config is included in the program during compilation.

### Building
Run `make` (`gmake` on Solaris 11).
#### Dependencies
**ncurses**. You need it (on an Arch-based system: `sudo pacman -S ncurses`, on a Debian-based system: `sudo apt install libncurses5-dev`).

Also, default config makes daemon use **notify-send** on task events (provided by `libnotify` and `libnotify-bin` on Arch and Debian based distributions respectively, requires notification deamon of your choise).

### Configuring
See **noaftodo.conf.template** if you want an example configuration.

### Adding a task, and just using NOAFtodo
Run `noaftodo -h` or press `?` in normal mode (use left and right arrows to scroll). The help message should cover most of the things you need.

The only thing you will probably find confusing is time and date fromat used: you specify time as `[a]YYYYyMMmDDdHHhμμ`, where:
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
* `15h00a1d` - task due is tomorrow at 3:00 pm: absolute and relative dues can be combined

Hope it makes sense.

You should also know that you can execute system commands with `!<command>`.

The best way to somewhat understand how NOAFtodo command-line works now will be reading **nooaftodo.conf.template**.

### Task categories
All tasks belong to the following categories and can be filtered by them in normal mode:
* Completed (marked completed manually)
* Failed (due has passed)
* (up)Coming (due is in less than 24 hours)
* Nodue (doesn't have a due)
* Uncategorized - all other

### Some of the default normal mode shortcuts:
* ? - `:?` - shows help
* q or \<esc> - `:q` - exit program
* up arrow or k - `:up` - navigate up the list
* down arrow or j - `:down` - navigate down the list
* a - `:a a ` (no autoexec) - adds entry (relative time)
* A - `:a` (no autoexec) - adds entry
* \<enter> - `:details` - show full information about entry
* \<space> - `:c` - toggles entry completion
* d - `:d` (no autoexec) - deletes entry
* gg - `:g 0`
* [input number]g - `:g [input number]` - goes to the entry with index [input number], or closest if hidden
* Gg - `:g [last item index]`
* ll - `:list all` - show tasks from all lists
* [input number]l - `:list [input number]` - switches to view only the list with index [input number]
* U|F|C|V|N - `:vtoggle uncat|failed|coming|completed|nodue` - toggles uncategorized|failed|upcoming|completed|nodue entries visibility
* / - `set regex_filter ` - filter tasks
