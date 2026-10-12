# Fish completion for snapper
# Supports both 'snapper' and 'sudo snapper'

function __fish_snapper_configs
    if test -d /etc/snapper/configs
        command ls -1 /etc/snapper/configs 2>/dev/null
    end
end

function __fish_snapper_needs_command
    set -l cmd (commandline -opc)
    set -l subcommands list-configs create-config delete-config get-config set-config list ls create modify delete remove rm mount umount status diff xadiff undochange rollback setup-quota cleanup
    for c in $cmd
        if contains -- $c $subcommands
            return 1
        end
    end
    return 0
end

function __fish_snapper_using_command
    set -l cmd (commandline -opc)
    for c in $cmd
        for target in $argv
            if test "$c" = "$target"
                return 0
            end
        end
    end
    return 1
end

# Disable default file completions when needing command
complete -c snapper -n '__fish_snapper_needs_command' -f

# Global options
complete -c snapper -s c -l config -x -a '(__fish_snapper_configs)' -d 'Configuration name'
complete -c snapper -s q -l quiet -d 'Suppress normal output'
complete -c snapper -s v -l verbose -d 'Increase verbosity'
complete -c snapper -l utc -d 'Display dates and times in UTC'
complete -c snapper -l iso -d 'Display dates and times in ISO format'
complete -c snapper -s t -l table-style -x -a '1 2 3' -d 'Table style'
complete -c snapper -l no-dbus -d 'Operate without D-Bus'
complete -c snapper -s r -l root -r -d 'Operate on target root directory'
complete -c snapper -l version -d 'Print version'
complete -c snapper -s h -l help -d 'Print help'

# Subcommands
complete -c snapper -n '__fish_snapper_needs_command' -a 'list-configs' -d 'List configurations'
complete -c snapper -n '__fish_snapper_needs_command' -a 'create-config' -d 'Create configuration'
complete -c snapper -n '__fish_snapper_needs_command' -a 'delete-config' -d 'Delete configuration'
complete -c snapper -n '__fish_snapper_needs_command' -a 'get-config' -d 'Get configuration'
complete -c snapper -n '__fish_snapper_needs_command' -a 'set-config' -d 'Set configuration'
complete -c snapper -n '__fish_snapper_needs_command' -a 'list' -d 'List snapshots'
complete -c snapper -n '__fish_snapper_needs_command' -a 'ls' -d 'List snapshots'
complete -c snapper -n '__fish_snapper_needs_command' -a 'create' -d 'Create snapshot'
complete -c snapper -n '__fish_snapper_needs_command' -a 'modify' -d 'Modify snapshot metadata'
complete -c snapper -n '__fish_snapper_needs_command' -a 'delete' -d 'Delete snapshot'
complete -c snapper -n '__fish_snapper_needs_command' -a 'rm' -d 'Delete snapshot'
complete -c snapper -n '__fish_snapper_needs_command' -a 'remove' -d 'Delete snapshot'
complete -c snapper -n '__fish_snapper_needs_command' -a 'mount' -d 'Mount snapshot'
complete -c snapper -n '__fish_snapper_needs_command' -a 'umount' -d 'Unmount snapshot'
complete -c snapper -n '__fish_snapper_needs_command' -a 'status' -d 'Compare snapshots status'
complete -c snapper -n '__fish_snapper_needs_command' -a 'diff' -d 'Diff snapshots'
complete -c snapper -n '__fish_snapper_needs_command' -a 'xadiff' -d 'Extended attribute diff'
complete -c snapper -n '__fish_snapper_needs_command' -a 'undochange' -d 'Undo changes between snapshots'
complete -c snapper -n '__fish_snapper_needs_command' -a 'rollback' -d 'Rollback system root subvolume'
complete -c snapper -n '__fish_snapper_needs_command' -a 'setup-quota' -d 'Setup btrfs quota'
complete -c snapper -n '__fish_snapper_needs_command' -a 'cleanup' -d 'Run cleanup algorithm'

# Subcommand: create
complete -c snapper -n '__fish_snapper_using_command create' -s t -l type -x -a 'single pre post' -d 'Snapshot type'
complete -c snapper -n '__fish_snapper_using_command create' -l print-number -d 'Print created snapshot number'
complete -c snapper -n '__fish_snapper_using_command create' -s d -l description -x -d 'Description'
complete -c snapper -n '__fish_snapper_using_command create' -s c -l cleanup-algorithm -x -a 'number timeline empty-pre-post' -d 'Cleanup algorithm'
complete -c snapper -n '__fish_snapper_using_command create' -s u -l userdata -x -d 'User data (key=value)'
complete -c snapper -n '__fish_snapper_using_command create' -l command -r -d 'Command to run between pre and post'

# Subcommand: list / ls
complete -c snapper -n '__fish_snapper_using_command list ls' -s t -l type -x -a 'all single pre-post' -d 'Type of snapshots'
complete -c snapper -n '__fish_snapper_using_command list ls' -s a -l all-configs -d 'List snapshots of all configs'

# Subcommand: delete / rm / remove
complete -c snapper -n '__fish_snapper_using_command delete rm remove' -s s -l sync -d 'Sync after deletion'

# Subcommand: cleanup
complete -c snapper -n '__fish_snapper_using_command cleanup' -x -a 'number timeline empty-pre-post' -d 'Cleanup algorithm'
