[![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://gitlab.xfce.org/xfce/libxfce4ui/-/blob/master/COPYING)

# libxfce4ui


The libxfce4ui library is used to share commonly used Xfce widgets among the Xfce applications.

----

### Homepage

[Libxfce4ui documentation](https://docs.xfce.org/xfce/libxfce4ui/start)

### Changelog

See [NEWS](https://gitlab.xfce.org/xfce/libxfce4ui/-/blob/master/NEWS) for details on changes and fixes made in the current release.

### Source Code Repository

[Libxfce4ui source code](https://gitlab.xfce.org/xfce/libxfce4ui)

### Download a Release Tarball

[Libxfce4ui archive](https://archive.xfce.org/src/xfce/libxfce4ui)
    or
[Libxfce4ui tags](https://gitlab.xfce.org/xfce/libxfce4ui/-/tags)

### Installation

From source: 

    % cd libxfce4ui
    % meson setup build
    % meson compile -C build
    % meson install -C build

From release tarball:

    % tar xf libxfce4ui-<version>.tar.xz
    % cd libxfce4ui-<version>
    % meson setup build
    % meson compile -C build
    % meson install -C build

### Uninstallation

    % ninja uninstall -C build

### Reporting Bugs

Visit the [reporting bugs](https://docs.xfce.org/xfce/libxfce4ui/bugs) page to view currently open bug reports and instructions on reporting new bugs or submitting bugfixes.

