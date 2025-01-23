USE_BREW = $$(QCTOOLS_USE_BREW)
USE_SYSTEM = $${QCTOOLS_USE_SYSTEM}

QCTOOLS_USE_BREW_NOT_EMPTY = false
QCTOOLS_USE_SYSTEM_NOT_EMPTY = false

!isEmpty(USE_BREW) {
    message("USE_BREW not empty")
    QCTOOLS_USE_BREW_NOT_EMPTY = true
}

!isEmpty(USE_SYSTEM) {
    message("USE_SYSTEM not empty")
    QCTOOLS_USE_SYSTEM_NOT_EMPTY = true
}

QCTOOLS_USE_BREW_EQUALS_TRUE = false
QCTOOLS_USE_SYSTEM_EQUALS_TRUE = false

equals(USE_BREW, true) {
    message("USE_BREW equals true")
    QCTOOLS_USE_BREW_EQUALS_TRUE = true
}

equals(USE_SYSTEM, true) {
    message("USE_SYSTEM equals true")
    QCTOOLS_USE_SYSTEM_EQUALS_TRUE = true
}

message("QCTOOLS_USE_BREW_NOT_EMPTY = " $$QCTOOLS_USE_BREW_NOT_EMPTY )
message("QCTOOLS_USE_BREW_EQUALS_TRUE = " $$QCTOOLS_USE_BREW_EQUALS_TRUE )

message("QCTOOLS_USE_SYSTEM_NOT_EMPTY = " $$QCTOOLS_USE_SYSTEM_NOT_EMPTY )
message("QCTOOLS_USE_SYSTEM_EQUALS_TRUE = " $$QCTOOLS_USE_SYSTEM_EQUALS_TRUE )

!isEmpty(USE_BREW):equals(USE_BREW, true) {
    message("DEFINES += USE_BREW")
    DEFINES += USE_BREW
}

!isEmpty(USE_SYSTEM):equals(USE_SYSTEM, true) {
    message("DEFINES += USE_SYSTEM")
    DEFINES += USE_SYSTEM
}

macx:contains(DEFINES, USE_BREW) {

    message("use brew")
    CONFIG += qwt release

    QMAKE_TARGET_BUNDLE_PREFIX = org.bavc
    QT_CONFIG -= no-pkg-config

    include ( $$system(brew --prefix qwt)/features/qwt.prf )

    CONFIG += link_pkgconfig

} else:unix:contains(DEFINES, USE_SYSTEM) {

    message("use system libraries")
    PKGCONFIG += Qt5Qwt6

    CONFIG += link_pkgconfig
} else {
    CONFIG += debug_and_release
}
