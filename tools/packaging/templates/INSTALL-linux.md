# Ubuntu 24.04 x64 installation

Install runtime dependencies:

```sh
sudo apt-get update
sudo apt-get install libgtk-3-0t64 librsvg2-common
./anteater-chess
```

Run from a graphical desktop and extract to a writable directory. Keep the archive
contents together. Game logs are created in `logs/` beside the actual executable,
including when launched through a symlink or from another working directory.
Move that directory with the program to retain its logs. An unwritable log directory
reports a diagnostic while game actions still apply. The runtime uses system GTK
libraries. See COPYRIGHT for application terms.
