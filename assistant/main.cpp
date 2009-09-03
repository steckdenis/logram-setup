#include <QApplication>
#include <QLocale>
#include <QTextCodec>

#include "win.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QString para;

    if (app.arguments().count() > 1)
    {
        para = app.arguments().at(1);
    }
    else
    {
        para = "/usr/share/assistant/" + QLocale::system().name().section('_', 0, 0) + "/tree.xml";
    }

    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    
    Win *win = new Win(para);

    return app.exec();
}