# Ubuntu 24.04 x64 installation

Install runtime dependencies:

```sh
sudo apt-get update
sudo apt-get install libqt6quick6 libqt6quickcontrols2-6 libqt6svg6 qml6-module-qtquick qml6-module-qtquick-window qml6-module-qtquick-controls qml6-module-qtquick-layouts qml6-module-qtquick-templates qml6-module-qtqml-workerscript
./anteater-chess
```

Run from a graphical desktop and extract to a writable directory. Keep the archive
contents together. Game logs are created in `logs/` beside the actual executable,
including when launched through a symlink or from another working directory.
Move that directory with the program to retain its logs. An unwritable log directory
reports a diagnostic while game actions still apply. The runtime uses system Qt
libraries. See COPYRIGHT for application terms.
