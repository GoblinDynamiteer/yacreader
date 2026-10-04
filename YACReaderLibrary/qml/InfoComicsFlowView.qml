import QtQuick

// Top part of the info view: the covers flow and the indicator pointing to the current comic.
// The info panel is shown in a different view, right below this one.
// The covers are sized to the height of the view, which can be resized with a splitter.
Rectangle {
    id: main

    color: "transparent"

    // Default height
    implicitHeight: 256 + indicator.height - 6

    anchors.margins: 0

    FlowView {
        id: flow
        objectName: "flow"
        height: main.height - indicator.height + 6

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        additionalBottomSpace: indicator.height
    }

    Image {
        id: top_shadow
        source: topShadow
        width: parent.width
        fillMode: Image.TileHorizontally
    }

    Rectangle {
        id: indicator_container
        width: parent.width
        y: flow.height - 6

        Image {
            id: indicator
            source: infoIndicator
        }

        Image {
            id: bottom_shadow
            x: indicator.width
            width: parent.width - indicator.width
            source: infoShadow
            fillMode: Image.TileHorizontally
        }
    }

    ComicsDropArea {
        anchors.fill: parent
    }
}
