set width 0
set height 0
set verbose off
set confirm off
set breakpoint pending on

b pmemobj_tx_end
run

info break 1

quit
