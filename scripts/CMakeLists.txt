add_subdirectory( testnet_np )

configure_file(start_npnode.sh start_npnode.sh COPYONLY)
configure_file(tn_bounce.sh tn_bounce.sh COPYONLY)
configure_file(tn_down.sh tn_down.sh COPYONLY)
configure_file(tn_gen_config.sh tn_gen_config.sh COPYONLY)
configure_file(tn_roll.sh tn_roll.sh COPYONLY)
configure_file(tn_start_all.sh tn_start_all.sh COPYONLY)
configure_file(tn_up.sh tn_up.sh COPYONLY)
