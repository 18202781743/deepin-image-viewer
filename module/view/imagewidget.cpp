#include "imagewidget.h"
#include <QPainter>
#include <QtDebug>
#include <QFileInfo>
#include <QDesktopWidget>
#include <QApplication>
#include <QMouseEvent>
#include "utils/imageutils.h"
#include "controller/databasemanager.h"

ImageWidget::ImageWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
}

void ImageWidget::setImage(const QString &path)
{
    setImage(QImage(path));
    m_path = path;
}

QString ImageWidget::imagePath() const
{
    return m_path;
}

void ImageWidget::setImage(const QImage &image)
{
    m_path = QString();
    m_image = image;
    m_pixmap = QPixmap::fromImage(m_image);
    resetTransform();
}

QString ImageWidget::imageName() const
{
    return QFileInfo(m_path).fileName();
}

void ImageWidget::resetTransform()
{
    //m_scale = 0.4;
    m_o_dev = rect().center();
    m_flipX = m_flipY = 1;
    m_rot = 0;
    if (m_image.isNull())
        return;
    const QSize s = m_image.size().scaled(rect().size(), Qt::KeepAspectRatio);
    m_o_img = QPoint(m_image.width()/2, m_image.height()/2);
    m_scale = qreal(s.width())/qreal(m_image.size().width());
    updateTransform();
}

void ImageWidget::setImageMove(int x, int y)
{
    setTransformOrigin(QPoint(x, y), QPoint());
}

void ImageWidget::setScaleValue(qreal value)
{
    if (m_scale == value)
        return;

    m_scale = value;
    Q_EMIT scaleValueChanged(value);
    updateTransform();
}

void ImageWidget::rotateClockWise()
{
    rotate(m_rot+90);
    utils::image::rotate(m_path, 90);
    DatabaseManager::instance()->updateThumbnail(imageName());
}

void ImageWidget::rotateCounterclockwise()
{
    rotate(m_rot-90);
    utils::image::rotate(m_path, -90);
    DatabaseManager::instance()->updateThumbnail(imageName());
}

void ImageWidget::rotate(int deg)
{
    if (m_rot == deg%360)
        return;
    m_rot = deg%360;
    updateTransform();
}

void ImageWidget::flipX()
{
    m_flipX = -m_flipX;
    updateTransform();
    Q_EMIT fliped(m_flipX < 0, m_flipY < 0);
}

void ImageWidget::flipY()
{
    m_flipY = -m_flipY;
    updateTransform();
    Q_EMIT fliped(m_flipX < 0, m_flipY < 0);
}

void ImageWidget::setTransformOrigin(const QPoint& imageP, const QPoint& deviceP)
{
    if (m_o_dev == deviceP && m_o_img == imageP)
        return;
    m_o_dev = deviceP;
    m_o_img = imageP;
    updateTransform();
}

QPoint ImageWidget::mapToImage(const QPoint &p) const
{
    return m_mat.inverted().map(p);
}

QRect ImageWidget::mapToImage(const QRect& r) const
{
    return m_mat.inverted().mapRect(r);
}

QRect ImageWidget::visibleImageRect() const
{
    return mapToImage(rect()) & QRect(0, 0, m_image.width(), m_image.height());
}

bool ImageWidget::isWholeImageVisible() const
{
    return visibleImageRect().size() == m_image.size();
}

bool ImageWidget::isMoving() const
{
    return m_moving;
}

void ImageWidget::timerEvent(QTimerEvent *e)
{
    if (e->timerId() != m_tid)
        return;
    m_scaling = false;
    update();
}

void ImageWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.save();

    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setTransform(m_mat);
    p.drawPixmap(0, 0, m_pixmap);
    p.restore();
    if (!m_scaling)
        return;
//    static const int kTipWidth = 60;
//    static const int kTipHeight = 30;
//    p.translate(width() - kTipWidth, 100);
//    p.fillRect(0, 0, kTipWidth, kTipHeight, QColor(0, 10, 0, 168));
//    p.setPen(Qt::white);
//    p.drawText(QRect(0, 0, kTipWidth, kTipHeight),
//               QString("%1%").arg(int(m_scale*100.0)),
//               QTextOption(Qt::AlignCenter));
}

void ImageWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_pos = event->pos();
        m_posG = event->globalPos();
        m_moving = true;
    }
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_pos = m_posG = QPoint();
    m_moving = false;
}

void ImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_moving) {
        QPoint dp = event->globalPos() - m_posG;
        setTransformOrigin(m_o_img, m_o_dev + dp);
        m_pos = event->pos();
        m_posG = event->globalPos();
    }

    QWidget::mouseMoveEvent(event);
}

void ImageWidget::setFullScreen(QSize fullSize) {
    this->setFixedSize(fullSize);
    m_imageOriginSize = m_image.size();
    m_image = m_image.scaled(fullSize, Qt::IgnoreAspectRatio,
                             Qt::SmoothTransformation);
    setImage(m_image);
}

void ImageWidget::resetImageSize() {
    //TODO: the window's size need to read from QWidget::window()
    //but the result may get the wrong size,
    //because the size change from fullScreen is delay!

    QSize origiWSize =  qApp->desktop()->size();
    int origiw = origiWSize.width() * 0.8 < 700 ? 700 : origiWSize.width() * 0.8;
    int origih = origiWSize.height() * 0.8 < 500 ? 500 : origiWSize.height() * 0.8;

    this->setFixedSize(origiw, origih);
    m_image = m_image.scaled(m_imageOriginSize, Qt::IgnoreAspectRatio,
                             Qt::SmoothTransformation);
    setImage(m_image);
    resetTransform();
}

void ImageWidget::wheelEvent(QWheelEvent *event)
{
    m_scaling = true;
    killTimer(m_tid);
    m_tid = startTimer(500);
    setTransformOrigin(mapToImage(event->pos()), event->pos());
    qreal deg = event->angleDelta().y()/8;
    QPoint dp = event->pixelDelta();
    qreal zoom = m_scale;
    if (dp.isNull())
        zoom += deg*3.14/180.0;
    else
        zoom += dp.y()/100.0;

    qDebug() << "zoom:" << zoom;

    zoom = qBound(qreal(0.5), zoom, qreal(10));
    setScaleValue(zoom);
}

void ImageWidget::updateTransform()
{
    if (m_image.isNull())
        return;
    const QTransform old = m_mat;
    //Q_EMIT scaleValueChanged(value); //TODO: emit only when scaled image is rendered
    const QSize s = m_image.size()*m_scale;//.scaled(rect().size(), Qt::KeepAspectRatio);
    m_scale = qreal(s.width())/qreal(m_image.size().width());
    //qDebug() << s << m_image.size() << m_scale;
    m_mat.reset();
    QPoint img_o = QTransform().rotate(m_rot).scale(m_scale*m_flipX, m_scale*m_flipY).map(m_o_img);
    m_mat.translate(m_o_dev.x() - (qreal)img_o.x(), m_o_dev.y() - (qreal)img_o.y());
    m_mat.rotate(m_rot);
    m_mat.scale(m_scale*m_flipX, m_scale*m_flipY);
    //qDebug() << m_o_dev << m_mat.inverted().map(m_o_dev);
    //qDebug() << m_mat;
    update();
    if (m_mat != old)
        Q_EMIT transformChanged(m_mat);
}
