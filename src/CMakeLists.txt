add_executable(musik)

ecm_add_qml_module(musik
    URI
    org.koderoots.musik
)

target_sources(musik
    PRIVATE
    main.cpp
    audioplayer.cpp
    audioplayer.h
    playlistmodel.cpp
    playlistmodel.h
)

ecm_target_qml_sources(musik
    SOURCES
    qml/Main.qml
)

target_link_libraries(musik
    PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::Gui
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::Multimedia
    KF6::I18n
    KF6::CoreAddons
    KF6::IconThemes
    TagLib::TagLib
)

install(TARGETS musik ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
