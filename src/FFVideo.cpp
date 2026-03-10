#include "FFVideo.h"

//#include <windows.h>

#include <QGroupBox>
#include <QVBoxLayout>
#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QThread>
#include <QFileDialog>
#include <QTextStream>

#include <iostream>
#include <sstream>   // 用于 std::ostringstream
#include <iomanip>   // 用于 std::put_time
#include <chrono>    // 用于处理系统时间
#include <ctime>     // 用于转换时间类型
#include <cstdlib>  // 用于 getenv

FFVideo::FFVideo(QWidget *parent)
    : FramelessWidget(parent),
    m_ChangeCamera(false)
{
    //QWidget* pWidget = new QWidget(this);

    QString resource_path = RESOURCE_DIR;
    QFile file(resource_path + "FF-Video.css");
    if (!file.open(QIODevice::OpenModeFlag::ReadOnly))
    {
        qWarning("css文件加载失败");
    }
    this->setStyleSheet(file.readAll());
    this->setMinimumSize(1920, 1080);
    this->setContentsMargins(0,0,0,0);

     //设置窗口背景色（区分内容区）
    QWidget* layoutContainer = new QWidget();
    layoutContainer->setStyleSheet("background-color: skyblue; margin:0px; padding:0px;");

    {
        // 标题标签布局区域
        QHBoxLayout* WidgetLayout = new QHBoxLayout(layoutContainer);
        WidgetLayout->setContentsMargins(0, 0, 0, 0); //去除边距
        WidgetLayout->setSpacing(0); // 设置间距为0

        QHBoxLayout* TitleLayout = new QHBoxLayout();
        TitleLayout->setContentsMargins(5, 5, 5, 5); //去除边距
        TitleLayout->setSpacing(0); // 设置间距为0
        QLabel* titleLabel = new QLabel("VideoCatcher", layoutContainer); // 注意父对象改为容器
        titleLabel->setObjectName("TitleLabel");
        titleLabel->setAlignment(Qt::AlignLeft);
        // 设置Icon
        QLabel* IconLabel = new QLabel();
        IconLabel->setPixmap(QIcon(resource_path + "XXX.ico").pixmap(35, 35));
        IconLabel->setFixedSize(40, 40);

        TitleLayout->addWidget(IconLabel);
        TitleLayout->addWidget(titleLabel);
        TitleLayout->addStretch();

        // 关闭，最大最小化布局区域
        QHBoxLayout* ButtonLayout = new QHBoxLayout();
        ButtonLayout->setContentsMargins(0, 0, 0, 0); //去除边距
        ButtonLayout->setSpacing(0); // 设置间距为0

        QPushButton* CloseButton = new QPushButton();
        CloseButton->setIcon(QPixmap(resource_path + "close.png"));
        CloseButton->setFixedSize(50, 50);
        CloseButton->setObjectName("TitleQPushButton");
        connect(CloseButton, &QPushButton::clicked, this, [this]() {
            if (QWidget* window = this->window()) {
                window->close();
            }
            });

        QPushButton* MaxButton = new QPushButton();
        MaxButton->setIcon(QPixmap(resource_path + "maximun.png"));
        MaxButton->setFixedSize(50, 50);
        MaxButton->setObjectName("TitleQPushButton");
        connect(MaxButton, &QPushButton::clicked, this, [this]() {
            if (QWidget* window = this->window()) {
                window->isMaximized() ? window->showNormal() : window->showMaximized();
            }
            });

        QPushButton* MinButton = new QPushButton();
        MinButton->setIcon(QPixmap(resource_path + "minimun.png"));
        MinButton->setFixedSize(50, 50);
        MinButton->setObjectName("TitleQPushButton");
        connect(MinButton, &QPushButton::clicked, this, [this]() {
            if (QWidget* window = this->window()) {
                window->showMinimized();
            }
            });

        ButtonLayout->addWidget(MinButton);
        ButtonLayout->addWidget(MaxButton);
        ButtonLayout->addWidget(CloseButton);
        ButtonLayout->setAlignment(Qt::AlignRight);

        WidgetLayout->addLayout(TitleLayout);
        WidgetLayout->addLayout(ButtonLayout);
    }

    QGroupBox* CameraGroup = new QGroupBox(tr("摄像头"));
    CameraGroup->setMaximumHeight(400);
    CameraGroup->setMaximumWidth(230);
    CameraGroup->setObjectName("ParameterQGroupBox");

    {
        QPushButton* InitCamera = CreateButton("初始化", this, &FFVideo::OpenCamera);

        QPushButton* AddFocalLength = CreateButton("截图", this, &FFVideo::onScreenshotButtonClicked);

        QPushButton* AddMultiple = CreateButton("开始录频", [this]() {StartRecording(); });

        QPushButton* MinusMultiple = CreateButton("结束录频", [this]() {EndRecording(); });

        //文件保存路径
        QPushButton* jpg = CreateButton("图片路径", [this]() {Set_m_imagePath(); });

        QPushButton* video = CreateButton("视频路径", [this]() {Set_m_videoPath(); });

        QPushButton* WorkOrder = CreateButton("工单导入", this, &FFVideo::OnSelectWorkOrder);

        QVBoxLayout* pVLayoutDir = new QVBoxLayout(CameraGroup);
        pVLayoutDir->setContentsMargins(3, 10, 3, 10); // 去除边距
        pVLayoutDir->setSpacing(0); // 设置间距为0  

        QHBoxLayout* pHLayoutDir1 = new QHBoxLayout();
        pHLayoutDir1->setContentsMargins(0, 5, 0, 5); // 去除边距
        pHLayoutDir1->setSpacing(0); // 设置间距为0  

        QHBoxLayout* pHLayoutDir2 = new QHBoxLayout();
        pHLayoutDir2->setContentsMargins(0, 5, 0, 5); // 去除边距
        pHLayoutDir2->setSpacing(0); // 设置间距为0  

        QHBoxLayout* pHLayoutDir3 = new QHBoxLayout();
        pHLayoutDir3->setContentsMargins(0, 5, 0, 5); // 去除边距
        pHLayoutDir3->setSpacing(0); // 设置间距为0  

        QHBoxLayout* pHLayoutDir4 = new QHBoxLayout();
        pHLayoutDir4->setContentsMargins(0, 5, 0, 5); // 去除边距
        pHLayoutDir4->setSpacing(0); // 设置间距为0  

        pHLayoutDir1->addWidget(InitCamera);
        pHLayoutDir1->addWidget(AddFocalLength);

        pHLayoutDir2->addWidget(AddMultiple, 1);
        pHLayoutDir2->addWidget(MinusMultiple, 1);

        pHLayoutDir3->addWidget(jpg, 1);
        pHLayoutDir3->addWidget(video, 1);

        pHLayoutDir4->addWidget(WorkOrder, 1);

        pVLayoutDir->addLayout(pHLayoutDir1);
        pVLayoutDir->addLayout(pHLayoutDir2);
        pVLayoutDir->addLayout(pHLayoutDir3);
        pVLayoutDir->addLayout(pHLayoutDir4);
    }

    QGroupBox* TextGroup = new QGroupBox(tr("视频水印"));
    TextGroup->setMaximumHeight(400);
    TextGroup->setMaximumWidth(230);
    TextGroup->setObjectName("ParameterQGroupBox");

    {
        QLabel* pDataLabel = new QLabel("视频水印文本");
        m_Multitext = new QPlainTextEdit();
        m_Multitext->setObjectName("YTQPlainTextEditStyle");
        m_Multitext->setMinimumSize(150, 200);

        QPushButton* OKBtn = CreateButton("应用", this, &FFVideo::OnProcessText);

        QVBoxLayout* pVlayout = new QVBoxLayout(TextGroup);
        QHBoxLayout* pHDataLayout1 = new QHBoxLayout();
        QHBoxLayout* pHDataLayout2 = new QHBoxLayout();

        pHDataLayout1->addWidget(m_Multitext, 3);

        pHDataLayout2->addWidget(OKBtn, 1);

        pVlayout->addLayout(pHDataLayout1);
        pVlayout->addLayout(pHDataLayout2);
    }

    QVBoxLayout* CameraLayout = new QVBoxLayout();
    CameraLayout->setContentsMargins(0, 0, 0, 0); // 去除边距
    CameraLayout->setSpacing(0); // 设置间距为0  
    CameraLayout->addWidget(CameraGroup);
    CameraLayout->addWidget(TextGroup);
    CameraLayout->addStretch();

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(5, 5, 5, 5); // 去除边距
    m_label = new QLabel(this);
    m_label->setMinimumSize(1560, 960);
    m_label->setAlignment(Qt::AlignCenter);  // 内容居中
    m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // 允许缩放
    layout->addWidget(m_label);

    QHBoxLayout* MainLayout = new QHBoxLayout();
    MainLayout->setContentsMargins(5, 5, 5, 5); // 去除边距
    MainLayout->setSpacing(0); // 设置间距为0
    MainLayout->addLayout(CameraLayout);
    MainLayout->addLayout(layout);
    //MainLayout->addStretch();

    QVBoxLayout* WindowLayout = new QVBoxLayout();
    WindowLayout->setContentsMargins(0, 0, 0, 0); // 设置为0去除与窗口边界的距离
    WindowLayout->setSpacing(0); // 设置间距为0
    WindowLayout->addWidget(layoutContainer);
    WindowLayout->addLayout(MainLayout);

    this->setLayout(WindowLayout);
    
    //this->setCentralWidget(pWidget);

    // 1. 获取应用程序可执行文件所在的目录
    QString exeDir = QCoreApplication::applicationDirPath();

    // 2. 构建视频与图片文件夹的完整路径
    QString VideoFolderName = "VideoPath";
    QString PhotoFolderName = "PhotoPath";
    QString WorkOrderFolderName = "WorkOrder";

    QString VideoFolderPath = exeDir + "/" + VideoFolderName + "/";
    QString PhotoFolderPath = exeDir + "/" + PhotoFolderName + "/";
    QString WorkOrderFolderPath = exeDir + "/" + WorkOrderFolderName;

    // 3. 创建QDir对象并检查、创建文件夹
    QDir dir;
    if (!dir.exists(VideoFolderPath)|| !dir.exists(PhotoFolderPath) || !dir.exists(WorkOrderFolderPath)) {
        // 使用mkpath，它可以递归创建所需的所有父目录
        if (!dir.mkpath(VideoFolderPath)|| !dir.mkpath(PhotoFolderPath) || !dir.mkpath(WorkOrderFolderPath)) {
            QMessageBox::information(nullptr, "Error", "视频或图片文件夹创建失败！");
        }
    }
    m_VideoPath = VideoFolderPath.toStdString();
    m_PhotoPath = PhotoFolderPath.toStdString();
    m_WorkOrderPath = WorkOrderFolderPath.toStdString();

    //支持中文水印的freeType对象设置
    m_freeType = cv::freetype::createFreeType2();
    //m_freeType->loadFontData("C:/Windows/Fonts/simsun.ttc", 0); // 加载宋体常规字体
    //m_freeType->loadFontData("C:/Windows/Fonts/msyh.ttc", 0); // 加载微软雅黑常规字体
    resource_path+="wqy-microhei.ttc";
    m_freeType->loadFontData(resource_path.toStdString(), 0);

    // m_RobotTimer = new QTimer(this);
    // m_RobotTimer->setInterval(1000);
    // connect(m_RobotTimer,&QTimer::timeout,this,&FFVideo::SetStringData);

    m_CompressWorker = new VideoCompressor(this);
    m_FFVideoWriter = std::make_shared<FFmpegVideoWriter>();
}

FFVideo::~FFVideo()
{
    if (m_isRecording) {
        EndRecording();
    }
    m_isRunning = false;
    if (m_capture.isOpened()) {
        m_capture.release();
    }
    if (m_RobotTimer) {
        m_RobotTimer->stop();
        disconnect(m_RobotTimer, nullptr, this, nullptr);
    }
}

void FFVideo::SetWaterMark(std::vector<std::string>& WaterMark)
{

}

bool FFVideo::SetRecording(bool isRecord)
{
    return false;
}

void FFVideo::Set_m_imagePath(const std::string& ImPath)
{
    QString m_imagePath = QFileDialog::getExistingDirectory(this, "选择文件夹", QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    m_PhotoPath = m_imagePath.toStdString();
}

void FFVideo::Set_m_videoPath(const std::string& ViPath)
{
    QString m_VideoPath = QFileDialog::getExistingDirectory(this, "选择文件夹", QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    m_PhotoPath = m_VideoPath.toStdString();
}

void FFVideo::OpenCamera()
{
    if (m_CompressWorker->isRunning()) {
        QMessageBox::information(this, "Error", "视频仍在解码中，请稍后！");
        return;
    }

    m_isRunning = false;
    if (m_capture.isOpened()) {
        m_capture.release();
    }

    m_ChangeCamera = !m_ChangeCamera;
    m_capture.open(m_ChangeCamera, cv::CAP_DSHOW);//CAP_DSHOW
    if (!m_capture.isOpened()) {
        m_label->setText("无法打开相机");
        return;
    }

    //m_capture.set(cv::CAP_PROP_AUTO_EXPOSURE, 0.25);
    //m_capture.set(cv::CAP_PROP_EXPOSURE, -3);
    m_capture.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    m_capture.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    m_capture.set(cv::CAP_PROP_FPS, 60);
    //m_capture.set(cv::CAP_PROP_BUFFERSIZE, 1);
    //m_capture.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('a', 'v', 'c', '1'));
    m_capture.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));

    m_RobotTimer->start();
    m_isRunning = true;
    cv::Mat frame;
    while (m_isRunning) {
        //auto start = std::chrono::high_resolution_clock::now();

        m_capture >> frame;
        if (frame.empty()) {//||*frame.data==NULL
            m_label->setText("获取帧失败");
            break;
        }

        // 发送帧信号
        m_freeType->putText(frame, "时间: " + Rand_str(), cv::Point(5, 45), 40, cv::Scalar(0, 0, 255), -1, 8, true);

        //m_freeType->putText(frame, m_MotorAngle, cv::Point(550, 45), 40, cv::Scalar(0, 0, 255), -1, 8, true);
        //m_freeType->putText(frame, m_RobotJoint, cv::Point(5, 85), 40, cv::Scalar(0, 0, 255), -1, 8, true);

        //m_freeType->putText(frame, m_StringData, cv::Point(5, 85), 40, cv::Scalar(0, 0, 255), -1, 8, true);

        int height = 2;
        for (auto& it : m_WaterMark) {
            m_freeType->putText(frame, it, cv::Point(5, 40 * ++height + 5), 40, cv::Scalar(0, 0, 255), -1, 8, true);
        }

        //if (m_isRecording && m_videoWriter.isOpened()) {
        //    m_videoWriter.write(frame);
        //}
        // 写入帧
        if (m_isRecording && m_FFVideoWriter->isopen()) {
            if (!m_FFVideoWriter->writeFrame(frame)) {
                QMessageBox::information(this, "Error", "写入帧失败！");
                break;
            }
        }

        QImage image = cvMatToQImage(frame);

        m_label->clear(); // 清空label，这会释放旧的QPixmap资源

        m_label->setPixmap(QPixmap::fromImage(image).scaled(
            m_label->size(), Qt::KeepAspectRatio, Qt::FastTransformation));

        //auto end = std::chrono::high_resolution_clock::now();

        //auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        //if (duration.count() < 16) {
        //    QThread::msleep(16 - duration.count());
        //}

        QThread::msleep(2);

        QCoreApplication::processEvents();

    }

    m_capture.release();
    m_RobotTimer->stop();
}

bool FFVideo::StartRecording()
{
    if (m_CompressWorker->isRunning()) {
        QMessageBox::information(this, "Error", "视频仍在解码中，请稍后！");
        return false;
    }

    m_CompressWorker->quit();

    if (m_isRecording) {
        QMessageBox::information(this, "Error", "已经在录制中！");
        return false;
    }

    if (!m_capture.isOpened()) {
        QMessageBox::information(this, "Error", "摄像头未打开！");
        return false;
    }

    // 尝试读取一帧确保摄像头工作正常
    cv::Mat testFrame;
    if (!m_capture.read(testFrame) || testFrame.empty()) {
        QMessageBox::information(this, "Error", "无法从摄像头读取帧！");
        return false;
    }

    // 设置视频编码器和帧率
    // 获取实际帧率
    double m_actualFps = m_capture.get(cv::CAP_PROP_FPS);
    if (m_actualFps <= 0) {
        // 设置默认值
        m_actualFps = 60;
    }

    cv::Size frameSize(testFrame.cols, testFrame.rows); // 使用实际帧尺寸

    int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1'); // H.264编码
    //fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    //int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G'); // H.264编码

    if (m_VideoPath.empty()) {
        QMessageBox::information(this, "Error", "路径为空！");
        return false;
    }

    m_VideoName = m_VideoPath + "Video_" + Rand_str() + ".mp4";

    int frame_width = 1920;
    int frame_height = 1080;
    int fps = 60;
    int bitrate = 25000000; // 25000 kbps 码率，可根据需要调整

    // 创建FFmpeg视频写入器
    if (!m_FFVideoWriter->open(m_VideoName, frame_width, frame_height, fps, bitrate, "h264_nvenc")) {
        if (!m_FFVideoWriter->open(m_VideoName, frame_width, frame_height, fps, bitrate, "h264_amf")) {
            if(!m_FFVideoWriter->open(m_VideoName, frame_width, frame_height, fps, bitrate, "libx264")){
                QMessageBox::information(this, "Error", "录制失败！");
                return false;
            }
        }
    }

    //// 打开视频写入器并检查结果
    //if (!m_videoWriter.open(m_VideoName, fourcc, m_actualFps, frameSize)) {
    //    // 尝试后备编码器
    //    fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    //    if (!m_videoWriter.open(m_VideoName, fourcc, m_actualFps, frameSize)) {
    //        QMessageBox::information(this, "Error", "无法创建视频文件！");
    //        return false;
    //    }
    //}

    m_isRecording = true;
    QMessageBox::information(this, "Success", "录制开始！");

    //视频保护
    if (testFrame.empty()) {
        EndRecording();
        QMessageBox::information(this, "Error", "画面丢失！");
        return false;
    }
}

bool FFVideo::EndRecording()
{
    if (m_CompressWorker->isRunning()) {
        QMessageBox::information(this, "Error", "视频仍在解码中，请稍后！");
        return false;
    }

    if (!m_isRecording) {
        //QMessageBox::information(this, "Error", "没有视频在录制！");
        return false;
    }
    //m_videoWriter.release();
    m_FFVideoWriter->close();
    m_isRecording = false;
    //m_CompressWorker->SetVideoName(m_VideoName);
    //m_CompressWorker->start();
    QMessageBox::information(this, "Success", "录制结束！");
    return true;
}

void FFVideo::onScreenshotButtonClicked()
{
    cv::Mat testFrame;
    if ((m_capture.read(testFrame) || !testFrame.empty()) && !m_PhotoPath.empty()) {
        std::string jpgname = m_PhotoPath + "Photo_" + Rand_str() + ".jpg";
        if (cv::imwrite(jpgname, testFrame)) {
            QMessageBox::information(this, "Success", "截图成功！");
        }
    }
}

void FFVideo::SetRobotParaMeter()
{

}

void FFVideo::SetMotorAngle()
{

}

void FFVideo::SetStringData()
{
    // // 1. 打开已存在的文件映射对象
    // HANDLE hMapFile = OpenFileMappingA(
    //     FILE_MAP_ALL_ACCESS,    // 可读写访问
    //     FALSE,                  // 不继承句柄
    //     "Local\\MySharedMemory"); // 名称必须与写入进程完全一致

    // if (hMapFile == NULL) {
    //     std::cerr << "Could not open file mapping object: " << GetLastError() << std::endl;
    //     return;
    // }

    // // 2. 映射到进程地址空间
    // LPVOID pBuf = MapViewOfFile(
    //     hMapFile,
    //     FILE_MAP_ALL_ACCESS,
    //     0,
    //     0,
    //     4096);

    // if (pBuf == NULL) {
    //     std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
    //     CloseHandle(hMapFile);
    //     return;
    // }

    // // 3. 从共享内存读取数据
    // m_StringData = std::string(reinterpret_cast<char*>(pBuf));

    // // 4. 清理资源（注意：读取进程通常不负责删除共享内存）
    // UnmapViewOfFile(pBuf);
    // CloseHandle(hMapFile);
}

void FFVideo::OnProcessText()
{
    m_WaterMark.clear();

    // 获取多行文本
    QString text = m_Multitext->toPlainText();

    // 按行分割并存储
    QStringList qLines = text.split("\n", Qt::SkipEmptyParts);

    for (const QString& qLine : qLines) {
        m_WaterMark.push_back(qLine.toStdString());
    }
}

void FFVideo::OnSelectWorkOrder()
{
    // 创建文件对话框
    QFileDialog dlg(this);
    dlg.setWindowTitle(tr("打开CSV文件"));
    dlg.setDirectory(m_WorkOrderPath.c_str());
    dlg.setNameFilter(tr("CSV Files (*.csv);;All Files(*.*)"));
    dlg.setFileMode(QFileDialog::ExistingFile);

    // 检查用户是否选择了文件（未选择时路径为空）
    if (dlg.exec() == QDialog::Accepted)
    {
        QStringList files = dlg.selectedFiles();
        if (!files.isEmpty())
        {
            m_WaterMark.clear();

            QString WorkOrderPath = QDir::toNativeSeparators(files.first());

            std::vector<std::vector<std::string>> data;
            read_csv(WorkOrderPath, data);

            for (auto& row : data) {
                if (row.size() < 2 || row[0].empty()) {
                    continue;
                }

                std::string Tem = row[0] + ": " + row[1];
                m_WaterMark.emplace_back(Tem);
            }
        }
        else {
            
        }
    }

}

std::string FFVideo::Rand_str()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y.%m.%d-%H.%M.%S");
    return ss.str();
}

void FFVideo::read_csv(const QString& path, std::vector<std::vector<std::string>>& data)
{
    // 使用Qt的QFile打开文件，支持中文路径
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法打开 CSV 文件进行读取！");
        return;
    }

    QTextStream in(&file);
    in.setCodec("GBK");
    QString line;
    while (in.readLineInto(&line)) {
        std::vector<std::string> row;

        // 预处理：替换",\""为临时标记，避免被逗号分割
        QString processedLine = line;
        processedLine.replace(",\"", "@@");

        // 分割逗号，保留空字段
        QStringList fields = processedLine.split(',', QString::KeepEmptyParts);

        for (const QString& field : fields) {
            // 还原临时标记
            QString restoredField = field;
            restoredField.replace("@@", ",\"");
            row.push_back(restoredField.toStdString());
        }
        data.push_back(row);
    }

}

void FFVideo::closeEvent(QCloseEvent* event)
{
    if (m_isRecording) {
        EndRecording();
    }
    m_isRunning = false;
    if (m_capture.isOpened()) {
        m_capture.release();
    }
    if (m_RobotTimer) {
        m_RobotTimer->stop();
        disconnect(m_RobotTimer, nullptr, this, nullptr);
    }
}

QImage FFVideo::cvMatToQImage(const cv::Mat& mat)
{
    switch (mat.type()) {
    case CV_8UC4: {
        return QImage(mat.data, mat.cols, mat.rows,
            static_cast<int>(mat.step),
            QImage::Format_ARGB32);
    }
    case CV_8UC3: {
        QImage image(mat.data, mat.cols, mat.rows,
            static_cast<int>(mat.step),
            QImage::Format_RGB888);
        return image.rgbSwapped(); // BGR -> RGB
    }
    case CV_8UC1: {
        return QImage(mat.data, mat.cols, mat.rows,
            static_cast<int>(mat.step),
            QImage::Format_Grayscale8);
    }
    default:
        return QImage();
    }
}

