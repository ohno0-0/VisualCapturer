#include "FFVideo.h"
#include <QtWidgets/QApplication>
#include <QTextCodec>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    FFVideo window;
    window.show();
    return app.exec();
}
