# Ubuntu 24.04 x64 installation

Install runtime dependencies:

```sh
sudo apt-get update
sudo apt-get install libgtk-3-0t64 librsvg2-common
./anteater-chess
```

Run from a graphical desktop. Keep the archive contents together. Logs go under $XDG_STATE_HOME/AnteaterChess-Reborn/logs (default ~/.local/state). The runtime uses system GTK libraries. See COPYRIGHT for application terms.
