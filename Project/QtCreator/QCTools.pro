TEMPLATE = subdirs

SUBDIRS = \
        qctools-lib \
        qctools-cli \
        qctools-gui

qctools-lib.subdir = qctools-lib
qctools-cli.subdir = qctools-cli
qctools-gui.subdir = qctools-gui

qctools-cli.depends = qctools-lib
qctools-gui.depends = qctools-lib
