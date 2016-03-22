import QtQuick 2.4
import Ubuntu.Components 1.3
import TweakGeek 1.0
import QtQuick.Layouts 1.1

MainView {
    // objectName for functional testing purposes (autopilot-qt5)
    objectName: "mainView"

    // Note! applicationName needs to match the "name" field of the click manifest
    applicationName: "keeprunningapp.liu-xiao-guo"

    width: units.gu(60)
    height: units.gu(85)

    property int index: 0
    property var myapp

    ApplicationModel {
        id: applicationModel
    }

    Timer {
        interval: 500; running: true; repeat: true
        onTriggered: {
            index ++
            console.log("index: " + index)
        }
    }

    function findMyApp() {
        var count = applicationModel.count();
        for ( var i = 0 ; i < count; i++ ) {
            var app = applicationModel.get(i)
            // console.log("app name: " + app.name)
            if ( app.name === "keeprunningapp" ) {
                // if our own app is found, make it not die
                console.log("My app setLifecycleException: " + app.lifecycleException)
                myapp = app
            }
        }
    }

    Page {
        header: PageHeader {
            title: "keeprunningapp"
        }

        Connections {
            target: Qt.application
            onActiveChanged: {
                console.log("Qt.application.active: " + Qt.application.active);
            }
        }

        Column {
            anchors.centerIn: parent
            spacing: units.gu(2)

            Label {
                text: "current app is: " + Qt.application.active
                fontSize: "large"
            }

            Label {
                text: "index: " + index
                fontSize: "large"
            }

            RowLayout {
                spacing: units.gu(5)

                Label {
                    text: "Prevent suspending"
                    Layout.fillWidth: true
                    fontSize: "large"
                }

                Switch {
                    checked: myapp.lifecycleException
                    onClicked: myapp.lifecycleException = checked
                }
            }
        }
    }

    Component.onCompleted: {
        findMyApp();
    }
}
