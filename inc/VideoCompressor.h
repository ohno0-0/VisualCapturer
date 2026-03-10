#pragma once

#include <QThread>

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavutil/opt.h>
}

class VideoCompressor : public QThread
{
public:
	VideoCompressor(QObject* parent = nullptr);

	~VideoCompressor();

	void SetVideoName(std::string VideoName);

public:
	virtual void run()override;

private:
	std::string m_VideoName;

	int64_t last_pts = AV_NOPTS_VALUE; // 初始化为无效时间戳
};