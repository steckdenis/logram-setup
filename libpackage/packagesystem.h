/*
 * packagesystem.h
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
 * @file packagesystem.h
 * @brief Déclarations globales à la gestion des paquets Logram
 */

#ifndef __LIBPACKAGE_H__
#define __LIBPACKAGE_H__

#include <QObject>
#include <QString>
#include <QList>
#include <QStringList>
#include <QRegExp>

// Architecture de LPM
#ifndef SETUP_ARCH
    #if __SIZEOF_POINTER__ == 4
        #define SETUP_ARCH "i686"
    #else
        #define SETUP_ARCH "x86_64"
    #endif
#endif

class QNetworkReply;
class QSettings;

/**
 * @namespace Logram
 * @brief Espace des noms conteneur des fonctions de LPM
 * 
 * Cet espace des noms contient toutes les classes et toutes les fonctions utilisées par le gestionnaire de paquets Logram.
 * 
 * Il est également utilisé par d'autres bibliothèques de Logram.
 */
namespace Logram
{

class DatabasePackage;
class DatabaseReader;
class Package;
class Solver;
class Communication;
class PackageFile;

/**
 * @brief Structure de gestion des téléchargements
 * 
 * Cette structure permet de gérer les téléchargements asynchrones effectués grâce à
 * PackageSystem::download.
 */
struct ManagedDownload
{
    /**
     * @brief Réponse
     * 
     * Structure QNetworkReply en provenance de Qt, permettant de gérer les erreurs de transfert, des informations
     * comme la progression dans le téléchargement, etc.
     */
    QNetworkReply *reply;
    
    /**
     * @brief Url source
     * 
     * Url du fichier à télécharger. Peut également être le nom d'un fichier local si le téléchargement est de type @b local.
     */
    QString url;
    
    /**
     * @brief Fichier de destination
     * 
     * Le fichier téléchargé est placé là ou le programme appelant l'a décidé. Ce champs contient cette information. C'est
     * toujours un fichier local
     */
    QString destination;
    
    /**
     * @brief Erreur
     * 
     * Est placé à @b true en cas d'erreur. Ce champs est indéterminé tant que le téléchargement n'est pas fini (une erreur le termine).
     */
    bool error;
};

/**
 * @brief Erreur globale
 * 
 * Cette classe permet de gérer les erreurs pouvant se produire lors des différents traitements. Les fonctions de l'API
 * de libpackage ne renvoient pas de code d'erreur, mais bien un booléen permettant juste de savoir si tout s'est bien passé.
 * 
 * Un appel à PackageSystem::lastError() permet de savoir si une erreur s'est produite, et le cas échant, de la récupérer.
 * 
 * Cette manière de faire permet de stocker des informations supplémentaires dans une erreur, comme un message d'erreur, etc.
 */
struct PackageError
{
    /**
     * @brief Type d'erreur
     */
    enum Error
    {
        OpenFileError,      /*!< @brief Impossible d'ouvrir un fichier */
        MapFileError,       /*!< @brief Impossible de mapper un fichier */
        ProcessError,       /*!< @brief Un sous-processus lancé par LPM a échoué */
        DownloadError,      /*!< @brief Erreur dans le téléchargement d'un fichier */
        ScriptException,    /*!< @brief Erreur dans un QtScript */
        SignatureError,     /*!< @brief Erreur dans la signature d'un fichier */
        SHAError,           /*!< @brief Mauvaise somme SHA1 d'un fichier */
        PackageNotFound,    /*!< @brief Paquet non trouvé */
        BadDownloadType,    /*!< @brief Mauvais type de téléchargement, erreur dans sources.list (champs @b Type)*/
        OpenDatabaseError,  /*!< @brief Impossible d'ouvrir la base de donnée MySQL (RepositoryManager) */
        QueryError,         /*!< @brief Erreur dans une requête SQL (RepositoryManager) */
        SignError,          /*!< @brief Erreur dans la signature d'un fichier (création de la signature) */
        InstallError,       /*!< @brief Impossible d'installer un paquet */
        ProgressCanceled    /*!< @brief Progression annulée par l'utilisateur */
    };
    
    Error type;             /*!< @brief Type d'erreur */
    QString info;           /*!< @brief Informations en une ligne (le nom du fichier qu'on peut pas ouvrir, etc) */
    QString more;           /*!< @brief Facultatif : informations supplémentaires (sortie du script qui a planté) */
};

/**
 * @brief dépôt
 * 
 * Cette structure permet de lister les dépôts LPM sans avoir à parser sources.list.
 * Ainsi, sa structure peut changer sans que ça pose de problèmes.
 */
struct Repository
{   
   /**
    * @brief Type de dépôt
    */
    enum Type
    {
        Unknown,        /*!< @brief Inconnu */
        Remote,         /*!< @brief Distant, url en http:// ou ftp:// */
        Local           /*!< @brief Local, chemin absolu */
    };
    
    Type type;          /*!< @brief Type de dépôt */
    
    QString name;       /*!< @brief Nom */
    QString description;            /*!< @brief Description */
    
    QStringList mirrors;            /*!< @brief Mirroirs (urls ou chemins séparés par des espaces) */
    QStringList distributions;      /*!< @brief Distributions séparées par des espaces */
    QStringList architectures;      /*!< @brief Architectures séparées par des espaces */
    
    bool active;                    /*!< @brief Dépôt actif, c'est à dire utilisé par "lpm update" */
    bool gpgcheck;                  /*!< @brief True si le dépôt propose des signatures GPG à vérifier */
};

/**
 * @brief Progression
 * 
 * Certains traitements dans LPM peuvent prendre du temps. Cette classe permet
 * de représenter une étape dans une opération, avec des informations sur celle_ci.
 * 
 * Le champs @b action de cette structure permet de savoir ce qui est effectué sur
 * la progression. En effet, plusieurs signaux PackageSystem::progression() sont émis
 * avec la même progression :
 * 
 *  - Action::Create : Initialise la progression côté client (création d'une barre de progression par exemple)
 *  - Action::Update : Met à jour les valeurs (changement de la barre de progression)
 *  - Action::End : Termine la progression (suppression de la barre de progression)
 * 
 * Action::Update peut être émis plusieurs fois.
 */
struct Progress
{
    /**
     * @brief Type de progression
     */
    enum Type
    {
        Other,                  /*!< @brief Autre, inconnue */
        GlobalDownload,         /*!< @brief Téléchargement global («Téléchargement de foo.bar») */
        Download,               /*!< @brief Téléchargement d'un fichier (pourcentage de foo.bar) */
        UpdateDatabase,         /*!< @brief Mise à jour de la base de donnée, info contient l'étape en cours */
        PackageProcess,         /*!< @brief Progression dans l'installation ou suppression des paquets */
        ProcessOut,             /*!< @brief Sortie d'un processus. info contient sa ligne de commande, more ce qu'il vient de sortir */
        GlobalCompressing,      /*!< @brief Compression globale («Compression de libfoo») */
        Compressing,            /*!< @brief Compression d'un fichier («/usr/share/chose dans libfoo) */
        Including,              /*!< @brief Inclusion d'un paquet */
        Exporting,              /*!< @brief Export d'une distribution. info = distribution, more = architecture */
        Trigger                 /*!< @brief Exécution d'un trigger */
    };
    
    /**
     * @brief Action de la progression
     */
    enum Action
    {
        Create,                 /*!< @brief Création de la progression */
        Update,                 /*!< @brief Mise à jour de la progression */
        End                     /*!< @brief Fin de la progression */
    };
    
    Type type;                  /*!< @brief Type de progression */
    Action action;              /*!< @brief Action */
    int current;                /*!< @brief Étape courante */
    int old;                    /*!< @brief Étape précédante */
    int total;                  /*!< @brief Nombre total d'étapes */
    QString info;               /*!< @brief Information générale (nom du paquet, etc) */
    QString more;               /*!< @brief Information supplémentaire (opération sur ce paquet) */
    void *data;                 /*!< @brief Données supplémentaires. Pointer vers un Package pour type = PackageProcess */
    bool canceled;              /*!< @brief True si l'opération est annulée. C'est l'application utilisant libpackage qui positionne ce membre. Son positionnement fait retourner false à PackageSystem::sendProgress() */
};

struct UpgradeInfo;

/**
 * @brief Coeur de la gestion des paquets Logram
 * 
 * PackageSystem est la classe centrale de la gestion des paquets Logram. Toute opération nécessite d'avoir instancié et initialisé cette classe.
 * 
 * L'initialisation de PackageSystem est assez simple, mais se fait en plusieurs étapes. Tout d'abord, il faut construire la classe :
 * 
 * @code
 * PackageSystem *ps = new PackageSystem(this);
 * @endcode
 * 
 * Ensuite, on peut définir des paramètres, et charger les valeurs par défaut @b après les initialisations
 * 
 * @code
 * ps->setInstallSuggests(true);
 * ps->loadConfig();
 * @endcode
 * 
 * Il faut maintenant connecter quelques signaux permettant de récupérer les communications lancées par des
 * paquets ou PackageSystem lui-même
 * 
 * @code
 * connect(ps, SIGNAL(communication(...)), this, SLOT(communication(...)));
 * @endcode
 * 
 * Si l'application hôte est lancée en @b root, et qu'elle a besoin de la base de donnée binaire (gestion des paquets, mais pas pour créer des paquets), il faut encore appeler init() :
 * 
 * @code
 * ps->init();
 * @endcode
 * 
 * La classe est maintenant prête à être utilisée :
 * 
 * @code
 * Solver *solver = ps->newSolver();
 * solver->addPackage("foo>=bar");
 * solver->solve();
 * solver->weight();
 * // Exploration des résultats et installation...
 * @endcode
 */
class PackageSystem : public QObject
{
    Q_OBJECT
    
    public:
        /**
         * @brief Constructeur
         * @param parent QObject parent
         */
        PackageSystem(QObject *parent = 0);
        ~PackageSystem();                   /*!< @brief Destructeur */
        bool init();                        /*!< @brief Initialise la base de donnée binaire */
        bool initialized() const;           /*!< @brief Permet de savoir si la base de donnée binaire est initialisée */
        void loadConfig();                  /*!< @brief Charge la configuration */

        // API publique
        /**
         * @brief Trouve un paquet par son nom
         * @param regex Regex à laquelle ne nom du paquet doit correspondre
         * @param rs Liste des ID des paquets correspondant
         * @return True si tout s'est bien passé
         * @sa package
         */
        bool packagesByName(const QRegExp &regex, QList<int> &rs);
        
        /**
         * @brief Trouve un paquet en fonction d'un nom et d'une version
         * @param name Nom du paquet
         * @param version qu'il doit avoir
         * @param op Opération (>=, >, !=) entre le nom et la version. Voir DEPEND_OP_*
         * @return Liste des paquets correspondant
         * @sa parseVersion
         */
        QList<int> packagesByVString(const QString &name, const QString &version, int op);
        
        /**
         * @brief Trouve un paquet ayant un nom est une version exacts
         * @param name Nom du paquet
         * @param version du paquet
         * @param rs Paquet trouvé
         * @return True si le paquet est trouvé, false sinon.
         */
        bool package(const QString &name, const QString &version, Package* &rs);
        
        /**
         * @brief Paquet en fonction de son ID
         * 
         * Fonction à bas niveau permettant de trouver un paquet en provenance de la base de donnée
         * en fonction de son ID dans celle-ci, par exemple avec les résultats de packagesByName() ou
         * packagesByVString().
         *
         * @param id ID du paquet
         * @return Paquet correspondant dans la base de donnée, 0 si l'ID est hors bornes.
         */
        DatabasePackage *package(int id);
        
        /**
         * @brief Fichiers ayant un certain nom
         * 
         * Renvoie la liste des fichiers dont le chemin d'accès est @p name. Name ne commence pas par un slash.
         * 
         * @code
         * QList<PackageFile *> files = ps->files("/usr/bin/lpm");
         * PackageFile *file = files.at(0);
         * @endcode
         * 
         * Plusieurs fichiers peuvent avoir le même chemin d'accès, s'ils sont par exemple installés par plusieurs paquets.
         *
         * @param name Nom complet du fichier, avec chemin d'accès
         * @return Liste des fichiers correspondant
         */
        QList<PackageFile *> files(const QString &name);
        
        /**
         * @brief Fichiers dont le nom correspond au motif
         * 
         * Renvoie la liste des fichiers dont le nom correspond à @p regex. Le chemin d'accès est ignoré.
         * 
         * Cette fonction est un peu lente, de complexité O(n) où n est le nombre de fichiers en base de donnée.
         * 
         * @code
         * QList<PackageFile *> files = ps->files(QRegExp("msg*", Qt::CaseSensitive, QRegExp::Wildcard));
         * 
         * // Renvoie ["/usr/bin/msgfmt", "/usr/bin/msgconv", ... , "/usr/share/man/man1/msgfmt.1", ... ]
         * @endcode
         * 
         * @param regex Motif auquel doivent correspondre les noms des fichiers
         * @return Liste des fichiers correspondant
         */
        QList<PackageFile *> files(const QRegExp &regex);
        int packages();                                 /*!< @brief Nombre de paquets dans la base de donnée */
        bool update();                                  /*!< @brief Met à jour la base de donnée, fonction bloquante */
        QList<DatabasePackage *> upgradePackages();     /*!< @brief Liste des paquets pouvant être mis à jour */
        QList<DatabasePackage *> orphans();             /*!< @brief Liste des paquets orphelins */
        Solver *newSolver();                            /*!< @brief Crée un solveur (classe qui a besoin de structures internes de PackageSystem) */

        // Fonctions statiques
        /**
         * @brief Compare deux versions
         * 
         * Équivalent de "(int)(a > b)". Retourne :
         * 
         *  - @b 1 si a > b
         *  - @b 0 si a = b
         *  - @b -1 si a < b
         * 
         * La comparaison s'effectue en respectant la sémantique LPM :
         * 
         *  - 1.2 > 1.1
         *  - 1.1.2 < 1.2.1
         *  - 1.2~3 < 1.2.1~1
         *  - 10.04ubuntu0 = 10.4
         * 
         * @param a Première version
         * @param b Deuxième version
         * @return Comparaison entre les deux
         */
        static int compareVersions(const char *a, const char *b);
        
        /**
         * @overload
         */
        static int compareVersions(const QByteArray &v1, const QByteArray &v2);
        
        /**
         * @brief Vérifie que deux versions conviennent
         * 
         * Cette fonction permet de vérifier que deux versions conviennent à une opération.
         * Par exemple, si @p v1 = "1.2" et @p v2 = "1.3" et que @p op = '<=', le résultat
         * sera @b true.
         * 
         * @code
         * QByteArray name, version;
         * int op = PackageSystem::parseVersion("foo>=1.2", name, version);
         * 
         * qDebug() << PackageSystem::matchVersion("1.3", version, op); // true
         * @endcode
         * 
         * @param v1 Version à comparer
         * @param v2 Version de référence
         * @param op Opération devant réussir (DEPEND_OP_*)
         * @return True si l'opération est vérifiée
         * @sa parseVersion
         */
        static bool matchVersion(const QByteArray &v1, const QByteArray &v2, int op);
        
        /**
         * @brief Transforme une chaîne lisible par un humain en une description de version
         * 
         * Transforme une chaîne comme "foo>=1.2" en "foo", DEPEND_OP_GREQ, "1.2"
         * 
         * @param verStr Chaîne source
         * @param name Nom du paquet
         * @param version Version de comparaison, QString() si pas de comparaison
         * @return Opération qui doit être vérifiée.
         */
        static int parseVersion(const QByteArray &verStr, QByteArray &name, QByteArray &version);
        
        /**
         * @brief Chaîne lisible par un humain pour une taille en octets
         * 
         * Transforme @b 1024 en @b 1 @b Kio. Les unités disponibles sont :
         * 
         *  - @b o : octets
         *  - @b Kio : kibioctets
         *  - @b Mio : mibioctets
         *  - @b Gio : gibioctets
         * 
         * La sortie est sous forme d'un nombre à virgule et d'une unité, comme 4,00 Kio, 25,13 Mio, etc. La taille maximale
         * est de 11 caractères (1023,99 Kio).
         */
        static QString fileSizeFormat(int size);
        
        /**
         * @brief Inverse de parseVersion
         * 
         * Transforme "foo", DEPEND_OP_GREQ, "1.2" en "foo>=1.2"
         * 
         * @param name Nom du paquet
         * @param version Version, si nécessaire
         * @param op Opération
         * @return Chaîne formattée aux normes LPM
         */
        static QString dependString(const QString &name, const QString &version, int op);

        // API utilisée par des éléments de liblpackages
        /**
         * @brief Lance le téléchargement d'un fichier
         * @param type Type de dépôt
         * @param url Url du fichier
         * @param dest Fichier local de destination
         * @param block True si la fonction doit attendre que le téléchargement soit fini.
         * @param rs ManagedDownload permettant de contrôler le téléchargement
         * @note Que @p block soit à true ou false, les progressions de type Progress::Download seront envoyées
         * @return True si tout s'est bien passé
         */
        bool download(Repository::Type type, const QString &url, const QString &dest, bool block, ManagedDownload* &rs);
        QList<Repository> repositories() const;                     /*!< @brief Liste des dépôts */
        
        /**
         * @brief Dépôt correspondant à un nom
         * @param name Nom du dépôt
         * @param rs Dépôt
         * @return True si le dépôt existe, false sinon
         */
        bool repository(const QString &name, Repository &rs) const;
        
        /**
         * @brief Liste des paquets installés
         * @internal
         */
        QSettings *installedPackagesList() const;

        // Options
        bool installSuggests() const;               /*!< @brief Installe les suggestions des paquets */
        int parallelDownloads() const;              /*!< @brief Nombre de téléchargements en parallèle */
        int parallelInstalls() const;               /*!< @brief Nombre d'installations parallèle */
        QString installRoot() const;                /*!< @brief Dossier racine pour l'installation */
        QString confRoot() const;                   /*!< @brief Dossier racine pour la configuration (root/etc/lgrpkg/...) */
        QString varRoot() const;                    /*!< @brief Dossier racine pour la base de donnée (root/var/cache/...) */
        QStringList pluginPaths() const;            /*!< @brief Chemin d'accès aux plugins PackageSource */
        bool runTriggers() const;                   /*!< @brief True pour lancer les triggers */
        void setInstallSuggests(bool enable);       /*!< @brief Installe les suggestions des paquets */
        void setParallelDownloads(int num);         /*!< @brief Nombre de téléchargements en parallèle */
        void setParallelInstalls(int num);          /*!< @brief Nombre d'installations en parallèle */
        void setInstallRoot(const QString &root);   /*!< @brief Dossier racine pour l'installation */
        void setConfRoot(const QString &root);      /*!< @brief Dossier racine pour la configuration */
        void setVarRoot(const QString &root);       /*!< @brief Dossier racine pour la base de donnée */
        void setRunTriggers(bool enable);           /*!< @brief True pour lancer les triggers */
        void addPluginPath(const QString &path);    /*!< @brief Ajoute un chemin de recherche pour les plugins */
        
        // Gestion des erreurs
        void setLastError(PackageError *err);       /*!< @brief Définit la dernière erreur, thread-safe */
        void setLastError(PackageError::Error type, const QString &info, const QString &more = QString()); /*!< @overload */
        PackageError *lastError();                  /*!< @brief Dernière erreur, ou 0 si pas d'erreur */

        /**
         * @brief Démarre une progression
         * @param type Type de progression
         * @param tot Nombre total d'étapes
         * @return Identificateur à passer à sendProgress() et endProgress()
         */
        int startProgress(Progress::Type type, int tot);
        
        /**
         * @brief Met à jour une progression
         * @param id ID de la progression, renvoyé par startProgress()
         * @param num Numéro de l'étape
         * @param msg Message d'informations
         * @param more Plus d'informations
         * @param data Données pouvant être jointes, par exemple Package* joint à Progress::PackageProcess
         * @return True si l'opération peut continuer, false si elle doit s'arrêter
         */
        bool sendProgress(int id, int num, const QString &msg, const QString &more = QString(), void *data = 0) __attribute__((warn_unused_result));
        
        /**
         * @brief Informe qu'un sous-processus a sorti une ligne
         * 
         * Se sert de sendProgress(), en utilisant une ID spéciale pour la sortie des processus.
         * 
         * @param command Commande qui a sorti une ligne
         * @param line Ligne sortie
         * @return True si l'opération peut continuer, false sinon
         */
        bool processOut(const QString &command, const QString &line) __attribute__((warn_unused_result));
        
        /**
         * @brief Termine une progression
         * @param id ID de la progression
         */
        void endProgress(int id);
        
        /**
         * @brief Lance une communication
         * @param sender Paquet de la communication, 0 si elle ne vient pas d'un paquet (communication alors considérée comme globale, genre «Insérez un CD»)
         * @param comm Communication
         */
        void sendCommunication(Package *sender, Communication *comm);
        
        // Usage interne
        QString bestMirror(const Repository &repo);     /*!< @internal */
        void releaseMirror(const QString &mirror);      /*!< @internal */
        DatabaseReader *databaseReader();               /*!< @internal */
        void saveFile(PackageFile *file);               /*!< @internal */
        void syncFiles();                               /*!< @brief Enregistre les changements apportés aux fichiers dans installed_files.list. Appelé automatiquement par le destructeur, peut être appelé manuellement si l'application a du temps à perdre */

    signals:
        void progress(Logram::Progress *progress);      /*!< @brief Progression émise */
        void downloadEnded(Logram::ManagedDownload *reply); /*!< @brief Téléchargement lancé par download() terminé */
        void communication(Logram::Package *sender, Logram::Communication *comm);   /*!< @brief Communication lancée (sender = 0 si communication globale) */

    private slots:
        void downloadFinished(QNetworkReply *reply);
        void dlProgress(qint64 done, qint64 total);

    protected:
        struct Private;
        Private *d;
};

} /* Namespace */

#endif