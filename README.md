# NOAFtodo
A TODO-manager No One Asked For. Written in C++, with ncurses, love and absolutely no clue why.

<img src="workflow.gif" width="576" height="324" alt="NOAFtodo workflow"></img>

### Current and future features (see [NOAFtodo roadmap](https://github.com/GregTheMadMonk/noaftodo/projects/1) to prepare for non-backwards-compatible changes in command interpreter)
### This README is kinda... not very informative. If you want to know more about building and using the program, there is a [manual](https://github.com/GregTheMadMonk/noaftodo/wiki/Manual) being created on the Wiki. After it is finished, the README will also be re-written.
*If you have any problems with the program, please open an issue here. It's much faster than waiting for me to notice a mistake, and I try to fix things as soon as possible.*
- [x] minimalisic interface written with ncurses
- [x] multiple lists between which tasks can be moved
- [x] primitive TODO list management: add and remove tasks with or without dues, mark tasks as completed, basic task editing
- [x] task filtering: by flag (failed, upcoming, completed, nodue, uncategoorized), by list (tag), regex serach titles and descriptions
- [x] a daemon that can work in background, track tasks dues and completion and is able to execute custom commands on certain events (like sending notifications)
- [x] per-task events on top of global events, which allow you to do some tricky things (creaing recurring tasks or setting up something like an alarm clock)
- [x] bugs
- [ ] option to configure program with source-code only (being considered, thinking of a way to do this and provide new functionality on top of just using a config file)
- [ ] \(not a feature really\) documentation

Default list is created as **$HOME/.noaftodo-list** and delault config is copied to **$XDG_CONFIG_HOME/noaftodo.conf** or **$HOME/.config/noaftodo.conf**, if **$XDG_CONFIG_HOME** is not defined.

Also running without creating a config is supported with `noaftodo -c default`, as default config is included in the program during compilation.

### Installing
#### AUR
Program is available in **AUR** as **noaftodo-git**.

### Building [[more](https://github.com/GregTheMadMonk/noaftodo/wiki/Manual#building)]
Run `make` (`gmake` on Solaris 11).

#### Dependencies
* **ncurses** (on an Arch-based system: `sudo pacman -S ncurses`, on a Debian-based system: `sudo apt install libncurses5-dev`)
* (optional) **notify-send** (provided by `libnotify` and `libnotify-bin` on Arch and Debian based distributions respectively) and a notification deamon of your choise

### Configuring
See **noaftodo.conf.template** if you want an example configuration.

### Adding tasks, and just using NOAFtodo
See [the manual](https://github.com/GregTheMadMonk/noaftodo/wiki/Manual).
