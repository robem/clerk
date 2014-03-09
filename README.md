# Clerk

## Current status

* Not even close to v1.0
* Still some segfaults, experimenting with colors, and keystrokes

## Build

* clone _termbox_ and _yajl_

`git submodule update --init`

* build _termbox_ and _yajl_

`cd clerk/lib/termbox`

`./waf configure`

`./waf`

`rm build/src/libtermbox.so*` => to use static lib

`cd clerk/lib/yajl`

`./configure`

`make`

`cd clerk`

`scons`

## Usage

* `p` new project
* `P` delete current project
* `t` new todo
* `T` delete current todo
* `S` save to JSON file
* `Q` quit

* Movement: vi-style

* `SPACE` toggle todo finished/not-finished
* `r` mark todo as "in progress"

## Why the hell another TODO app?

Many times, I found myself in the situation to switch from one task to another?
Those tasks are more like mini-projects.
Therefore, I need to shift my brain to a completely different topic which is annoying because you have to remember all the important stuff you wanted to do.
To make it easier for me I tried different apps including some mindmapping tools.
I never felt really comfortable but to be honest, I didn't spent much time to learn all the cool quirks per app.

## What does clerk aim for?

*Satisfy my needs!*

* No mouse interaction at all
* No fancy UI
* Interactive commandline
* Get the job done with as few keystrokes as possible
* Visual feedback such as highlighting

## When this is all for you why make it public?

You got the same needs? => Get it!

You like the approach but you can do better? => Fork it!
