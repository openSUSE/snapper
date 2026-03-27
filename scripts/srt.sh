#!/bin/bash

# SRT - (Snapper) Snapshot Restore Tool v1.0
# by Dan MacDonald

# SRT makes it easy to restore user home directories from BTRFS snapshots created by Snapper using rsync and a simple dialog TUI interface.
# SRT does not require root permission to run but it does require rsync, dialog and read access to your users ~/.snapshots dir.

# Close your web browser and any open documents before running this, if you are running it locally.

# Configuration
CONFIG_NAME=$(whoami)
SNAPSHOT_DIR="$HOME/.snapshots"

# Dependency Checks
for cmd in dialog rsync snapper; do
    if ! command -v "$cmd" &> /dev/null; then
        echo "Error: Missing $cmd"
        exit 1
    fi
done

# Loading message
# We use --infobox because it doesn't wait for user input
dialog --title "Snapper Restore Tool" --infobox "\nProcessing snapshot data for user '$CONFIG_NAME'...\nPlease wait." 7 60

# Build the snapshots menu
MENU_OPTIONS=()
while read -r line; do
    ID=$(echo "$line" | awk -F'|' '{print $1}' | xargs)
    DATE=$(echo "$line" | awk -F'|' '{print $4}' | xargs)
    DESC=$(echo "$line" | awk -F'|' '{print $8}' | xargs)

    if [[ "$ID" =~ ^[0-9]+$ ]] && [ "$ID" -ne 0 ]; then
        MENU_OPTIONS+=("$ID" "[$DATE] $DESC")
    fi
done < <(snapper -c "$CONFIG_NAME" list --disable-used-space 2>/dev/null | grep -v "current" | grep "|")

if [ ${#MENU_OPTIONS[@]} -eq 0 ]; then
    dialog --title "Error" --msgbox "No snapshots found for config: $CONFIG_NAME" 10 60
    exit 1
fi

# Reverse list so newest is at the top
REVERSED_OPTIONS=()
for (( i=${#MENU_OPTIONS[@]}-2; i>=0; i-=2 )); do
    REVERSED_OPTIONS+=("${MENU_OPTIONS[i]}" "${MENU_OPTIONS[i+1]}")
done

# Snapshot selection
SNAP_NUM=$(dialog --title "Snapper Restore Tool: $CONFIG_NAME" \
    --cancel-label "Exit" \
    --menu "Select a snapshot to restore:" 20 90 12 \
    "${REVERSED_OPTIONS[@]}" \
    3>&1 1>&2 2>&3)

[ $? -ne 0 ] && clear && exit

# Action choice
ACTION=$(dialog --title "Snapshot #$SNAP_NUM options" \
    --cancel-label "Back" \
    --menu "Select an action:" 12 60 3 \
    "1" "DRY RUN (Preview changes and Exit)" \
    "2" "RESTORE (Destructive Revert)" \
    3>&1 1>&2 2>&3)

[ $? -ne 0 ] && clear && exit

TARGET_PATH="$SNAPSHOT_DIR/$SNAP_NUM/snapshot"

# --- ACTION 1: DRY RUN ---
if [ "$ACTION" == "1" ]; then
    clear
    echo "--- DRY RUN PREVIEW: Snapshot #$SNAP_NUM ---"
    echo "Snapshot Source: $TARGET_PATH"
    echo "Target Directory: $HOME"
    echo "------------------------------------------------------------"

    # Run rsync dry run
    rsync -aAXvi --dry-run --delete --exclude='.snapshots' "$TARGET_PATH/" "$HOME/"

    echo "------------------------------------------------------------"
    echo "DRY RUN COMPLETE. No files were changed."
    echo "Script exiting."
    exit 0
fi

# --- ACTION 2: RESTORE ---
if [ "$ACTION" == "2" ]; then
    dialog --title "!!! FINAL WARNING !!!" --colors \
        --yesno "User: \Zb$CONFIG_NAME\Zn\nSnapshot: \Zb#$SNAP_NUM\Zn\n\nEverything in your home directory (except the ~/.snapshots directory) will be \ZbDELETED\Zn.\n\nContinue?" 15 65

    if [ $? -eq 0 ]; then
        clear
        echo "EXECUTING RESTORE: Snapshot #$SNAP_NUM"
        echo "----------------------------------------"

        rsync -aAXvh --delete --exclude='.snapshots' --info=progress2 "$TARGET_PATH/" "$HOME/"

        if [ ${PIPESTATUS[0]} -eq 0 ]; then
            dialog --title "Success" --msgbox "Home directory successfully rolled back to snapshot #$SNAP_NUM." 7 60
        else
            echo -e "\nError: Rsync failed. Check for open files."
        fi
    fi
fi

clear
