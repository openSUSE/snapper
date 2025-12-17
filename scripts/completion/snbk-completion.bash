# snbk(8) autocompletion

_snbk()
{
    local configdir="/etc/snapper/backup-configs"
    local cur prev words cword
    _init_completion || return

    local GLOBAL_SNBK_OPTIONS='
        -q --quiet
        -v --verbose
        --utc
        --iso
        -t --table-style
        --machine-readable
        --csvout
        --jsonout
        --separator
	--no-headers
        -b --backup-config
	--no-dbus
	--target-mode
        --automatic
        --version
        --help
    '

    # see if the user selected a command already
    local COMMANDS=(
        "list-configs"
        "list" "ls"
        "transfer"
	"restore"
	"delete"
	"transfer-and-delete")

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
        --backup-config|-b)
            local configs=()
            # Get basenames of config files in "$configdir"
            for configfile in "$configdir"/*.json; do
		configfile="${configfile##*/}"
		configfile="${configfile%.*}"
                configs+=("${configfile}")
            done
            COMPREPLY=( $( compgen -W "${configs[*]}" -- "$cur" ) )
            return 0
            ;;
        --machine-readable)
            COMPREPLY=( $( compgen -W 'csv json' -- "$cur" ) )
            return 0
            ;;
    esac

    # supported options per command
    if [[ "$cur" == -* ]]; then
        case $command in
            *)
                COMPREPLY=( $( compgen -W "$GLOBAL_SNBK_OPTIONS" -- "$cur" ) )
                return 0
                ;;
        esac
    fi

    # no command yet, show what commands we have
    if [ "$command" = "" ]; then
        #COMPREPLY=( $( compgen -W '${COMMANDS[@]} ${GLOBAL_SNBK_OPTIONS[@]}' -- "$cur" ) )
        COMPREPLY=( $( compgen -W "${COMMANDS[*]}" -- "$cur" ) )
    fi

    return 0
} &&
complete -F _snbk snbk
