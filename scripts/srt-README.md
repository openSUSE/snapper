# SRT

SRT is the [Snapper](http://snapper.io/) Snapshot Restoration Tool.

## WARNING

Running SRT can be dangerous! srt provides a text interface to easily choose a snapper home directory snapshot to revert to.

**All files created since the date of the chosen snapshot will be deleted!**

Use the Dry Run mode to preview what files would be deleted if you restore to the chosen snapshots state. You should always use Dry Run mode first. I cannot be held resposible for any loss of data or damage caused by running this program. You run SRT entirely at your own risk.

Quit any browsers and stop any other running programs before running this script locally.

## Why does SRT exist?

I was a happy ZFS user for several years before I tried BTRFS and one of my favourite ZFS features is filesystem snapshots. ZFS has snapshots of datasets whereas BTRFS has snapshots of subvolumes but there are differences in how subvols and datasets are handled and how snapper reverts snapshots compared to ZFS.

When I started using BTRFS I wanted to be able to revert snapper snapshots, at least of user home directories, in a very similar way to how the `zfs rollback` command works but unfortunately the `snapper rollback` command does not work in a similar manner to zfs rollback. snapper also has an `undochange` subcommand which is closer to what I wanted but I think its confusing for users because it undoes changes between two snapshots instead of reverting to one. In many cases this would require using rsync to work out what would happen wheh using `snapper undochange`.

I was unable to find a program to use alongside snapper that made it easy to restore users home directories to a previous snapshot state. I don't want a new snapshot or subvol to be created when I do a rollback which is why I wrote srt, to effectively rollback without having to deal with handling more subvols or trying to work out what result undochange might give me.

SRT functions a bit like `zfs rollback`, reverting all files in the current users home directory to how they were as per a pre-defined snapper timeline created snapshot but it doesn't destroy the intervening snapshots so you could potentially revert to a newer snapshot after reverting to an older one, if available. A new snapshot or subvol is not created when you rollback with SRT.

## Required Snapper configuration

You do not need to have root permissions to run SRT. It requires that your user has read access to their own `~/.snapshots` directory.

This tool cannot work without the following snapper configuration which needs to be applied for every user who wants to run srt.

This script presumes your users snapper config name matches the Linux user name of the user who wants to run srt.

```
chown root:USERS_GID /home/USER/.snapshots/
chmod g+rx /home/USER/.snapshots/
```

You would need to replace USERS_GID with your Linux users GID and USER the srt users Linux user name. You can find your users GID by running the `id` command.

Your user also needs permission to run the snapper tool to list the snapshots date and time:

```
snapper -c USER set-config ALLOW_USERS=USER
```

Here you replace USER with your user name to enable your user to be able to query snapper for snapshot info.

## Using srt

With a suitable snapper timeline config in place and the above snapper configuration per Linux user, you can copy `srt.sh` into your $PATH and make it executable so that users can just run `srt` to pick a snapshot to restore by following the on screen instructions and using the cursor keys and ENTER to select options.
