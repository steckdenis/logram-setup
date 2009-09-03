#include "win.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QSplitter>
#include <QPushButton>
#include <QFrame>
#include <QApplication>
#include <QLabel>
#include <QFont>
#include <QSize>
#include <QTreeWidgetItem>
#include <QSizePolicy>
#include <QDesktopWidget>
#include <QPixmap>
#include <QIcon>

#include <QFile>

Win::Win(const QString &path) : QWidget(0)
{
    resize(800, 600);
    move(QApplication::desktop()->width() / 2 - 400, QApplication::desktop()->height() / 2 - 300);
    setWindowIcon(QIcon(":/icon.png"));
    
    // Créer le layout principal
    QVBoxLayout *mLayout = new QVBoxLayout(this);
    this->setLayout(mLayout);
    
    // Créer le QSplitter
    QSplitter *splitter = new QSplitter(this);
    mLayout->addWidget(splitter);

    // Créer l'arbre
    tree = new QTreeWidget(splitter);
    tree->setHeaderHidden(true);
    tree->setMinimumSize(QSize(120, 120));
    splitter->addWidget(tree);

    // Créer le conteneur de la page
    pageWidget = new QWidget(splitter);
    splitter->addWidget(pageWidget);

    layout = new QVBoxLayout(pageWidget);
    pageWidget->setLayout(layout);
    layout->addStretch();
    layout->setSpacing(12);

    // Créer la séparation avec les boutons
    QFrame *sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    mLayout->addWidget(sep);

    // Créer le layout des boutons
    QHBoxLayout *btnLayout = new QHBoxLayout();

    mLayout->addLayout(btnLayout);

    // Créer les boutons
    btnQuit = new QPushButton(tr("&Quitter"));
    btnPrev = new QPushButton(tr("« &Précédant"));
    btnNext = new QPushButton(tr("&Suivant »"));
    btnNextPage = new QPushButton(tr("P&age suivante »"));
    btnUp = new QPushButton(tr("Remonter"));

    btnLayout->addWidget(btnQuit);
    btnLayout->addStretch();
    btnLayout->addWidget(btnPrev);
    btnLayout->addWidget(btnUp);
    btnLayout->addWidget(btnNext);
    btnLayout->addStretch();
    btnLayout->addWidget(btnNextPage);

    // Afficher les contrôles
    splitter->show();
    tree->show();
    pageWidget->show();
    sep->show();
    btnQuit->show();
    btnPrev->show();
    btnNext->show();
    btnNextPage->show();
    btnUp->show();

    // Relier les signaux
    connect(btnQuit, SIGNAL(clicked()), qApp, SLOT(quit()));
    connect(btnNext, SIGNAL(clicked()), this, SLOT(nextClicked()));
    connect(btnPrev, SIGNAL(clicked()), this, SLOT(prevClicked()));
    connect(btnUp, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(btnNextPage, SIGNAL(clicked()), this, SLOT(nextPageClicked()));
    connect(tree, SIGNAL(itemActivated(QTreeWidgetItem *, int)), this, SLOT(itemClicked(QTreeWidgetItem *, int)));

    // Charger le document
    QFile fl(path);
    doc.setContent(&fl);

    QDir::setCurrent(path.section('/', 0, -2));

    buildTree(doc.documentElement());
    displayPage(doc.documentElement());

    // Redimensionnement
    show();
    splitter->setSizes(QList<int>() << 220 << 580);
}

void Win::prevClicked()
{
    displayPage(currentPage.previousSiblingElement());
}

void Win::nextClicked()
{
    displayPage(currentPage.nextSiblingElement());
}

void Win::nextPageClicked()
{
    if (currentPg != maxPg)
    {
        currentPg++;
        displayPage(currentPage, currentPg);
    }
}

void Win::upClicked()
{
    displayPage(currentPage.parentNode().toElement());
}

void Win::itemClicked(QTreeWidgetItem *item, int column)
{
    if (item == 0) return;
    
    QString page = item->data(0, Qt::UserRole + 0).toString();

    currentPage = nodeByPath(page);
    
    displayPage(page);
}

void Win::buttonClicked()
{
    QPushButton *btn = static_cast<QPushButton *>(sender());
    if (btn == 0) return;

    // Récupérer la page du bouton
    QString page = btn->property("page").toString();

    currentPage = nodeByPath(page);
    
    displayPage(page);
}

void Win::displayPage(const QDomElement &page, int pg)
{
    currentPage = page;
    
    displayPage(page.attribute("page"), pg);
}

void Win::displayPage(const QString &path, int pg)
{
    // Supprimer tous les widgets de la page précédante
    while (widgets.count() != 0)
    {
        QWidget *wg = widgets.takeAt(0);

        delete wg;
    }

    // Activer la page dans l'arbre
    tree->setCurrentItem(itemByPath(path));

    // Mettre à jour les boutons
    btnUp->setEnabled(!currentPage.parentNode().isDocument());
    btnPrev->setEnabled(!currentPage.previousSiblingElement().isNull());
    btnNext->setEnabled(!currentPage.nextSiblingElement().isNull());
    
    // Explorer le document de la page
    QDomDocument pdoc;
    QFile fl(path);
    pdoc.setContent(&fl);

    QDomElement el = pdoc.documentElement().firstChildElement();

    // Gérer les pages et le titre
    QString tt = pdoc.documentElement().attribute("title");
    currentPg = pg;
    setWindowTitle(tt + tr(" - Logram Assistant"));

    // Créer le titre de la page
    QLabel *title = new QLabel(tt, this);
    QFont fnt = title->font();
    fnt.setItalic(true);
    fnt.setPixelSize(22);
    title->setFont(fnt);
    title->setAlignment(Qt::AlignCenter);
    title->show();
    layout->insertWidget(0, title);
    widgets.append(title);

    bool hasPage = false;
    int pageNumber = 1;
    
    while (!el.isNull())
    {
        if (pageNumber == pg)
        {
            if (el.tagName() == "text")
            {
                // Élément de texte, créer un QLabel
                QLabel *txt = new QLabel(this);
                txt->setTextFormat(Qt::RichText);
                txt->setText(el.text().trimmed().replace("[", "<").replace("]", ">"));
                txt->setWordWrap(true);
                txt->setAlignment(Qt::AlignTop | Qt::AlignLeft);
                txt->show();
                layout->insertWidget(layout->count()-1, txt);

                widgets.append(txt);
            }
            else if (el.tagName() == "button")
            {
                // Un bouton, lien vers une page
                QPushButton *btn = new QPushButton(this);
                btn->setText(el.attribute("title"));
                btn->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
                btn->show();
                layout->insertWidget(layout->count()-1, btn, 0, Qt::AlignCenter);
                widgets.append(btn);

                // Utiliser une propriété pour retrouver ce bouton
                btn->setProperty("page", el.attribute("to"));

                connect(btn, SIGNAL(clicked()), this, SLOT(buttonClicked()));
            }
            else if (el.tagName() == "image")
            {
                QLabel *img = new QLabel(this);
                img->setAlignment(Qt::AlignCenter);
                img->setPixmap(QPixmap(el.attribute("path")));
                img->setToolTip(el.attribute("tooltip"));

                layout->insertWidget(layout->count()-1, img, 0, Qt::AlignCenter);
                widgets.append(img);
            }
        }
        if (el.tagName() == "page")
        {
            hasPage = true;
            pageNumber++;
        }

        el = el.nextSiblingElement();
    }

    // Activer la page suivante
    if (hasPage)
    {
        btnNextPage->setEnabled(hasPage);
        btnNextPage->setText(tr("(%0/%1) P&age suivante »").arg(pg).arg(pageNumber));
    }
    else
    {
        btnNextPage->setEnabled(hasPage);
        btnNextPage->setText(tr("P&age suivante »"));
    }
    maxPg = pageNumber;
}

QDomElement Win::nodeByPath(const QString &path)
{
    if (doc.documentElement().attribute("page") == path)
    {
        return doc.documentElement();
    }
    
    return nodeByPath(path, doc.documentElement());
}

QDomElement Win::nodeByPath(const QString &path, const QDomElement &parent)
{
    // Explorer les enfants
    QDomElement child = parent.firstChildElement();

    while (!child.isNull())
    {
        if (child.attribute("page") == path)
        {
            return child;
        }

        // Voir si cet enfant contient ce qu'on cherche
        QDomElement rs = nodeByPath(path, child);

        if (!rs.isNull())
        {
            return rs;
        }

        child = child.nextSiblingElement();
    }

    return QDomElement();
}

QTreeWidgetItem *Win::itemByPath(const QString &path, QTreeWidgetItem *parent)
{
    if (parent == 0) parent = tree->topLevelItem(0);

    // Explorer les enfants
    for (int i=0; i<parent->childCount(); i++)
    {
        QTreeWidgetItem *child = parent->child(i);

        if (child->data(0, Qt::UserRole + 0).toString() == path)
        {
            return child;
        }

        // Voir si cet élément n'a pas d'enfants
        QTreeWidgetItem *match = itemByPath(path, child);

        if (match != 0)
        {
            return match;
        }
    }

    return 0;
}

void Win::buildTree(const QDomElement &main)
{
    // Créer l'élément à ajouter
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(0, main.attribute("title"));

    tree->addTopLevelItem(item);

    // Ajouter une donnée utilisateur à cet élément pour savoir afficher sa page
    item->setData(0, Qt::UserRole + 0, main.attribute("page"));

    buildTree(main, item);
}

void Win::buildTree(const QDomElement &el, QTreeWidgetItem *parent)
{
    // Explorer les enfants de el, et les ajouter
    QDomElement child = el.firstChildElement();

    while (!child.isNull())
    {
        // Créer un élément
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, child.attribute("title"));

        parent->addChild(item);

        // Ajouter une donnée utilisateur à cet élément pour savoir afficher sa page
        item->setData(0, Qt::UserRole + 0, child.attribute("page"));

        // Explorer les enfants de cet élément
        buildTree(child, item);
        
        child = child.nextSiblingElement();
    }
}