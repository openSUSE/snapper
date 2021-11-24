# snapper(8) autocompletion

_snapper()
{
    local configdir="/etc/snapper/configs"
    local cur prev words cword
    _init_completion || return

    local GLOBAL_SNAPPER_OPTIONS='
        -q --quiet
        -v --verbose
        --utc
        --iso
        -t --table-style
        --abbreviate
        --machine-readable
        --csvout
        --jsonout
        --separator
        -c --config
        --no-dbus
        -r --root
        -a --ambit
        --version
        --help
    '

    # see if the user selected a command already
    local COMMANDS=(
        "list-configs" "create-config" "delete-config" "get-config" "set-config"
        "list" "ls"
        "create" "modify" "delete" "remove" "rm"
        "mount" "umount"
        "status" "diff" "xadiff"
        "undochange" "rollback"
        "setup-quota"
        "cleanup")

    local command i
    for (( i=0; i < ${#words[@]}-1; i++ )); do
        # Match word only either from start of string or after space to prevent options
        # like -c from matching commands that have -c in them, like list-configs
        if [[ ${COMMANDS[@]} =~ (^| )"${words[i]}" ]]; then
            command=${words[i]}
            break
        fi
    done

    # Global options autocomplete
    case $prev in
        --version|--help)
            return 0
            ;;
        --config|-c)
            local configs=()
            # Get basenames of config files in "$configdir"
            for configfile in "$configdir"/*; do
                configs+=("${configfile##*/}")
            done
            COMPREPLY=( $( compgen -W "${configs[*]}" -- "$cur" ) )
            return 0
            ;;
        --machine-readable)
            COMPREPLY=( $( compgen -W 'csv json' -- "$cur" ) )
            return 0
            ;;
        --root|-r)
            COMPREPLY=( $( compgen -f -- "$cur" ) )
            return 0
            ;;
    esac

    # supported options per command
    if [[ "$cur" == -* ]]; then
        case $command in
            list-configs)
                # --columns completion not implemented
                COMPREPLY=( $( compgen -W '--columns
                    ' -- "$cur" ))
                return 0
                ;;
            create-config)
                COMPREPLY=( $( compgen -W '--fstype -f
                  --template -t' -- "$cur" ) )
                return 0
                ;;
            list|ls)
                COMPREPLY=( $( compgen -W '--type -t
                  --disable-used-space
                  --all-configs -a
                  --columns' -- "$cur" ) )
                return 0
                ;;
            create)
                COMPREPLY=( $( compgen -W '--type -t
                  --pre-number
                  --print-number -p
                  --description -d
                  --cleanup-algorithm -c
                  --userdata -u
                  --command
                  --read-only
                  --read-write
                  --from' -- "$cur" ) )
                return 0
                ;;
            modify)
                COMPREPLY=( $( compgen -W '--description -d
                  --cleanup-algorithm -c
                  --userdata -u' -- "$cur" ) )
                return 0
                ;;
            delete|remove|rm)
                COMPREPLY=( $( compgen -W '--sync -s
                  ' -- "$cur" ) )
                return 0
                ;;
            status)
                COMPREPLY=( $( compgen -W '--output -o
                    ' -- "$cur" ) )
                return 0
                ;;
            diff)
                COMPREPLY=( $( compgen -W '--input -i
                    --diff-cmd
                    --extensions -x' -- "$cur" ) )
                return 0
                ;;
            undochange)
                COMPREPLY=( $( compgen -W '--input -i
                    ' -- "$cur" ) )
                return 0
                ;;
            rollback)
                COMPREPLY=( $( compgen -W '--print-number -p
                    --description -d
                    --cleanup-algorithm -c
                    --userdata -u' -- "$cur" ) )
                return 0
                ;;
            cleanup)
                COMPREPLY=( $( compgen -W '--path --free-space
                   ' -- "$cur" ) )
                return 0
                ;;
            *)
                COMPREPLY=( $( compgen -W "$GLOBAL_SNAPPER_OPTIONS" -- "$cur" ) )
                return 0
                ;;
        esac
    fi

    # specific command arguments
    if [[ -n $command ]]; then
        case $command in
            list-configs)
                case "$prev" in
                    --columns)
                        COMPREPLY=( $( compgen -W 'config subvolume
                        ' -- "$cur" ) )
                        ;;
                esac
                return 0
                ;;
            create-config)
                case "$prev" in
                    --fstype|-f)
                        COMPREPLY=( $( compgen -W 'btrfs ext4 lvm(xfs) lvm(ext4)
                        ' -- "$cur" ) )
                        ;;
                    --template|-t)
                        ;;
                    *)
                        COMPREPLY=( $( compgen -f -- "$cur" ) )
                        ;;
                esac
                return 0
                ;;
            list)
                case "$prev" in
                    --type|-t)  
                        COMPREPLY=( $( compgen -W 'all single pre-post
                        ' -- "$cur" ) )
                        ;;
                    --columns)
                        COMPREPLY=( $( compgen -W 'config subvolume number
                            default active type date user used-space cleanup
                            description userdata pre-number post-number
                            post-date
                        ' -- "$cur" ) )
                        ;;
                esac
                return 0
                ;;
            create)
                case "$prev" in
                    --type|-t)
                        COMPREPLY=( $( compgen -W 'single pre post
                        ' -- "$cur" ) )
                        ;;
                    --pre-number)
                        COMPREPLY=( $( compgen -W '
                        ' -- "$cur" ) )
                        ;;
                    --cleanup-algorithm|-c)
                        COMPREPLY=( $( compgen -W 'empty-pre-post timeline number
                        ' -- "$cur" ) )
                        ;;
                esac
                return 0
                ;;
            modify)
                case "$prev" in
                    --cleanup-algorithm|-c)
                        COMPREPLY=( $( compgen -W 'empty-pre-post timeline number
                        ' -- "$cur" ) )
                        ;;
                esac
                return 0
                ;;
            status)
                case "$prev" in
                    --output|-o)
                        COMPREPLY=( $( compgen -f -- "$cur" ) )
                        ;;
                esac
                return 0
                ;;
            cleanup)
                case "$prev" in
                    empty-pre-post|timeline|number)
                        ;;
                    --path)
                        COMPREPLY=( $( compgen -f -- "$cur" ) ) 
                        ;;
                    *)
                        COMPREPLY=( $( compgen -W 'empty-pre-post timeline number
                        ' -- "$cur" ) )
                        ;;
                esac
                return 0
                ;;
            diff)
                return 0
                ;;
            undochange)
                return 0
                ;;
            rollback)
                case "$prev" in
                    --cleanup-algorithm|-c)
                        COMPREPLY=( $( compgen -W 'empty-pre-post timeline number
                        ' -- "$cur" ) )
                        ;;
                esac
                return 0
                ;;
        esac
    fi

    # no command yet, show what commands we have
    if [ "$command" = "" ]; then
        #COMPREPLY=( $( compgen -W '${COMMANDS[@]} ${GLOBAL_SNAPPER_OPTIONS[@]}' -- "$cur" ) )
        COMPREPLY=( $( compgen -W "${COMMANDS[*]}" -- "$cur" ) )
    fi

    return 0
} &&
complete -F _snapper snapper
