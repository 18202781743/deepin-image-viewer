// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtGraphicalEffects 1.0
import org.deepin.dtk 1.0
import org.deepin.image.viewer 1.0 as IV

Item {
    id: thumbnailView

    // 除ListView外其它按键的占用宽度
    property int btnContentWidth: switchArrowLayout.width + leftRowLayout.width + rightRowLayout.width + deleteButton.width
    property bool imageIsNull: null === targetImage

    // 用于外部获取当前缩略图栏内容的长度，用于布局, 10px为焦点缩略图不在ListView中的边框像素宽度(radius = 4 * 1.25)
    property int listContentWidth: bottomthumbnaillistView.contentWidth + 10
    property Image targetImage

    function deleteCurrentImage() {
        if (!IV.FileControl.deleteImagePath(IV.GControl.currentSource)) {
            // 取消删除文件
            return;
        }
        IV.GControl.removeImage(IV.GControl.currentSource);
        if (0 === IV.GControl.imageCount) {
            stackView.switchOpenImage();
        }
    }

    function next() {
        // 切换时滑动视图不响应拖拽等触屏操作
        IV.GStatus.viewInteractive = false;
        IV.GControl.nextImage();
        IV.GStatus.viewInteractive = true;
    }

    function previous() {
        // 切换时滑动视图不响应拖拽等触屏操作
        IV.GStatus.viewInteractive = false;
        IV.GControl.previousImage();
        IV.GStatus.viewInteractive = true;
    }

    Binding {
        delayed: true
        property: "thumbnailVaildWidth"
        target: IV.GStatus
        value: window.width - 20 - thumbnailListView.btnContentWidth
    }

    Row {
        id: switchArrowLayout

        leftPadding: 10
        spacing: 10

        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
        }

        IconButton {
            id: previousButton

            ToolTip.delay: 500
            ToolTip.text: qsTr("Previous")
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            enabled: IV.GControl.hasPreviousImage
            height: 50
            icon.name: "icon_previous"
            width: 50

            onClicked: {
                previous();
            }

            Shortcut {
                sequence: "Left"

                onActivated: previous()
            }
        }

        IconButton {
            id: nextButton

            ToolTip.delay: 500
            ToolTip.text: qsTr("Next")
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            enabled: IV.GControl.hasNextImage
            height: 50
            icon.name: "icon_next"
            width: 50

            onClicked: next()

            Shortcut {
                sequence: "Right"

                onActivated: {
                    next();
                }
            }
        }
    }

    Row {
        id: leftRowLayout

        leftPadding: 40
        rightPadding: 20
        spacing: 10

        anchors {
            left: switchArrowLayout.right
            verticalCenter: parent.verticalCenter
        }

        IconButton {
            id: fitImageButton

            ToolTip.delay: 500
            ToolTip.text: qsTr("Original size")
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            anchors.leftMargin: 30
            enabled: !imageIsNull
            height: 50
            icon.name: "icon_11"
            width: 50

            onClicked: {
                imageViewer.fitImage();
                imageViewer.recalculateLiveText();
            }
        }

        IconButton {
            id: fitWindowButton

            ToolTip.delay: 500
            ToolTip.text: qsTr("Fit to window")
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            enabled: !imageIsNull
            height: 50
            icon.name: "icon_self-adaption"
            width: 50

            onClicked: {
                imageViewer.fitWindow();
                imageViewer.recalculateLiveText();
            }
        }

        IconButton {
            id: rotateButton

            ToolTip.delay: 500
            ToolTip.text: qsTr("Rotate")
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            enabled: !imageIsNull && IV.FileControl.isRotatable(IV.GControl.currentSource)
            height: 50
            icon.name: "icon_rotate"
            width: 50

            onClicked: {
                imageViewer.rotateImage(-90);
            }
        }
    }

    ListView {
        id: bottomthumbnaillistView

        property bool lastIsMultiImage: false

        // 重新定位图片位置
        function rePositionView() {
            // 特殊处理，防止默认显示首个缩略图时采用Center的策略会被遮挡部分
            if (0 === currentIndex) {
                positionViewAtBeginning();
            } else {
                // 尽可能将高亮缩略图显示在列表中
                positionViewAtIndex(currentIndex, ListView.Center);
            }
        }

        cacheBuffer: 60
        clip: true
        focus: true
        height: thumbnailView.height + 10
        highlightFollowsCurrentItem: true
        // 使用范围模式，允许高亮缩略图在preferredHighlightBegin~End的范围外，使缩略图填充空白区域
        highlightRangeMode: ListView.ApplyRange
        model: IV.GControl.globalModel
        orientation: Qt.Horizontal
        preferredHighlightBegin: width / 2 - 25
        preferredHighlightEnd: width / 2 + 25
        spacing: 4
        width: thumbnailView.width - thumbnailView.btnContentWidth

        delegate: Loader {
            id: thumbnailItemLoader

            property url imageSource: model.imageUrl

            active: true
            asynchronous: true
            // NOTE:需设置默认的 Item 大小，以便于 ListView 计算 contentWidth
            // 防止 positionViewAtIndex() 时 Loader 加载，contentWidth 变化
            // 导致定位异常，同时 Delegate 使用 state 切换控件宽度
            width: Loader.Ready === status ? item.width : 30

            onActiveChanged: {
                if (active && imageInfo.delegateSource) {
                    setSource(imageInfo.delegateSource, {
                            "source": thumbnailItemLoader.imageSource
                        });
                }
            }

            IV.ImageInfo {
                id: imageInfo

                property url delegateSource
                property bool isCurrentItem: thumbnailItemLoader.ListView.isCurrentItem

                function checkDelegateSource() {
                    if (IV.ImageInfo.Ready !== status && IV.ImageInfo.Error !== status) {
                        return;
                    }
                    if (IV.Types.MultiImage === type && isCurrentItem) {
                        delegateSource = "qrc:/qml/ThumbnailDelegate/MultiThumnailDelegate.qml";
                    } else {
                        delegateSource = "qrc:/qml/ThumbnailDelegate/NormalThumbnailDelegate.qml";
                    }
                }

                source: thumbnailItemLoader.imageSource

                onDelegateSourceChanged: {
                    if (thumbnailItemLoader.active && delegateSource) {
                        setSource(delegateSource, {
                                "source": thumbnailItemLoader.imageSource
                            });
                    }
                }

                // 图片被删除、替换，重设当前图片组件
                onInfoChanged: {
                    checkDelegateSource();
                    var temp = delegateSource;
                    delegateSource = "";
                    delegateSource = temp;
                }
                onIsCurrentItemChanged: {
                    checkDelegateSource();

                    // 切换图片涉及多页图时，由于列表内容宽度变更，焦点item定位异常，延迟定位
                    if (IV.Types.MultiImage === type) {
                        bottomthumbnaillistView.lastIsMultiImage = true;
                        delayUpdateTimer.start();
                    } else if (bottomthumbnaillistView.lastIsMultiImage) {
                        delayUpdateTimer.start();
                        bottomthumbnaillistView.lastIsMultiImage = false;
                    }
                }
                onStatusChanged: checkDelegateSource()
            }
        }
        footer: Rectangle {
            width: 5
        }

        // 添加两组空的表头表尾用于占位，防止在边界的高亮缩略图被遮挡, 5px为不在ListView中维护的焦点缩略图边框的宽度 radius = 4 * 1.25
        header: Rectangle {
            width: 5
        }

        Component.onCompleted: {
            bottomthumbnaillistView.currentIndex = IV.GControl.currentIndex;
            forceLayout();
            rePositionView();
        }

        //滑动联动主视图
        onCurrentIndexChanged: {
            if (currentItem) {
                currentItem.forceActiveFocus();
            }

            // 直接定位，屏蔽动画效果
            rePositionView();

            // 仅在边缘缩略图时进行二次定位
            if (0 === currentIndex || currentIndex === (count - 1)) {
                delayUpdateTimer.start();
            }
        }

        anchors {
            left: leftRowLayout.right
            right: rightRowLayout.left
            verticalCenter: parent.verticalCenter
        }

        Connections {
            function onCurrentIndexChanged() {
                bottomthumbnaillistView.currentIndex = IV.GControl.currentIndex;
            }

            target: IV.GControl
        }

        Connections {
            function onFullScreenAnimatingChanged() {
                // 动画结束时处理
                if (!IV.GStatus.fullScreenAnimating) {
                    // 当缩放界面时，缩略图栏重新进行了布局计算，导致高亮缩略图可能不居中
                    if (0 == bottomthumbnaillistView.currentIndex) {
                        bottomthumbnaillistView.positionViewAtBeginning();
                    } else {
                        // 尽可能将高亮缩略图显示在列表中
                        bottomthumbnaillistView.positionViewAtIndex(bottomthumbnaillistView.currentIndex, ListView.Center);
                    }
                }
            }

            target: IV.GStatus
        }

        Timer {
            id: delayUpdateTimer

            interval: 100
            repeat: false

            onTriggered: {
                bottomthumbnaillistView.forceLayout();
                bottomthumbnaillistView.rePositionView();
            }
        }
    }

    Row {
        id: rightRowLayout

        leftPadding: 20
        rightPadding: 20
        spacing: 10

        anchors {
            right: deleteButton.left
            verticalCenter: parent.verticalCenter
        }

        IconButton {
            id: ocrButton

            ToolTip.delay: 500
            ToolTip.text: qsTr("Extract text")
            ToolTip.timeout: 5000
            ToolTip.visible: hovered
            enabled: IV.FileControl.isCanSupportOcr(IV.GControl.currentSource) && !imageIsNull
            height: 50
            icon.name: "icon_character_recognition"
            width: 50

            onClicked: {
                IV.GControl.submitImageChangeImmediately();
                IV.FileControl.ocrImage(IV.GControl.currentSource, IV.GControl.currentFrameIndex);
            }
        }
    }

    IconButton {
        id: deleteButton

        ToolTip.delay: 500
        ToolTip.text: qsTr("Delete")
        ToolTip.timeout: 5000
        ToolTip.visible: hovered
        enabled: IV.FileControl.isCanDelete(IV.GControl.currentSource)
        height: 50
        icon.color: enabled ? "red" : "ffffff"
        icon.name: "icon_delete"
        icon.source: "qrc:/res/dcc_delete_36px.svg"
        width: 50

        onClicked: deleteCurrentImage()

        anchors {
            right: parent.right
            rightMargin: 10
            verticalCenter: parent.verticalCenter
        }
    }
}
