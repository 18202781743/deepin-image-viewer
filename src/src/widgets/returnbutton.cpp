/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "returnbutton.h"
#include "application.h"

#include <QDebug>
#include <QDesktopWidget>
#include <DFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include <QIcon>
#include <QApplication>
#include <DLabel>

DWIDGET_USE_NAMESPACE
typedef DLabel QLbtoDLabel;
typedef DFrame QFrToDFrame;

ReturnButton::ReturnButton(QWidget *parent)
    : QWidget(parent)
    , m_checked(false)
    , m_isPressed(false)
    , m_spacing(2)
    , m_maxWidth(24)
    , m_buttonWidth(24)
{
    onThemeChanged(dApp->viewerTheme->getCurrentTheme());
    connect(this, &ReturnButton::returnBtnWidthChanged, this,
            &ReturnButton::setButtonWidth);
    connect(dApp->viewerTheme, &ViewerThemeManager::viewerThemeChanged,
            this, &ReturnButton::onThemeChanged);
}

QString ReturnButton::normalPic() const
{
    return m_normalPic;
}

QString ReturnButton::hoverPic() const
{
    return m_hoverPic;
}

QString ReturnButton::pressPic() const
{
    return m_pressPic;
}

QString ReturnButton::disablePic() const
{
    return m_disablePic;
}

QString ReturnButton::text() const
{
    return m_text;
}

QColor ReturnButton::normalColor() const
{
    return m_normalColor;
}

QColor ReturnButton::hoverColor() const
{
    return m_hoverColor;
}

QColor ReturnButton::pressColor() const
{
    return m_pressColor;
}

QColor ReturnButton::disableColor() const
{
    return m_disableColor;
}

void ReturnButton::setText(QString text)
{
    m_text = text;
    emit textChanged(text);
    update();
}

bool ReturnButton::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        if (QHelpEvent *he = static_cast<QHelpEvent *>(e)) {
            showTooltip(he->globalPos());

            return false;
        }
    }

    return QWidget::event(e);
}

void ReturnButton::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);

    QMargins m = contentsMargins();
    qreal ration = this->devicePixelRatioF();
    QIcon icon(getPixmap());

    QPixmap pixmap = icon.pixmap(QPixmap(getPixmap()).size());
    pixmap.setDevicePixelRatio(ration);

    const qreal pixWidth = pixmap.width() / ration;
    const qreal pixHeight = pixmap.height() / ration;

    if (! pixmap.isNull()) {
        //修复style，把ph放更小的范围
        int ph = 0;
        if (pixWidth > width() || pixHeight > height()) {
            ph = height() - m.top() - m.bottom();
            const QRect pr(m.left(), (height() - ph) / 2, ph, ph);
            painter.drawPixmap(QPoint(pr.x(), pr.y()), pixmap
                               /*pixmap.scaled(pr.size(), Qt::KeepAspectRatioByExpanding)*/);
        } else {
            ph = pixHeight;
            const QRect pr(m.left(), (height() - ph) / 2, pixWidth, ph);
            painter.drawPixmap(QPoint(pr.x(), pr.y()), pixmap);
        }
    }

    QFontMetrics fm(font());
    int maxWidth = m_maxWidth - pixWidth - 6;
    int textWidth = fm.boundingRect(m_text).width();
    QString mt;
    if (textWidth > maxWidth) {
        mt = fm.elidedText(m_text, Qt::ElideMiddle, maxWidth - 6);
    } else {
        mt = m_text;
    }
    textWidth = fm.boundingRect(mt).width();
    setFixedWidth(textWidth + pixWidth + 6);

    int oldWidth = m_buttonWidth;
    m_buttonWidth = std::max(24, int(textWidth + pixWidth + 6));
    if (oldWidth != m_buttonWidth) {
        emit returnBtnWidthChanged(m_buttonWidth);
    }
    const int th = fm.height();
    QRect textRect = QRect(pixWidth, (height() - th) / 2 - 1, textWidth, pixHeight);
    painter.setPen(QPen(getTextColor()));
    painter.drawText(textRect, Qt::AlignCenter, mt);
    QWidget::paintEvent(e);
}

int ReturnButton::buttonWidth()
{
    return m_buttonWidth;
}

void ReturnButton::setButtonWidth(int width)
{
    m_buttonWidth = width;
}

void ReturnButton::enterEvent(QEvent *e)
{
    Q_UNUSED(e)
    m_currentPic = hoverPic();
    m_currentColor = hoverColor();
    setCursor(Qt::PointingHandCursor);
    this->update();
}

void ReturnButton::leaveEvent(QEvent *e)
{
    Q_UNUSED(e)
    m_currentColor = normalColor();
    m_currentPic = normalPic();
    setCursor(Qt::ArrowCursor);
    this->update();

    emit mouseLeave();
}

void ReturnButton::mousePressEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    m_isPressed = true;
    m_currentColor = pressColor();
    m_currentPic = pressPic();
    this->update();
}

void ReturnButton::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    m_currentColor = normalColor();
    m_currentPic = normalPic();
    this->update();
    if (m_isPressed) {
        m_isPressed = false;
        emit clicked();
    }
}

QSize ReturnButton::sizeHint() const
{
    QPixmap p(getPixmap());
    QMargins m = contentsMargins();
    QFontMetrics fm(font());
    int spacing = p.isNull() ? 0 : m_spacing;
    int h = p.height() + m.top() + m.bottom();
    int w = p.width() + fm.width(m_text) + m.left() + m.right() + spacing + 3;

    return QSize(w, qMax(h, fm.height()));
}

QString ReturnButton::getPixmap() const
{
    if (m_checked) {
        return checkedPic();
    } else if (isEnabled()) {
        return m_currentPic;
    } else {
        return disablePic();
    }
}

QColor ReturnButton::getTextColor() const
{
    if (isEnabled()) {
        return m_currentColor;
    } else {
        return disableColor();
    }
}

void ReturnButton::showTooltip(const QPoint &pos)
{
    QFrToDFrame *tf = new QFrToDFrame();
//    tf->setStyleSheet(this->styleSheet());
    tf->setWindowFlags(Qt::ToolTip);
    tf->setAttribute(Qt::WA_TranslucentBackground);
    QLbtoDLabel *tl = new QLbtoDLabel(tf);
    tl->setObjectName("ButtonTooltip");
    tl->setText(toolTip());
    QHBoxLayout *layout = new QHBoxLayout(tf);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tl);

    tf->show();
    QRect dr = qApp->desktop()->geometry();
    int y = pos.y() + tf->height();
    if (y > dr.y() + dr.height()) {
        y = pos.y() - tf->height() - 10;
    }
    tf->move(pos.x() - tf->width() / 3, y - tf->height() / 3);

    QTimer::singleShot(5000, tf, SLOT(deleteLater()));

    connect(this, &ReturnButton::mouseLeave, tf, &QFrToDFrame::deleteLater);
    connect(this, &ReturnButton::clicked, tf, &QFrToDFrame::deleteLater);
}

void ReturnButton::onThemeChanged(ViewerThemeManager::AppTheme theme)
{
    Q_UNUSED(theme);
}

void ReturnButton::setChecked(bool checked)
{
    m_checked = checked;
    this->update();
}

QString ReturnButton::checkedPic() const
{
    return m_checkedPic;
}

void ReturnButton::setSpacing(int spacing)
{
    m_spacing = spacing;
}
