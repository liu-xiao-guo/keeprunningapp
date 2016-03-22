TEMPLATE = app
TARGET = keeprunningapp

load(ubuntu-click)

CONFIG += c++11

QT += qml quick

INCLUDEPATH += /usr/include/click-0.4/ \
    /usr/include/glib-2.0/ \
    /usr/lib/x86_64-linux-gnu/glib-2.0/include \
    /usr/lib/arm-linux-gnueabihf/glib-2.0/include \
    /usr/include/json-glib-1.0 \
    /usr/include/x86_64-linux-gnu/qt5/QGSettings/ \
    /usr/include/arm-linux-gnueabihf/qt5/QGSettings/

SOURCES += main.cpp \
           applicationmodel.cpp

HEADERS += \
    applicationmodel.h

RESOURCES += keeprunningapp.qrc

QML_FILES += $$files(*.qml,true) \
             $$files(*.js,true)

CONF_FILES +=  keeprunningapp.apparmor \
               keeprunningapp.png

AP_TEST_FILES += tests/autopilot/run \
                 $$files(tests/*.py,true)

#show all the files in QtCreator
OTHER_FILES += $${CONF_FILES} \
               $${QML_FILES} \
               $${AP_TEST_FILES} \
               keeprunningapp.desktop

LIBS += -lglib-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lclick-0.4 -lgsettings-qt

#specify where the config files are installed to
config_files.path = /keeprunningapp
config_files.files += $${CONF_FILES}
INSTALLS+=config_files

#install the desktop file, a translated version is 
#automatically created in the build directory
desktop_file.path = /keeprunningapp
desktop_file.files = $$OUT_PWD/keeprunningapp.desktop
desktop_file.CONFIG += no_check_exist
INSTALLS+=desktop_file

# Default rules for deployment.
target.path = $${UBUNTU_CLICK_BINARY_PATH}
INSTALLS+=target

