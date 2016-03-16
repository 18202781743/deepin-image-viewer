#pragma once

#include "module/modulepanel.h"
#include "imagewidget.h"

class ViewPanel : public ModulePanel
{
    Q_OBJECT
public:
    explicit ViewPanel(QWidget *parent = 0);

    QWidget *toolbarBottomContent() Q_DECL_OVERRIDE;
    QWidget *toolbarTopLeftContent() Q_DECL_OVERRIDE;
    QWidget *toolbarTopMiddleContent() Q_DECL_OVERRIDE;
    QWidget *extensionPanelContent() Q_DECL_OVERRIDE;

    void updateToolbarContent();

private Q_SLOTS:
    void openImage(const QString& path);

private:
    ImageWidget *m_view = NULL;
};