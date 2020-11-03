set width 0
set height 0
set verbose off
set confirm off
set pagination off
set print address off

# This test does the following:
# 1. Run all code until gdb_sync1 invocation and pause all threads
# 2. Advance thread 10 (thread_id == 0) to internal_insert_ndode invocation
#    (threads 11 and 12 are waiting on spin loop)
# 3. Advance thread 11 (thread_id == 1) to internal_insert_node invocation
# 4. Advance thread 12 (thread_id == 2, reader thread) map->count invocation
#    New node should not be visible (neither thread 10 nor 11 completed)

break gdb_sync1
break loop_forever
run
rbreak concurrent_skip_list_impl.hpp:internal_insert_node
thread 10
c
jump loop_forever
thread 11
break gdb_sync2
c
del 4
finish
set variable loop_sync_1 = 0
c
jump loop_forever
thread 12
break gdb_sync3
c
del 5
finish
set variable loop_sync_2 = 0
break gdb_sync_exit thread 12
c
finish
info threads
quit
