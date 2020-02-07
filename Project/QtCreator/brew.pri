USE_BREW = $$(QCTOOLS_USE_BREW)

QCTOOLS_USE_BREW_NOT_EMPTY = false

!isEmpty(USE_BREW) {
    message("USE_BREW not empty")
    QCTOOLS_USE_BREW_NOT_EMPTY = true
}

QCTOOLS_USE_BREW_EQUALS_TRUE = false

equals(USE_BREW, true) {
    message("USE_BREW equals true")
    QCTOOLS_USE_BREW_EQUALS_TRUE = true
}

message("QCTOOLS_USE_BREW_NOT_EMPTY = " $$QCTOOLS_USE_BREW_NOT_EMPTY )
message("QCTOOLS_USE_BREW_EQUALS_TRUE = " $$QCTOOLS_USE_BREW_EQUALS_TRUE )

!isEmpty(USE_BREW):equals(USE_BREW, true) {
    message("DEFINES += USE_BREW")
    DEFINES += USE_BREW
}

macx:contains(DEFINES, USE_BREW) {

    message("use brew")
    CONFIG += qwt release

    QMAKE_TARGET_BUNDLE_PREFIX = org.bavc
    QT_CONFIG -= no-pkg-config

    include ( $$system(brew --prefix qwt)/features/qwt.prf )

    CONFIG += link_pkgconfig

} else {
    CONFIG += debug_and_release
}
