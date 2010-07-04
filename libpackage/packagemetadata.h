/*
 * packagemetadata.h
 * This file is part of Logram
 *
 * Copyright (C) 2009, 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
 *
 * Logram is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Logram is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Logram; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

/**
 * @file packagemetadata.h
 * @brief Gestion des métadonnées des paquets
 */

#ifndef __PACKAGEMETADATA_H__
#define __PACKAGEMETADATA_H__

#include <QtXml/QDomDocument>

#include <QObject>
#include <QDateTime>
#include <QProcess>
#include <QStringList>

namespace Logram
{

class Package;
class PackageSystem;
class Templatable;
class Communication;

/**
 * @brief Entrée de changelog
 * 
 * Cette structure est utilisée par PackageMetaData pour fournir la liste
 * des modifications d'un paquet. Ainsi, vous n'avez pas à parser un fichier
 * XML.
 */
struct ChangeLogEntry
{
    /**
     * Type d'entrée
     */
    enum Type
    {
        LowPriority = 1,    /*!< @brief Faible priorité, typiquement un changement facilitant le travail des empaqueteurs mais ne changeant pas les binaires */
        Feature = 2,        /*!< @brief Nouvelle version du paquet, avec de nouvelles fonctionnalités */
        BugFix = 3,         /*!< @brief Correction d'un bug (upstream ou par Logram) */
        Security = 4        /*!< @brief Correction d'un problème de sécurité. Mise à jour urgente */
    };
    
    Type type;              /*!< @brief Type de l'entrée */
    QString version;        /*!< @brief Version de l'entrée */
    QString author;         /*!< @brief Auteur de cette révision */
    QString email;          /*!< @brief Email de l'auteur */
    QString distribution;   /*!< @brief Dépôt dans lequel la modification est allée */
    QDateTime date;         /*!< @brief Date de l'entrée */
    QString text;           /*!< @brief Description du changement, dans la langue de l'utilisateur (cf PackageMetaData::stringOfKey() ) */
};

/**
 * @brief Dépendant à la construction d'un paquet source
 */
struct SourceDepend
{
    int type;       /*!< @brief Type, cf PACKAGE_DEPEND* */
    QString string; /*!< @brief Texte de la dépendance (paquet>=version) */
};

/**
 * @brief Métadonnées d'un paquet
 * 
 * Cette classe utilitaire permet de facilement obtenir des informations sur
 * un paquet, ou de gérer un fichier XML de métadonnées.
 * 
 * Comme cette classe hérite de QDomDocument, il est possible pour
 * l'application d'effectuer toutes les opérations qu'elle veut sur le
 * document. C'est le côté extensible de cette classe.
 */
class PackageMetaData : public QObject, public QDomDocument
{
    Q_OBJECT
    
    public:
        /**
         * @brief Constructeur
         * @param ps PackageSystem utilisé
         */
        PackageMetaData(PackageSystem *ps);
        
        /**
         * @brief Constructeur
         * @param ps PackageSystem utilisé
         * @param parent QObject parent
         */
        PackageMetaData(PackageSystem *ps, QObject *parent);
        
        /**
         * @brief Destructeur
         */
        ~PackageMetaData();
        
        /**
         * @brief Erreur
         * @return True si une erreur s'est produite dans le constructeur, false sinon.
         */
        bool error() const;
        
        /**
         * @brief Initialise la classe avec un fichier
         * 
         * PackageMetaData peut être initialisé de plusieurs manières. Charger
         * un fichier en est une, assez facile si vous avez téléchargé les
         * métadonnées vous-même.
         * 
         * Cette fonction supporte la décompression d'un fichier XZ
         * (metadata.xml.xz), ainsi que la vérification du hash SHA1 du fichier
         * une fois décompressé.
         * 
         * @param fileName Nom du fichier XML à charger
         * @param sha1hash Hash SHA1 calculé par QCryptographicHash. QByteArray() pour ne pas vérifier le hash
         * @param decompress True si le fichier est compressé en XZ, false si non-compressé.
         */
        void loadFile(const QString &fileName, const QByteArray &sha1hash, bool decompress);
        
        /**
         * @brief Initialise la classe avec des données brutes
         * 
         * Fonction simple, équivalent de QDomDocument::setContent. Charge
         * les données XML brutes dans le document.
         * 
         * @param data Données XML
         */
        void loadData(const QByteArray &data);
        
        /**
         * @brief Initialise la classe avec un paquet
         * 
         * Cette fonction est utilisée par Package::metadata(). Elle télécharge
         * les métadonnées d'un paquet, si ce n'a pas déjà été fait, et passe
         * les bons paramètres à loadFile().
         * 
         * Si vous pensez avoir besoin de cette fonction, utilisez plutôt
         * Package::metadata().
         * 
         * @code
         * // Ceci
         * Package *pkg = foo();
         * PackageMetaData *md = new PackageMetaData(ps);
         * md->bindPackage(pkg);
         * 
         * if (md->error) error();
         * 
         * // Est équivalent à
         * Package *pkg = foo();
         * PackageMetaData *md = pkg->metadata();
         * 
         * if (!md || md->error()) error();
         * @endcode
         * 
         * @param pkg Package à utiliser
         */
        void bindPackage(Package *pkg);
        
        /**
         * @brief Définit le paquet
         * 
         * Cette fonction permet simplement d'associer un paquet au
         * PackageMetaData. Par exemple, si vous avez appelé loadData, et
         * que ces données viennent d'un paquet, cette fonction peut
         * servir.
         * 
         * Pour le moment, PackageMetaData ne fait rien avec ceci, mais
         * pourrait un jour s'en servir pour les communications éventuellement
         * lancées.
         * 
         * @param pkg le Package utilisé.
         */
        void setPackage(Package *pkg);
        
        /**
         * @brief Définit le moteur de templates de la classe
         * 
         * Pour le confort des empaqueteurs, un petit moteur de templates
         * est utilisé. Il ne fait que remplacer des clefs par des valeurs.
         * 
         * Cette fonction vous permet de donner un moteur de templates à
         * PackageMetaData. Vous avez ainsi pu le remplir avec certaines clefs,
         * adaptées avec l'utilisation que vous faites de cette classe.
         * 
         * @code
         * // Lancer les scripts pre* et post*
         * Templatable *tpl = new Templatable(0);
         * 
         * tpl->addKey("instroot", d->ps->installRoot());
         * tpl->addKey("varroot", d->ps->varRoot());
         * tpl->addKey("confroot", d->ps->confRoot());
         * tpl->addKey("name", d->pkg->name());
         * tpl->addKey("version", d->pkg->version());
         * tpl->addKey("date", QDate::currentDate().toString(Qt::ISODate));
         * tpl->addKey("time", QTime::currentTime().toString(Qt::ISODate));
         * tpl->addKey("arch", SETUP_ARCH);
         * tpl->addKey("purge", (d->pkg->action() == Solver::Purge ? "true" : "false"));
         * 
         * md->setTemplatable(tpl);
         * @endcode
         * 
         * @param tpl Moteur de templates à utiliser.
         */
        void setTemplatable(Templatable *tpl);
        
        
        QString primaryLang() const;            /*!< @brief Langue primaire du paquet (langue maternelle de l'empaqueteur) */
        QString packageDescription() const;     /*!< @brief Description du paquet courant, dans la langue de l'utilisateur ou primaryLang si non disponible */
        QString packageTitle() const;           /*!< @brief Titre du paquet courant */
        QString packageEula() const;            /*!< @brief License utilisateur finale du paquet courant */
        QString currentPackage() const;         /*!< @brief Nom du paquet courant, un paquet binaire construit par ce paquet source */
        QString upstreamUrl() const;            /*!< @brief Url du site web de l'auteur du paquet source (gcc.gnu.org, etc) */
        QStringList triggers() const;           /*!< @brief Liste des triggers lancés par le paquet courant */
        
        QList<ChangeLogEntry *> changelog() const;   /*!< @brief Historique du paquet source */
        QList<SourceDepend *> sourceDepends() const; /*!< @brief Dépendances à la construction du paquet source */
        
        /**
         * @brief Définit le paquet courant
         * 
         * Les fonctions du type packageDescription() ne s'occupent que du
         * paquet courant, défini ici.
         * 
         * @code
         * md->setCurrentPackage("logram-de-artwork");
         * qDebug() << md->packageDescription();
         * 
         * // Affiche la description longue du paquet logram-de-artwork
         * @endcode
         * 
         * @param name Nom du nouveau paquet courant
         */
        void setCurrentPackage(const QString &name);
        QDomElement currentPackageElement() const;  /*!< @brief QDomElement du paquet courant (\<package name="..." /\>) */
        
        /**
         * @brief Obtient une chaîne traduite
         * 
         * Les chaînes de caractère dans un fichier de métadonnées sont
         * traduites en plusieurs langues. Cette fonction permet de trouver
         * la meilleur chaîne et de la retourner. Les langes sont prises dans
         * cet ordre :
         * 
         *  - Langue de l'utilisateur (QLocale::system())
         *  - @p primaryLang
         *  - Première langue disponible dans le fichier
         * 
         * @p element est du style :
         * 
         * @code
         * <element>
         *     <fr>Texte en français</fr>
         *     <en>Text in English</en>
         * </element>
         * @endcode
         * 
         * @param element Élément contenant les traductions
         * @param primaryLang Langue primaire à utiliser si une traduction n'est pas trouvée.
         * @return Chaîne traduite
         */
        static QString stringOfKey(const QDomElement &element, const QString &primaryLang);
        
        /**
         * @brief Obtient une chaîne traduite
         * 
         * Comme la version statique de stringOfKey, sauf que la primaryLang 
         * du fichier de métadonnées est utilisée.
         * 
         * @param element Élément contenant les traductions
         * @return Chaîne traduite
         */
        QString stringOfKey(const QDomElement &element) const;
        
        /**
         * @brief Script
         * 
         * Un fichier de métadonnées peut contenir différent scripts, comme
         * ceux de construction ou de post-installation.
         * 
         * @param key Clef à utiliser (source pour les scripts de la source, ou le nom d'un paquet binaire)
         * @param type Valeur que doit avoir l'attribut @b type du \<script /\> pour qu'il soit pris 
         * @return Script sous forme brute (passable à Bash). QByteArray vide si le script n'existe pas.
         */
        QByteArray script(const QString &key, const QString &type);
        
        /**
         * @brief Lance un script
         * 
         * Lance un script, pouvant être obtenu par script(). Cette fonction
         * bloque pendant que le script tourne
         * 
         * @param script Texte du script (en shell /bin/sh)
         * @param args Arguments à lui passer
         * @return True si le script renvoie 0, false sinon (ou s'il n'a pu se lancer)
         */
        bool runScript(const QByteArray &script, const QStringList &args);
        
        /**
         * @brief Lance un script
         * @overload
         * @param key Clef à utiliser, comme pour script()
         * @param type Comme pour script()
         * @param currentDir Dossier courant dans lequel se placer avant de lancer le script
         * @param args Arguments à lui passer
         * @return True si le script renvoie 0, false sinon ou si erreur
         */
        bool runScript(const QString &key, const QString &type, const QString &currentDir, const QStringList &args);
        
        /**
         * @brief Dernière valeur sortie par le script
         * 
         * Fonction utilitaire permettant de ne pas devoir connecter
         * processLineOut() si seulement la dernière ligne renvoyée
         * par le script vous intéresse, comme les scripts renvoyant
         * comme seule sortie la version d'un paquet, calculée d'une
         * certaine manière.
         * 
         * @return Dernière ligne sortie par le script.
         * @sa runScript
         */
        QByteArray scriptOut() const;
        
    private slots:
        void processDataOut();
        void processTerminated(int exitCode, QProcess::ExitStatus exitStatus);
        
    signals:
        /**
         * @brief Sortie d'une ligne par un script
         * 
         * Ce signal est émis pour chaque ligne sortie par un script.
         * 
         * @param process QProcess du script (/bin/sh avec le script pour entrée)
         * @param line Ligne sortie
         * @sa runScript
         */
        void processLineOut(QProcess *process, const QByteArray &line);
        
    private:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif