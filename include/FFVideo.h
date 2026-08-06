#pragma once
#pragma execution_character_set("utf-8")

#include <QtWidgets/QMainWindow>

#include <QPlainTextEdit>
#include <QLabel>
#include <QTimer>
#include <QPushButton>

// 重写过后的主窗口文件
#include "FramelessWidget.h"

#include <string>
#include <vector>

// opencv Header
#include <opencv2/opencv.hpp>
#include <opencv2/freetype.hpp>

#include "VideoCompressor.h"
#include "FFmpegVideoWriter.h"

class FFVideo : public FramelessWidget
{
    Q_OBJECT

public:
    FFVideo(QWidget *parent = nullptr);
    ~FFVideo();

	// 创建按钮QPushButton
	template<typename T>
	static QPushButton* CreateButton(
		const QString& text,
		T* receiver, void (T::* member)(),
		const int MinWidth = 30,
		const int MinHeight = 30,
		const int MaxWidth = 80,
		const int MaxHeight = 40
	) {
		QPushButton* button = new QPushButton(text);
		button->setMinimumWidth(MinWidth);
		button->setMaximumWidth(MaxWidth);
		button->setMinimumHeight(MinHeight);
		button->setMaximumHeight(MaxHeight);
		button->setObjectName("StyleQPushButton");
		button->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
		QObject::connect(button, &QPushButton::clicked, receiver, member);
		return button;
	}

	// 可以使用lambda表达式 [3/13/2025 DaiYueJuan]
	template<typename Func>
	static QPushButton* CreateButton(
		const QString& text,
		Func&& slotFunc,
		const int MinWidth = 30,
		const int MinHeight = 30,
		const int MaxWidth = 80,
		const int MaxHeight = 40
	) {
		QPushButton* button = new QPushButton(text);
		button->setMinimumWidth(MinWidth);
		button->setMaximumWidth(MaxWidth);
		button->setMinimumHeight(MinHeight);
		button->setMaximumHeight(MaxHeight);
		button->setObjectName("StyleQPushButton");
		button->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
		QObject::connect(button, &QPushButton::clicked, std::forward<Func>(slotFunc));
		return button;
	}

public:
	void SetWaterMark(std::vector<std::string>& WaterMark);

	bool SetRecording(bool isRecord);

	void Set_m_imagePath(const std::string& ImPath = "");

	void Set_m_videoPath(const std::string& ViPath = "");

	void OpenCamera();

	bool StartRecording();

	bool EndRecording();

	void onScreenshotButtonClicked();

	QImage cvMatToQImage(const cv::Mat& mat);

protected slots:
	void SetRobotParaMeter();

	void SetMotorAngle();

	void SetStringData();

	void OnProcessText();

	void OnSelectWorkOrder();

	std::string Rand_str();

	void read_csv(const QString& path, std::vector<std::vector<std::string>>& data);

	void closeEvent(QCloseEvent* event) override;

private:
	//----------界面控件----------//
	QLabel* m_label;
	QPlainTextEdit* m_Multitext;
	QTimer* m_MotorTimer;
	QTimer* m_RobotTimer;

	//----------Opencv对象成员----------//
	cv::VideoCapture m_capture;
	cv::Ptr<cv::freetype::FreeType2> m_freeType;
	cv::VideoWriter m_videoWriter;
	std::shared_ptr<FFmpegVideoWriter> m_FFVideoWriter;

	std::string m_VideoName;

	bool m_ChangeCamera;
	bool m_isRunning;
	bool m_isRecording;

	VideoCompressor* m_CompressWorker;

	// 水印内容
	std::vector<std::string> m_WaterMark;
	std::string m_RobotJoint;
	std::string m_MotorAngle;
	std::string m_StringData;

	//----------文件夹路径----------//
	std::string m_VideoPath;
	std::string m_PhotoPath;
	std::string m_WorkOrderPath;
};

