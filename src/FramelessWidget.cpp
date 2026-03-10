#include "FramelessWidget.h"
#include <QMouseEvent>
#include <QCoreApplication>

const int RESIZE_MARGIN = 10;

FramelessWidget::FramelessWidget(QWidget* parent) : QWidget(parent)
{
    setWindowFlag(Qt::FramelessWindowHint);
    // 启用鼠标跟踪
    setMouseTracking(true);
    // 为所有子部件启用鼠标跟踪
    installEventFilter(this);
}
FramelessWidget::~FramelessWidget()
{
}
void FramelessWidget::setOldWindowState(Qt::WindowStates state)
{
    m_OldWindowState = state;
}
void FramelessWidget::mousePressEvent(QMouseEvent* event)
{
    if (this->windowState() == Qt::WindowFullScreen)
    {
        // 全屏状态下，不响应鼠标事件
        return;
    }
    if (event->button() == Qt::LeftButton)
    {
        if (m_resizeEdge != 0)
        {
            // 如果在边缘区域按下，开始调整大小
            m_resizing = true;
            m_resizeStartPos = event->globalPos();
            m_resizeStartGeometry = geometry();
        }
        else
        {
            // 鼠标在窗口内容区域按下了左键,准备开始移动
            m_readyMove = true;
            // 记录当前窗口和鼠标的位置
            m_currentPos = frameGeometry().topLeft();
            m_mouseStartPoint = event->globalPos();
        }
    }
}
void FramelessWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (this->windowState() == Qt::WindowFullScreen)
    {
        // 全屏状态下，不响应鼠标事件
        return;
    }
    if (m_resizing)
    {
        // 正在调整窗口大小
        handleResize(event);
    }
    else if (m_readyMove)
    {
        // 正在移动窗口
        QPoint moveDistance = event->globalPos() - m_mouseStartPoint;
        move(m_currentPos + moveDistance);
    }
    else
    {
        // 更新鼠标光标形状
        updateCursorShape(event->globalPos());
    }
}
void FramelessWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_resizing = false;
        m_readyMove = false;
        m_resizeEdge = 0;
        // 恢复默认光标
        setCursor(Qt::ArrowCursor);
        // 靠近顶部全屏
        if (event->globalPos().y() <= 10 && !m_resizing)
        {
            this->setWindowState(Qt::WindowFullScreen);
        }
    }
}
void FramelessWidget::updateCursorShape(const QPoint& globalPos)
{
    if (this->windowState() == Qt::WindowFullScreen || this->isMaximized())
    {
        setCursor(Qt::ArrowCursor);
        return;
    }
    // 将全局坐标转换为窗口内的局部坐标
    QPoint localPos = mapFromGlobal(globalPos);
    int x = localPos.x();
    int y = localPos.y();
    int width = this->width();
    int height = this->height();
    // 检测鼠标在哪个边缘区域
    bool left = x < RESIZE_MARGIN;
    bool right = x > width - RESIZE_MARGIN;
    bool top = y < RESIZE_MARGIN;
    bool bottom = y > height - RESIZE_MARGIN;
    if (left && top)
    {
        setCursor(Qt::SizeFDiagCursor);
        m_resizeEdge = Qt::TopEdge | Qt::LeftEdge;
    }
    else if (left && bottom)
    {
        setCursor(Qt::SizeBDiagCursor);
        m_resizeEdge = Qt::BottomEdge | Qt::LeftEdge;
    }
    else if (right && top)
    {
        setCursor(Qt::SizeBDiagCursor);
        m_resizeEdge = Qt::TopEdge | Qt::RightEdge;
    }
    else if (right && bottom)
    {
        setCursor(Qt::SizeFDiagCursor);
        m_resizeEdge = Qt::BottomEdge | Qt::RightEdge;
    }
    else if (left)
    {
        setCursor(Qt::SizeHorCursor);
        m_resizeEdge = Qt::LeftEdge;
    }
    else if (right)
    {
        setCursor(Qt::SizeHorCursor);
        m_resizeEdge = Qt::RightEdge;
    }
    else if (top)
    {
        setCursor(Qt::SizeVerCursor);
        m_resizeEdge = Qt::TopEdge;
    }
    else if (bottom)
    {
        setCursor(Qt::SizeVerCursor);
        m_resizeEdge = Qt::BottomEdge;
    }
    else
    {
        setCursor(Qt::ArrowCursor);
        m_resizeEdge = 0;
    }
}
void FramelessWidget::handleResize(QMouseEvent* event)
{
    QRect newGeometry = m_resizeStartGeometry;
    QPoint delta = event->globalPos() - m_resizeStartPos;
    if (m_resizeEdge & Qt::LeftEdge)
    {
        newGeometry.setLeft(m_resizeStartGeometry.left() + delta.x());
        if (newGeometry.width() < minimumWidth())
        {
            newGeometry.setLeft(m_resizeStartGeometry.right() - minimumWidth());
        }
    }
    if (m_resizeEdge & Qt::RightEdge)
    {
        newGeometry.setRight(m_resizeStartGeometry.right() + delta.x());
        if (newGeometry.width() < minimumWidth())
        {
            newGeometry.setRight(m_resizeStartGeometry.left() + minimumWidth());
        }
    }
    if (m_resizeEdge & Qt::TopEdge)
    {
        newGeometry.setTop(m_resizeStartGeometry.top() + delta.y());
        if (newGeometry.height() < minimumHeight())
        {
            newGeometry.setTop(m_resizeStartGeometry.bottom() - minimumHeight());
        }
    }
    if (m_resizeEdge & Qt::BottomEdge)
    {
        newGeometry.setBottom(m_resizeStartGeometry.bottom() + delta.y());
        if (newGeometry.height() < minimumHeight())
        {
            newGeometry.setBottom(m_resizeStartGeometry.top() + minimumHeight());
        }
    }
    setGeometry(newGeometry);
}
bool FramelessWidget::eventFilter(QObject* obj, QEvent* event)
{
    // 将鼠标移动事件传递给主窗口，用于更新光标形状
    if (event->type() == QEvent::MouseMove && !m_resizing && !m_readyMove)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        updateCursorShape(mouseEvent->globalPos());
    }
    return QWidget::eventFilter(obj, event);
}
void FramelessWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (this->windowState() == Qt::WindowFullScreen)
        {
            this->setWindowState(Qt::WindowNoState);
        }
        else
        {
            this->setWindowState(Qt::WindowFullScreen);
        }
    }
}
void FramelessWidget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        if (m_WindowState == Qt::WindowMinimized && this->windowState() != Qt::WindowFullScreen)
        {
            if (m_OldWindowState == Qt::WindowFullScreen)
            {
                this->setWindowState(Qt::WindowFullScreen);
            }
        }
        m_WindowState = this->windowState();
    }
    QWidget::changeEvent(event);
}