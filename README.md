# pam_shepherd

A minimal PAM module to automatically start GNU Shepherd for user sessions.

## Overview

`pam_shepherd` is a lightweight PAM session module that starts the GNU Shepherd daemon when a user logs in. It implements a simple fire-and-forget pattern: start shepherd if not running, don't care about cleanup.

## Features

- Starts GNU Shepherd daemon on user login if not already running
- Simple single-fork daemon pattern
- No session tracking or reference counting
- Redirects output to /dev/null (shepherd handles its own logging)
- Fail-fast privilege dropping for security

## Installation

```bash
make
sudo make install
```

The module will be installed to the appropriate PAM module directory (automatically detected, e.g., `/lib64/security/` on x86_64 systems).

## Configuration

Add the following line to your PAM configuration file (typically `/etc/pam.d/system-login`):

```
session  optional  pam_shepherd.so
```

## How It Works

1. Checks if a Shepherd socket exists at `/run/user/$UID/shepherd/socket`
2. If no socket exists, forks and starts shepherd as a daemon
3. The child process:
   - Becomes a session leader with `setsid()`
   - Redirects stdin/stdout/stderr to `/dev/null`
   - Drops privileges to the user
   - Executes shepherd with proper environment
4. Shepherd continues running after logout (fire-and-forget)

## Implementation Details

- **No double-fork**: Uses simple fork + setsid (PAM process is short-lived, child gets reparented to init)
- **No zombie processes**: Parent returns immediately, init adopts the daemon
- **Privilege separation**: Drops root privileges before executing shepherd
- **Minimal dependencies**: Only requires standard PAM and system libraries

## Requirements

- GNU Shepherd installed and available in PATH
- PAM-enabled system
- `/run/user/$UID` directory (usually created by systemd-logind or elogind)

## License

ISC License