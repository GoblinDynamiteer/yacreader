import QtQuick

// Imports dropped comics, or uses a dropped image as the cover of the current comic
DropArea {
    onEntered: {
        if(drag.hasUrls)
        {
            if(dropManager.canDropImage(drag.urls)
                    || dropManager.canDropUrls(drag.urls, drag.action))
            {
                drag.accepted = true;
            }else
                drag.accepted = false;
        }
    }

    onDropped: {
        if(drop.hasUrls && dropManager.canDropImage(drop.urls))
        {
            dropManager.droppedImage(drop.urls);
            drop.accepted = true;
        }
        else if(drop.hasUrls && dropManager.canDropUrls(drop.urls, drop.action))
        {
            dropManager.droppedFiles(drop.urls, drop.action);
        }
    }
}
