# WebUSB on Linux

Linux may deny the browser access to an OpenPuck USB device even after the
device chooser succeeds. Chromium-family browsers report the underlying error
in `chrome://device-log/`; an `Operation not permitted` error indicates that a
narrow udev access rule may be required.

## Rule template

Review `tools/udev/50-openpuck.rules` before installing it. The template:

- matches exact VID/PID pairs used by OpenPuck modes with WebUSB;
- includes exact Adafruit Feather nRF52840 IDs declared by the pinned core;
- uses the active desktop session's `uaccess` ACL;
- does not use `MODE="0666"` or grant access to every device from a vendor.

OpenPuck deliberately impersonates existing controllers. A rule for an
impersonated VID/PID also applies to a real controller with the same identity.
Delete every application-mode line that is not needed on the host. The Valve
`28de:1304` line is sufficient for the default Steam mode.

Install the reviewed file:

```sh
sudo install -m 0644 tools/udev/50-openpuck.rules \
  /etc/udev/rules.d/50-openpuck.rules
sudo udevadm control --reload-rules
```

Then unplug and reconnect the device. Some desktop/session configurations may
require logging out and back in before `uaccess` ACLs appear.

Inspect a connected device before expanding the template:

```sh
arduino-cli board list
udevadm info --attribute-walk --name=/dev/ttyACM0
```

Do not add a broad vendor-only rule. Do not make all USB devices world
writable.

Snap-packaged Chromium may also need:

```sh
sudo snap connect chromium:raw-usb
```

That sandbox permission is independent of udev.
