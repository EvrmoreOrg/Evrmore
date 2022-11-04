
Debian
====================
This directory contains files used to package evrmored/evrmore-qt
for Debian-based Linux systems. If you compile evrmored/evrmore-qt yourself, there are some useful files here.

## evrmore: URI support ##


evrmore-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install evrmore-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your evrmore-qt binary to `/usr/bin`
and the `../../share/pixmaps/evrmore128.png` to `/usr/share/pixmaps`

evrmore-qt.protocol (KDE)

