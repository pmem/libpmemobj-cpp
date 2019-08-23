set width 0
set height 0
set verbose off
set confirm off
set breakpoint pending on

# break after 2nd persist
b *pmem::obj::experimental::internal::hash_map_base::insert_new_node+104
run
quit
