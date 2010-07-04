/*
 * packagesource.h
 * This file is part of Logram
 *
 * Copyright (C) 2009 - Denis Steckelmacher <steckdenis@logram-project.org>
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
 * @file packagesource.h
 * @brief Gestion des paquets sources
 */

#ifndef __PACKAGESOURCE_H__
#define __PACKAGESOURCE_H__

#include <QVariant>
#include <QProcess>
#include <QStringList>

#include "templatable.h"

namespace Logram
{

class PackageSystem;
class FilePackage;
class PackageMetaData;
class PackageSourceInterface;

/**
 * @brief Remarque lors de la création d'un paquet source
 * 
 * La création d'un paquet source peut engendrer plusieurs remarques, renvoyées
 * par des plugins (voir PackageSourceInterface).
 */
struct PackageRemark
{
    /**
     * @brief Séverité de la remarque
     */
    enum Severity
    {
        Information,    /*!< @brief Information, peut être ignorée sans le moindre problème */
        Warning,        /*!< @brief Attention, ne devrait pas être ignoré, mais n'interromp pas la construction (le serveur de construction positionne un flag) */
        Error           /*!< @brief Erreur, interromp la construction et positionne un flag. Le paquet binaire est inutilisable */
    };
    
    Severity severity;      /*!< @brief Séverité de la remarque */
    QString packageName;    /*!< @brief Nom du paquet binaire ayant provoqué la remarque */
    QString message;        /*!< @brief Message de la remarque */
};

/**
 * @brief Gestion des paquets sources
 * 
 * Cette classe permet d'effectuer toutes les opérations nécessaires sur les
 * paquets sources, se présentant comme de simples fichiers metadata.xml
 * (éventuellement avec un dossier @b control/ ).
 */
class PackageSource : public Templatable
{
    Q_OBJECT
    
    public:
        /**
         * @brief Constructeur
         * @param ps PackageSystem utilisé
         */
        PackageSource(PackageSystem *ps);
        
        /**
         * @brief Destructeur
         */
        ~PackageSource();
        
        /**
         * @brief PackageSystem utilisé
         * 
         * Les plugins reçoivent en paramètre un PackageSource. Cette fonction
         * permet de récupérer son PackageSystem, et ainsi d'accéder à
         * l'ensemble des fonctions de la @b libpackage, comme l'énumération
         * des paquets.
         * 
         * @return PackageSystem utilisé
         */
        PackageSystem *packageSystem();
        
        /**
         * @brief Listage des fichiers d'un dossier
         * 
         * Fonction utilitaire statique permettant de liste le contenu d'un
         * dossier. Elle diffère de QDirIterator de par le fait que les flags
         * sont automatique positionnés pour correspondre aux fichiers que
         * PackageSource place habituellement dans des paquets binaires.
         * 
         * De plus, les dossiers vides sont compris dans la liste, comme des
         * fichiers. Les dossiers non-vides sont ignorés, seuls leurs fichiers
         * sont ajoutés
         * 
         * @param dir Chemin du dossier à explorer
         * @param prefix Préfixe à ajouter devant chaque entrée (généralement QString())
         * @param list Référence sur une QStringList qui accueillera les entrées
         */
        static void listFiles(const QString &dir, const QString &prefix, QStringList &list);
        
        /**
         * @brief Ajoute des plugins
         * 
         * Permet aux applications d'ajouter des plugins hors des chemins de
         * recherche des plugins (PackageSystem::pluginPaths()).
         * 
         * Le server de construction utilise par exemple cette fonction :
         * 
         * @code
         * // Prendre les plugins hors du chroot
         * QList<PackageSourceInterface *> plugins;
         * 
         * foreach (const QString &pluginPath, tps->pluginPaths())
         * {
         *     QDir pluginDir(pluginPath);
         *     
         *     foreach (const QString &fileName, pluginDir.entryList(QDir::Files))
         *     {
         *         QPluginLoader loader(pluginDir.absoluteFilePath(fileName));
         *         PackageSourceInterface *plugin = qobject_cast<PackageSourceInterface *>(loader.instance());
         *         
         *         if (plugin)
         *         {
         *             plugins.append(plugin);
         *         }
         *     }
         * }
         * 
         * // Chrooter
         * chroot(...);
         * 
         * ...
         * 
         * // Ajout des plugins chargés en mémoire, le chroot n'en contient pas
         * src->addPlugins(plugins);
         * @endcode
         * 
         * @param plugins Liste des plugins à ajouter
         */
        void addPlugins(const QList<PackageSourceInterface *> plugins);
        
        /**
         * @brief Options passables à setOption()
         * 
         * setOption() permet de définir quelques options pour la construction
         * des paquets.
         */
        enum Option
        {
            SourceDir,      /*!< @brief Chemin du dossier source ( @b src/ ) */
            BuildDir,       /*!< @brief Chemin du dossier de construction ( @b build/ ) */
            ControlDir      /*!< @brief Chemin du dossier control ( @b control/ ) */
        };
        
        /**
         * @brief Initialise le paquet source avec un fichier
         * 
         * PackageSource a besoin d'un fichier XML au format metadata.xml pour
         * fonctionner. Cette fonction permet de le lui fournir.
         * 
         * @param fileName Nom du fichier
         */
        bool setMetaData(const QString &fileName);
        
        /**
         * @overload
         */
        bool setMetaData(const QByteArray &data);
        
        /**
         * @brief Définit une option
         * 
         * PackageSource est légèrement configurable, grâce à cette
         * fonction.
         * 
         * Si cette fonction est appelée plusieurs fois pour une même
         * option, le dernier appel a la priorité.
         * 
         * @param opt Option
         * @param value Valeur
         * @sa loadKeys
         */
        void setOption(Option opt, const QVariant &value);
        
        /**
         * @brief Récupère une option
         * @param opt Option
         * @param defaultValue Valeur par défaut
         */
        QVariant option(Option opt, const QVariant &defaultValue);
        
        /**
         * @brief Charge les option
         * 
         * Une fois toutes les options définies (si nécessaire),
         * cette fonction initialise le moteur de template de
         * PackageSource avec ces valeurs.
         * 
         * Actuellement, @b sourcedir, @b builddir, @b controldir et @b arch
         * sont définis.
         * 
         * @sa setOption
         */
        void loadKeys();
        
        bool getSource();                                   /*!< @brief Télécharge la source (appelle le script @b download) */
        bool checkSource(const QString &dir, bool fail);    /*!< @brief Vérifie que le dossier @p dir existe, et renvoie false sinon et si @p fail est à @b true */
        bool build();                                       /*!< @brief Construit la source (appelle le script @b build) */
        bool binaries();                                    /*!< @brief Construit les paquets binaires, exécute les plugins */
        
        QList<PackageRemark *> remarks();                   /*!< @brief Remarques lancées par binaries() */
        void addRemark(PackageRemark *remark);              /*!< @brief Permet aux plugins d'ajouter une remarque */
        
    private:
        struct Private;
        Private *d;
};

/**
    @class PackageSourceInterface
    @brief Interface pour les plugins de gestion des sources
    
    Cette interface permet de créer des plugins effectuant des opérations
    sur les paquets construits par PackageSource.
    
    Ces plugins ont une grande liberté, et peuvent accéder au PackageSystem
    courant, ainsi qu'à tout l'arbre XML du paquet construit et au
    PackageSource utilisé.
    
    Les différentes fonctions de cette classe sont appelées par PackageSource
    comme ceci :
    
    @code
    // Dans le constructeur
    ///////////////////////
    
    QString name = plugin->name();
    // Un hash(name, plugin) est maintenu.
    
    // Pour chaque paquet source
    ////////////////////////////
    
    plugin->init(le PackageMetaData de la source, this);
    
    // Pour chaque paquet binaire
    /////////////////////////////
    
    bool enable = plugin->byDefault();
    allowSourceToChangeEnable(enable);   // <plugin name="..." enable="true|false" />
    
    plugin->processPackage(currentPackageName, listOfFilesInPackage, true si c'est le paquet <source />, sinon false);
    
    // Quand les paquets binaires sont finis, toujours dans le même paquet source
    plugin->end();
    @endcode
*/            
class PackageSourceInterface
{
    public:
        virtual ~PackageSourceInterface() {}
        
        virtual QString name() const = 0;
        virtual bool byDefault() const = 0;
        
        virtual void init(PackageMetaData *md, PackageSource *src) = 0;
        virtual void processPackage(const QString &name, QStringList &files, bool isSource) = 0;
        virtual void end() = 0;
};
    
} /* Namespace */

Q_DECLARE_INTERFACE(Logram::PackageSourceInterface, "org.logram-project.org.lgrpkg.PackageSourceInterface/0.1");

#endif