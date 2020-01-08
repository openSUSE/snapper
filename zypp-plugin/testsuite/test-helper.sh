#!/bin/bash
# source this from the individual tests

MYDIR=$(dirname "$0")

runit() {
    local CONFIG="${1:-../../data/zypp-plugin.conf}"
    local STRACE=""
    # STRACE="strace -efile"

    SNAPPER_ZYPP_PLUGIN_CONFIG="$MYDIR/$CONFIG" \
      SNAPPER_ZYPP_PLUGIN_SNAPPER_CONFIG=testsuite \
      SNAPPER_ZYPP_PLUGIN_DBUS_SESSION=1 \
      $STRACE \
      "$MYDIR"/../snapper-zypp-plugin
}

# https://stomp.github.io/
stomp_message() {
    local COMMAND="$1"
    local HEADERS="$2"
    local BODY="$3"
    if [ -n "$HEADERS" ]; then
        HEADERS="$HEADERS"$'\n'
    fi
    printf '%s\n%s\n%s\0' "$COMMAND" "$HEADERS" "$BODY"
}

json() {
    local PACKAGE="${1?Package name expected}"
    local JSON=('{'
                '  "TransactionStepList": ['
                '    {'
                '      "type": "...",'
                '      "stage": "...",'
                '      "solvable": {'
                "        \"n\": \"${PACKAGE}\""
                '      }'
                '    }'
                '  ]'
                '}')
    echo "${JSON[@]}"
}

test_empty_messages() {
    stomp_message PLUGINBEGIN "" ""
    stomp_message COMMITBEGIN "" ""
    stomp_message COMMITEND "" ""
    stomp_message PLUGINEND "" ""
}

test_pre_post_snapshots() {
    stomp_message PLUGINBEGIN "userdata: a=b,c=d" ""
    stomp_message COMMITBEGIN "" "$(json mypkg)"
    stomp_message COMMITEND "" "$(json mypkg)"
    stomp_message PLUGINEND "" ""
}

test_pre_del_snapshots() {
    stomp_message PLUGINBEGIN "" ""
    stomp_message COMMITBEGIN "" "$(json mypkg)"
    stomp_message COMMITEND "" ""
    stomp_message PLUGINEND "" ""
}

# snapper needs a DBus connection even if it ends up not using it :-/
dbus_session_setup() {
    if [ -z "${DBUS_SESSION_BUS_ADDRESS-}" ]; then
        if ! type -P dbus-run-session >/dev/null; then
            echo "dbus-run-session cannot be run, skipping test"
            return 77
        else
            echo "Restarting test with dbus-run-session"
            exec dbus-run-session -- "$0"
        fi
    fi
}

mock_snapperd_setup() {
    MOCKDEP=(ruby -e "require 'dbus'")
    if ! "${MOCKDEP[@]}"; then
        echo "Mock snapperd cannot be run, skipping test"
        echo "('${MOCKDEP[*]}' failed)"
        return 77
    fi
    pkill -f mock-snapperd
    "$MYDIR"/mock-snapperd &
    sleep 1
    PID=$!
    trap "kill \$PID" EXIT TERM INT
}
