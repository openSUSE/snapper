---
title: SSH-Pull Mode for Snapper Backups
author: Arvin Schnell
layout: post
---

Snapper's backup tool snbk uses btrfs send and receive to replicate
snapshots to other filesystems. So far, snbk supported two target
modes: "local" and "ssh-push".

In "ssh-push" mode, the system containing the source snapshots
initiates the transfer and connects to a remote backup server over
ssh. This method has a drawback: if the source machine is compromised,
the attacker has ssh access to the backup server, potentially putting
existing backups at risk.

To address this, snbk now supports a third target mode: "ssh-pull"

With "ssh-pull", the architecture is reversed. The backup server
initiates the connection, pulls the snapshots from the client machine
over ssh, and writes them to its local storage.

Here is an example backup config:

~~~
{
    "config": "root",
    "target-mode": "ssh-pull",
    "automatic": true,
    "source-path": "/",
    "target-path": "/backups/eberich/root",
    "ssh-host": "eberich.example.com",
    "ssh-identity": "/etc/snapper/certs/id_ecdsa"
}
~~~

The snbk commands work identically to the other target modes, but are
executed on the backup server.

The new "ssh-pull" mode is so far only available in the git
repository.
