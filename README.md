# lawnch

lawnch is a hard-coded, keyboard Wayland launcher. Just a fast daemon-style launcher with a search input, keyboard navigation, and css styling using GTK4 and GTK layer shell.

## The thing

This is not meant to compete with highly extensible launchers.  
It exists to stay small, understandable, and hackable **at the source level only**.

## Goal

- Search input
- Hard-coded launch targets
- Keyboard navigation
- Runs as a lightweight daemon
- hard-coded extension just for the input.

> **Important**
>
> This project is in an early stage and is highly experimental.
> Interfaces, behavior, and internal design may change rapidly without notice.

## Building

The project is built using Nix.

```sh
# Enter development shell
nix-shell

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

./lawnch -d # start the daemon
./lawnch -t # toggle the search window
./lawnch -q # kill the daemon
./lawnch -r # reload the daemon
```

## Styling

You can find example style in **assets/style.css**. Copy the style to ~/.config/lawnch/style.css.
