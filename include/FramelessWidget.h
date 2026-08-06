#pragma once

#include <QWidget>

class FramelessWidget : public QWidget
{
	Q_OBJECT
public:
	explicit FramelessWidget(QWidget* parent = nullptr);
	~FramelessWidget();
	void setOldWindowState(Qt::WindowStates state); // 设置历史窗口状态
protected:
	// 重写 Qt 事件处理函数
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	bool eventFilter(QObject* obj, QEvent* event) override;
	void changeEvent(QEvent* event) override;
	// 自定义辅助函数
	void handleResize(QMouseEvent* event); // 处理窗口调整大小
	void updateCursorShape(const QPoint& globalPos); // 更新光标形状
private:
	Qt::WindowStates m_OldWindowState; // 最小化前的窗口状态（用于恢复）
	Qt::WindowStates m_WindowState;    // 当前窗口状态
	bool m_readyMove;                  // 是否准备拖拽移动
	QPoint m_currentPos;               // 窗口初始位置（拖拽时用）
	QPoint m_mouseStartPoint;          // 鼠标按下时的全局位置（拖拽时用）
	bool m_resizing;                   // 是否正在调整窗口大小
	int m_resizeEdge;                  // 当前调整的窗口边缘（左/右/上/下/角落）
	QPoint m_resizeStartPos;           // 调整大小开始时的鼠标全局位置
	QRect m_resizeStartGeometry;       // 调整大小开始时的窗口几何信息
};