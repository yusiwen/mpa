# Message Process Adminitrator Framework

## Prerequites

### Resource limits

Add `20-msgqueue.conf` to `/etc/security/limits.d`, it contains:

```
*          soft    msgqueue     819200
<USERNAME> soft    msgqueue     8192000
<USERNAME> hard    msgqueue     8192000
```

`<USERNAME>` is the user who runs the environment.

Reboot to take effect

### Kernel parameters

Add `20-msgqueue.conf` to `/etc/sysctl.d`, it contains:

```
# max queues system wide
kernel.msgmni=1024

# max size of message (bytes)
kernel.msgmax=8192

# default max size of queue (bytes)
kernel.msgmnb=4194304
```

NOTE: `kernel.msgmnb` size must not exceed the number of HARD LIMIT in `/etc/security/limits.d/20-msgqueue.conf`

Reboot to take effect
