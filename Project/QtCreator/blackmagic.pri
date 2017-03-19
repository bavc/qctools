USE_BLACKMAGIC = $$(QCTOOLS_USE_BLACKMAGIC)
equals(USE_BLACKMAGIC, true) {
    message("QCTOOLS_USE_BLACKMAGIC is true, blackmagic integration enabled ")
    DEFINES += BLACKMAGICDECKLINK_YES

    win32 {
        IDL = "$$THIRD_PARTY_PATH/Blackmagic DeckLink SDK/Win/include/DeckLinkAPI.idl"
        idl_c.output = ${QMAKE_FILE_IN}.h
        idl_c.input = IDL
        idl_c.commands = $${QMAKE_IDL} ${QMAKE_FILE_IN} $${IDLFLAGS} \
                         /h ${QMAKE_FILE_IN}.h /iid ${QMAKE_FILE_IN}.c
        idl_c.variable_out = SOURCES
        idl_c.name = MIDL
        idl_c.clean = ${QMAKE_FILE_IN}.h ${QMAKE_FILE_IN}.c
        idl_c.CONFIG = no_link target_predeps

        QMAKE_EXTRA_COMPILERS += idl_c

        LIBS += -lOle32
    }

    linux:SOURCES += "$$THIRD_PARTY_PATH/Blackmagic DeckLink SDK/Linux/include/DeckLinkAPIDispatch.cpp"
    macx:!contains(DEFINES, USE_BREW) SOURCES += "$$THIRD_PARTY_PATH/Blackmagic DeckLink SDK/Mac/include/DeckLinkAPIDispatch.cpp"

    HEADERS += \
        $$SOURCES_PATH/Core/BlackmagicDeckLink.h \
        $$SOURCES_PATH/Core/BlackmagicDeckLink_Glue.h

    SOURCES += \
        $$SOURCES_PATH/Core/BlackmagicDeckLink.cpp \
        $$SOURCES_PATH/Core/BlackmagicDeckLink_Glue.cpp

} else {
    message("QCTOOLS_USE_BLACKMAGIC is not true, blackmagic integration disabled")
}

macx:contains(DEFINES, USE_BREW) {
    message("don't use Blackmagic DeckLink SDK for brew build")
} else {
    INCLUDEPATH += "$$THIRD_PARTY_PATH/Blackmagic DeckLink SDK"
}
