import QtQuick

import QtQuick.Controls

import com.yacreader.ComicModel 1.0

import QtQuick.Controls.Basic

Rectangle {
    id: main

    color: "transparent"

    //width: parent.width
    //height: parent.height
    anchors.margins: 0

    Rectangle {
        id: info_container
        anchors.fill: parent

        clip: true

        color: infoBackgroundColor

        Flickable{
            id: infoFlickable
            anchors.fill: parent
            anchors.margins: 0

            contentWidth: infoView.width
            contentHeight: infoView.height

            ComicInfoView {
                id: infoView
                width: info_container.width - 14
            }

            WheelHandler {
                onWheel: {
                    if (infoFlickable.contentHeight <= infoFlickable.height) {
                        return;
                    }

                    var newValue =  Math.min((infoFlickable.contentHeight - infoFlickable.height), (Math.max(infoFlickable.originY , infoFlickable.contentY - event.angleDelta.y)));
                    infoFlickable.contentY = newValue;
                }
            }

            ScrollBar.vertical: ScrollBar {
                visible: infoFlickable.contentHeight > infoFlickable.height

                contentItem: Item {
                    implicitWidth: 12
                    implicitHeight: 26
                    Rectangle {
                        color: infoScrollbarColor
                        anchors.fill: parent
                        anchors.topMargin: 6
                        anchors.leftMargin: 5
                        anchors.rightMargin: 4
                        anchors.bottomMargin: 6
                        radius: 2
                    }
                }
            }
        }
    }

    ComicsDropArea {
        anchors.fill: parent
    }
}
