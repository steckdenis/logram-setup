#ifndef __WIN_H__
#define __WIN_H__

#include <QWidget>
#include <QList>

#include <QtXml>

class QVBoxLayout;
class QTreeWidget;
class QPushButton;
class QTreeWidgetItem;

class Win : public QWidget
{
    Q_OBJECT

    public:
        Win(const QString &path);

        void displayPage(const QString &path, int pg = 1);
        void displayPage(const QDomElement &page, int pg = 1);
        void buildTree(const QDomElement &main);
        void buildTree(const QDomElement &el, QTreeWidgetItem *parent);

        QDomElement nodeByPath(const QString &path);
        QDomElement nodeByPath(const QString &path, const QDomElement &parent);
        QTreeWidgetItem *itemByPath(const QString &path, QTreeWidgetItem *parent = 0);

    private slots:
        void prevClicked();
        void nextClicked();
        void nextPageClicked();
        void upClicked();

        void itemClicked(QTreeWidgetItem *item, int column);
        void buttonClicked();

    private:
        QList<QWidget *> widgets;
        QWidget *pageWidget;
        QVBoxLayout *layout;
        QTreeWidget *tree;

        QPushButton *btnQuit, *btnPrev, *btnNext, *btnNextPage, *btnUp;

        QDomDocument doc;
        QDomElement currentPage;
        int currentPg, maxPg;
};

#endif