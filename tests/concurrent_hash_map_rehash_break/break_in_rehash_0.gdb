set width 0
set height 0
set verbose off
set confirm off
set breakpoint pending on

b set_rehashed
run

info break 1

quit
