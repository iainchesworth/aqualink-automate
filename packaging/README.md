# Linux packaging — system integration files

These files turn the `.deb` / `.rpm` / `.tar.gz` from "a binary in `/usr/bin`" into a
managed **systemd service** with a service account, `/etc` config, and serial-port
access. They are wired into CPack in [`cmake/CPackConfig.cmake`](../cmake/CPackConfig.cmake)
(Linux branch).

| File | Installed to (deb/rpm) | Purpose |
|------|------------------------|---------|
| `systemd/aqualink-automate.service` | `/usr/lib/systemd/system/` | `Type=simple` unit; runs as `aqualink`, reads `/etc/.../aqualink-automate.conf`, logs to journald, hardened, `dialout` for serial. |
| `config/aqualink-automate.conf` | `/etc/aqualink-automate/` (conffile / `%config(noreplace)`) | Default config. Listens on `0.0.0.0:9000`, HTTPS off; serial port is left for the admin to set. |
| `sysusers.d/aqualink-automate.conf` | `/usr/lib/sysusers.d/` | Declares the `aqualink` system user + `dialout` membership. |
| `udev/60-aqualink-automate.rules.example` | `…/share/doc/aqualink-automate/examples/` | Template for a stable `/dev/aqualink-rs485` symlink (admin copies + edits). |
| `deb/{postinst,prerm,postrm,conffiles}` | deb control archive | Create the account, enable the unit; never auto-start on a *fresh* install (the service needs a serial port first). |
| `rpm/{post.sh,preun.sh}` | rpm scriptlets | Same behaviour for the `.rpm`. |
| `tarball/{install,uninstall}.sh` | `…/share/aqualink-automate/packaging/tarball/` (+ `install.sh` at the tarball root) | The relocatable `.tar.gz` has no maintainer scripts, so these do the same job by hand (install to `/opt/aqualink-automate`, template the unit's `ExecStart`). |

## First-run behaviour (deb/rpm)

A fresh install creates the service and **enables it on boot** but does **not start it**,
because the service is useless (and would `Restart=on-failure` loop) until a serial port
is configured. The postinst prints the two next steps:

```
sudo nano /etc/aqualink-automate/aqualink-automate.conf   # set serial-port = /dev/ttyUSB0
sudo systemctl start aqualink-automate
journalctl -u aqualink-automate -f
```

An *upgrade* restarts the service only if it was already running.
