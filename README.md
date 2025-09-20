# pam_shepherd

A minimal PAM module to automatically start GNU Shepherd for user sessions.

## Overview

`pam_shepherd` is a lightweight PAM session module that starts the GNU Shepherd daemon when a user logs in. It implements a simple fire-and-forget pattern: start shepherd if not running, don't care about cleanup.

## Features

- Starts GNU Shepherd daemon on user login if not already running
- Proper double-fork daemon pattern with `waitpid()`
- No session tracking or reference counting
- No zombie processes - intermediate child is properly reaped
- Fail-fast privilege dropping for security
- Fire-and-forget: shepherd keeps running after logout

## Installation

### From Source

```bash
make
sudo make install
```

The module will be installed to the appropriate PAM module directory (automatically detected, e.g., `/lib64/security/` on x86_64 systems).


## Configuration

Add the following line to your PAM configuration file:

```
session  optional  pam_shepherd.so
```

Common locations:
- `/etc/pam.d/system-login` (Gentoo/OpenRC)
- `/etc/pam.d/common-session` (Debian/Devuan)

## How It Works

1. **Checks for existing instance**: Looks for shepherd socket at `/run/user/$UID/shepherd/socket`
2. **Double-fork pattern**: If no socket exists, uses proper double-fork to daemonize:
   - Parent forks and waits for intermediate child with `waitpid()`
   - Intermediate child forks again and exits immediately
   - Grandchild becomes session leader and runs shepherd
3. **Daemon setup**:
   - Calls `setsid()` to detach from terminal
   - Redirects stdin/stdout/stderr to `/dev/null`
   - Drops root privileges (setgroups → setgid → setuid)
   - Sets up environment (XDG_RUNTIME_DIR, HOME, USER, PATH, LANG)
   - Executes shepherd via `execlp()`
4. **Fire-and-forget**: Shepherd continues running after logout

## Implementation Details

- **Double-fork with waitpid**: Prevents zombies by properly reaping intermediate child
- **No signal manipulation**: Avoids side effects from changing SIGCHLD handler
- **Proper privilege dropping**: Clears supplementary groups before changing uid/gid
- **Minimal dependencies**: Only requires standard PAM and system libraries
- **Clean and simple**: Follows UNIX philosophy

## Requirements

- GNU Shepherd installed and available in PATH
- PAM-enabled system
- `/run/user/$UID` directory (usually created by systemd-logind or elogind)

## Version History

- **0.2.0**: Stable double-fork implementation with waitpid
- **0.1.1**: Added /run/user directory creation
- **0.1.0**: Initial release

## License

ISC License

## Repository

https://github.com/Aromate/pam_shepherd