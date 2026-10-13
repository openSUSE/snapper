# Fish completion for snapper
# Supports both 'snapper' and 'sudo snapper'

function __fish_snapper_configs
    if test -d /etc/snapper/configs
        command ls -1 /etc/snapper/configs 2>/dev/null
    end
end

# Disable default file completions when subcommand is expected
complete -c snapper -n '__fish_use_subcommand' -f

# Global options (only proposed before a subcommand is given)
complete -c snapper -n '__fish_use_subcommand' -s c -l config -x -a '(__fish_snapper_configs)' -d 'Configuration name'
complete -c snapper -n '__fish_use_subcommand' -s q -l quiet -d 'Suppress normal output'
complete -c snapper -n '__fish_use_subcommand' -s v -l verbose -d 'Increase verbosity'
complete -c snapper -n '__fish_use_subcommand' -l utc -d 'Display dates and times in UTC'
complete -c snapper -n '__fish_use_subcommand' -l iso -d 'Display dates and times in ISO format'
complete -c snapper -n '__fish_use_subcommand' -s t -l table-style -x -a '1 2 3' -d 'Table style'
complete -c snapper -n '__fish_use_subcommand' -l no-dbus -d 'Operate without D-Bus'
complete -c snapper -n '__fish_use_subcommand' -s r -l root -r -d 'Operate on target root directory'
complete -c snapper -n '__fish_use_subcommand' -l version -d 'Print version'
complete -c snapper -n '__fish_use_subcommand' -s h -l help -d 'Print help'

# Subcommands
complete -c snapper -n '__fish_use_subcommand' -a 'list-configs' -d 'List configurations'
complete -c snapper -n '__fish_use_subcommand' -a 'create-config' -d 'Create configuration'
complete -c snapper -n '__fish_use_subcommand' -a 'delete-config' -d 'Delete configuration'
complete -c snapper -n '__fish_use_subcommand' -a 'get-config' -d 'Get configuration'
complete -c snapper -n '__fish_use_subcommand' -a 'set-config' -d 'Set configuration'
complete -c snapper -n '__fish_use_subcommand' -a 'list' -d 'List snapshots'
complete -c snapper -n '__fish_use_subcommand' -a 'ls' -d 'List snapshots'
complete -c snapper -n '__fish_use_subcommand' -a 'create' -d 'Create snapshot'
complete -c snapper -n '__fish_use_subcommand' -a 'modify' -d 'Modify snapshot metadata'
complete -c snapper -n '__fish_use_subcommand' -a 'delete' -d 'Delete snapshot'
complete -c snapper -n '__fish_use_subcommand' -a 'rm' -d 'Delete snapshot'
complete -c snapper -n '__fish_use_subcommand' -a 'remove' -d 'Delete snapshot'
complete -c snapper -n '__fish_use_subcommand' -a 'mount' -d 'Mount snapshot'
complete -c snapper -n '__fish_use_subcommand' -a 'umount' -d 'Unmount snapshot'
complete -c snapper -n '__fish_use_subcommand' -a 'status' -d 'Compare snapshots status'
complete -c snapper -n '__fish_use_subcommand' -a 'diff' -d 'Diff snapshots'
complete -c snapper -n '__fish_use_subcommand' -a 'xadiff' -d 'Extended attribute diff'
complete -c snapper -n '__fish_use_subcommand' -a 'undochange' -d 'Undo changes between snapshots'
complete -c snapper -n '__fish_use_subcommand' -a 'rollback' -d 'Rollback system root subvolume'
complete -c snapper -n '__fish_use_subcommand' -a 'setup-quota' -d 'Setup btrfs quota'
complete -c snapper -n '__fish_use_subcommand' -a 'cleanup' -d 'Run cleanup algorithm'

# Subcommand: create
complete -c snapper -n '__fish_seen_subcommand_from create' -s t -l type -x -a 'single pre post' -d 'Snapshot type'
complete -c snapper -n '__fish_seen_subcommand_from create' -l print-number -d 'Print created snapshot number'
complete -c snapper -n '__fish_seen_subcommand_from create' -s d -l description -x -d 'Description'
complete -c snapper -n '__fish_seen_subcommand_from create' -s c -l cleanup-algorithm -x -a 'number timeline empty-pre-post' -d 'Cleanup algorithm'
complete -c snapper -n '__fish_seen_subcommand_from create' -s u -l userdata -x -d 'User data (key=value)'
complete -c snapper -n '__fish_seen_subcommand_from create' -l command -r -d 'Command to run between pre and post'

# Subcommand: list / ls
complete -c snapper -n '__fish_seen_subcommand_from list ls' -s t -l type -x -a 'all single pre-post' -d 'Type of snapshots'
complete -c snapper -n '__fish_seen_subcommand_from list ls' -s a -l all-configs -d 'List snapshots of all configs'

# Subcommand: delete / rm / remove
complete -c snapper -n '__fish_seen_subcommand_from delete rm remove' -s s -l sync -d 'Sync after deletion'

# Subcommand: cleanup
complete -c snapper -n '__fish_seen_subcommand_from cleanup' -x -a 'number timeline empty-pre-post' -d 'Cleanup algorithm'
