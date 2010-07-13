<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0" language="en_US">
<defaultcodec>UTF-8</defaultcodec>
<context>
    <name>App</name>
    <message>
        <location filename="../lpm/app.cpp" line="+459"/>
        <source>Utilisation : lpm [options] &lt;action&gt; [arguments]
    help               Afficher l&apos;aide
    version            Afficher la version
    search &lt;pattern&gt;   Afficher tous les paquets dont le nom
                       correspond à &lt;pattern&gt;
    showpkg &lt;name&gt;     Affiche les informations du paquet &lt;name&gt;
    getsource &lt;name&gt;   Télécharge le paquet source de &lt;name&gt;
    update             Met à jour la base de donnée des paquets
    add &lt;packages&gt;     Ajoute des paquets (préfixés de &quot;-&quot; pour les supprimer)
    files &lt;pkg&gt;        Affiche la liste des fichiers installés par &lt;pkg&gt;
    file &lt;path|regex&gt;  Affiche des informations sur le fichier &lt;path&gt;,
                       s&apos;il commence par &quot;/&quot;. Sinon, &lt;path&gt; est considéré
                       comme une expression régulière permettant de trouver
                       la liste des fichiers correspondant à ce motif.
    upgrade            Mise à jour des paquets. Lancez update avant.
    autoremove         Supprimer automatiquement les paquets orphelins.
    tag [file|package] Tag les paquets correspondants à &lt;pkg&gt; (p&gt;=v, etc) avec
        &lt;pkg|fl&gt; &lt;tag&gt; le tag &lt;tag&gt;. Si &lt;tag&gt; commence par &quot;-&quot;, alors retirer
                       ce tag. Si &lt;pkg&gt; est un fichier (/usr/truc, ou mac*n),
                       alors c&apos;est ce seront les fichiers correspondants au
                       motif qui seront taggués. &lt;fl&gt; peut être de la forme
                       &quot;pkg:fl&quot;, ce qui permet de ne taguer que les
                       fichiers correspondants à &lt;fl&gt; et appartenant à &lt;pkg&gt;

Commandes pour la gestion des sources :
    download &lt;src&gt;     Télécharge la source du paquet dont &lt;src&gt; est le
                       metadata.xml.
    build &lt;src&gt;        Compile la source spécifiée par le metadata.xml &lt;src&gt;.
    binaries &lt;src&gt;     Créer les .lpk binaires du metadata.xml &lt;src&gt;.

Commandes pour la gestion des dépôts :
    include &lt;pkg&gt;      Inclus le paquet &lt;pkg&gt; dans le dépôt config/repo.conf
    export &lt;distros&gt;   Exporte la liste des paquets et créer les fichiers
                       packages.lzma et translate.lang.lzma pour les
                       distributions spécifiées.

Options (insensible à la casse) :
    -S [off]           Active (on) ou pas (off) l&apos;installation des suggestions.
    -nD                Ignorer les dépendances : installer les paquets demandés
                       et uniquement eux.
    -nI                Ignorer les paquets installés, générer tout l&apos;arbre de
                       dépendances.
    -G                 Sortie dans stdout la représentation Graphviz de l&apos;arbre
                       des dépendances.
    -I &lt;num&gt;           Définit le nombre de téléchargements en parallèle.
    -D &lt;num&gt;           Définit le nombre d&apos;installations en parallèle.
    -iR &lt;install root&gt; Chemin d&apos;installation racine (&quot;/&quot; par défaut).
                       Sert à installer un «Logram dans le Logram».
    -cR &lt;conf root&gt;    Chemin racine de la configuration (&quot;/&quot; par défaut).
    -vR &lt;var root&gt;     Chemin racine des fichiers temporaires (&quot;/&quot; par défaut).
    -P path            Ajoute un chemin à inspecter pour trouver les plugins
                       de vérification des paquets (lpm binaries).
    -C                 Affiche l&apos;historique des modifications d&apos;un paquet
                       quand utilisé avec showpkg.
    -L                 Affiche la licence d&apos;un paquet quand utilisé avec showpkg.
    -T                 Désactive l&apos;exécution des triggers.
    -W                 Désactive les couleurs dans la sortie de LPM.
</source>
        <translation>Usage: lpm [options] &lt;action&gt; [arguments]
    help               Show help
    version            Afficher the version
    search &lt;pattern&gt;   Show all packages with a name
                       matching &lt;pattern&gt;
    showpkg &lt;name&gt;     Show informations about &lt;name&gt;
    getsource &lt;name&gt;   Download the source package of &lt;name&gt;
    update             Update the packages database
    add &lt;packages&gt;     Add packages (prefix them with &quot;-&quot; to remove)
    files &lt;pkg&gt;        Show the files installed by &lt;pkg&gt;
    file &lt;path|regex&gt;  Show informations about the file &lt;path&gt;,
                       if it begins with &quot;/&quot;. Else, &lt;path&gt; is considered
                       as a regular expression returing the list of the
                       files matching this pattern.
    upgrade            Updating of the packages. Launch update before.
    autoremove         Automatically remove orphan packages.
    tag [file|package] Tag the packages matching &lt;pkg&gt; (e.g. p&gt;=v) with
        &lt;pkg|fl&gt; &lt;tag&gt; the tag &lt;tag&gt;. If &lt;tag&gt; begins with &quot;-&quot; then remove
                       it. If &lt;pkg&gt; is a file (/usr/truc, or some*ing)
                       then the files matching &lt;fl&gt; (see the &quot;file&quot; operation) will be
                       tagged. &lt;fl&gt; can be in the form of &quot;pkg:fl&quot;, and will tag
                       the files matching &lt;fl&gt; and belonging to &lt;pkg&gt;

Commands to manage source packages:
    download &lt;src&gt;     Download the source of the package, following the
                       instructions in the XML file &lt;src&gt;.
    build &lt;src&gt;        Compile the source following the XML file &lt;src&gt;.
    binaries &lt;src&gt;     Create the binary .lpk packages of &lt;src&gt;.

Commands to manage repositories:
    include &lt;pkg&gt;      Include the package &lt;pkg&gt; in the repository
    export &lt;distros&gt;   Exports the repository, create the files in dist/
                       (packages.xz and translate.lang.xz)for the distributions
                       &lt;distros&gt; (space separated names).

Options (case insensitive):
    -S [off]           Enable (on) or not (off) the installation of the suggestions.
    -nD                Ignore the dependencies, only install the packages specified
                       on the command line.
    -nI                Ingore the installed packages, re-install all the dependencies
    -G                 Output on stdout the Graphviz form of the dependencies tree.
    -I &lt;num&gt;           Number of parallel installations.
    -D &lt;num&gt;           Number of parallel downloads.
    -iR &lt;install root&gt; Root installation path (&quot;/&quot; by default).
                       Useful to install a «Logram in Logram».
    -cR &lt;conf root&gt;    Root path of the configuration (&quot;/&quot; by default).
    -vR &lt;var root&gt;     Root path of the temporary files (&quot;/&quot; by default).
    -P path            Add a path to explore to find plugins used to check
                       the binary packages made by the &quot;binaries&quot; operation.
    -C                 Show the package history when used with &quot;showpkg&quot;.
    -L                 Show the package license when used with &quot;showpkg&quot;.
    -T                 Disable the execution of the triggers.
    -W                 Disable the colored output of LPM.
</translation>
    </message>
    <message>
        <source>Logram LPM </source>
        <translation type="obsolete">Logram LPM </translation>
    </message>
    <message>
        <location line="+65"/>
        <source>Logram Package Manager </source>
        <translation>Logram Package Manager </translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Impossible d&apos;ouvrir le fichier </source>
        <translation>Unable to open the file </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Impossible de mapper le fichier </source>
        <translation>Unable to map the file </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Erreur dans la commande </source>
        <translation>Error in the command </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Impossible de télécharger </source>
        <translation>Unable to download </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Erreur dans le QtScript </source>
        <translation>Error in the QtScript </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Mauvaise signature GPG du fichier </source>
        <translation>Bad GPG signature of the file </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Mauvaise somme SHA1, fichier corrompu : </source>
        <translation>Bad SHA1 sum, file corrupted: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Paquet inexistant : </source>
        <translation>Package not found:</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Mauvais type de téléchargement, vérifier sources.list : </source>
        <translation>Bad download type, please check sources.list: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Impossible d&apos;ouvrir la base de donnée : </source>
        <translation>Unable to open the database: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Erreur dans la requête : </source>
        <translation>Error in the query: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Impossible de vérifier la signature : </source>
        <translation>Unable to check the signature: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Impossible d&apos;installer le paquet </source>
        <translation>Unable to install the package </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Opération annulée : </source>
        <translation>Operation canceled: </translation>
    </message>
    <message>
        <location line="+11"/>
        <location filename="../lpm/package.cpp" line="+67"/>
        <location line="+34"/>
        <source>ERREUR : </source>
        <translation>ERROR: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Pas d&apos;erreur ou erreur inconnue</source>
        <translation>No error or unknown error</translation>
    </message>
    <message>
        <location line="+238"/>
        <source>Progression : </source>
        <translation>Progression: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Téléchargement de </source>
        <translation>Downloading of </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Mise à jour de la base de donnée : </source>
        <translation>Updating of the database: </translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Opération sur </source>
        <translation>Operation on </translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Installation de </source>
        <translation>Installation of </translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Suppression de  </source>
        <translation>Removal of     </translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Purge de        </source>
        <translation>Purge of        </translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Mise à jour de  </source>
        <translation>Upgrading of    </translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Sortie du processus : </source>
        <translation>Process out: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Création du paquet </source>
        <translation>Creation of the packag </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Compression de </source>
        <translation>Compression of </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Inclusion de </source>
        <translation>Including of </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Export de la distribution </source>
        <translation>Export of the distribution </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Exécution du trigger </source>
        <translation>Executing trigger </translation>
    </message>
    <message>
        <location line="+47"/>
        <source>vers</source>
        <comment>Mise à jour de &lt;paquet&gt;~&lt;version&gt; vers &lt;nouvelle version&gt;</comment>
        <translation>to</translation>
    </message>
    <message>
        <location filename="../lpm/cache.cpp" line="+151"/>
        <source> appartient à </source>
        <translation> belongs to </translation>
    </message>
    <message>
        <location line="+17"/>
        <source> (installé)</source>
        <translation> (installed)</translation>
    </message>
    <message>
        <location line="+51"/>
        <source>Liste des fichiers installés par %1 :</source>
        <translation>List of the files installed by %1:</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Légende : 
  * d: dossier
  * i: installé
  * r: ne pas supprimer, sauf si on purge
  * p: ne pas supprimer, même si on purge
  * b: toujours sauvegarder avant remplacement
  * c: sauvegarder seulement si modifié localement depuis l&apos;installation
  * o: toujours écraser, même si un autre paquet le remplace
  * v: n&apos;est pas installé par le paquet, mais créé à l&apos;utilisation
</source>
        <translation>Legend:
  * d: directory
  * i: installed
  * r: don&apos;t remove, except if we purge
  * p: don&apos;t remove, event if we purge
  * b: always backup before replacing
  * c: backup only if locally modified since the installation
  * o: always overwrite, even if another package replaces it
  * v: isn&apos;t installed by the package, but created when it is used
</translation>
    </message>
    <message>
        <location line="+131"/>
        <source>%1 fichiers vont être tagués, continuer ? </source>
        <translation>%1 files will be tagged, continue? </translation>
    </message>
    <message>
        <location line="+47"/>
        <location line="+88"/>
        <source>Tags disponibles :</source>
        <translation>Available tags:</translation>
    </message>
    <message>
        <location line="-86"/>
        <source>  * dontremove  : Ne pas supprimer quand son paquet est supprimé
  * dontpurge   : Ne pas supprimer même si son paquet est purgé
  * backup      : Toujours sauvegarder (.bak) avant remplacement
  * checkbackup : Sauvegarder avant remplacement si modifié
  * overwrite   : Ne jamais sauvegarder avant remplacement
</source>
        <translation>  * dontremove  : Don&apos;t remove when its package is removed
  * dontpurge   : Don&apos;t remove even if its package is purged
  * backup      : Always backup (.bak) before replacing
  * checkbackup : Backup before replacing if modified
  * overwrite   : Never backup before replacing
</translation>
    </message>
    <message>
        <location line="+40"/>
        <source>%1 paquets vont être tagués, continuer ? </source>
        <translation>%1 packages will be tagged, continue? </translation>
    </message>
    <message>
        <location line="+48"/>
        <source>  * dontupdate  : Ne pas mettre à jour le paquet
  * dontinstall : Ne pas installer le paquet
  * dontremove  : Ne pas supprimer le paquet
  * wanted      : Ne pas supprimer ce paquet s&apos;il n&apos;est plus nécessaire
</source>
        <translation>  * dontupdate  : Don&apos;t update the package
  * dontinstall : Don&apos;t install the package
  * dontremove : Don&apos;t remove the package
  * wanted      : Don&apos;t remove this package if it isn&apos;t needed by another
</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Intégration à KDE        : </source>
        <translation>KDE integration          : </translation>
    </message>
    <message>
        <location line="+5"/>
        <source>pas intégré</source>
        <translation>not integrated</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>utilisable</source>
        <translation>useable</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>bien intégré</source>
        <translation>well integrated</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>parfaitement intégré</source>
        <translation>perfectly integrated</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Oui</source>
        <translation>Yes</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Non</source>
        <translation>No</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Paquet graphique         : </source>
        <translation>Graphical package        : </translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Paquet principal         : </source>
        <translation>Primary package          : </translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Ne pas mettre à jour     : </source>
        <translation>Don&apos;t update             : </translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Ne pas installer         : </source>
        <translation>Don&apos;t install            : </translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Ne pas supprimer         : </source>
        <translation>Don&apos;t remove             : </translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Licence à approuver      : </source>
        <translation>License to approve       : </translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Nécessite un redémarrage : </source>
        <translation>Needs a reboot           : </translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Installé manuellement    : </source>
        <translation>Manually installed       : </translation>
    </message>
    <message>
        <location line="+39"/>
        <source>Supprimé</source>
        <translation>Deleted</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Installé</source>
        <translation>Installed</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Non installé</source>
        <translation>Not installed</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Nom                 : </source>
        <translation>Name                : </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Version             : </source>
        <translation>Version             : </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Titre               : </source>
        <translation>Title               : </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Drapeaux            : </source>
        <translation>Flags               : </translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Section             : </source>
        <translation>Section             : </translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Distribution        : </source>
        <translation>Distribution        : </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Dépôt d&apos;origine     : </source>
        <translation>Repository          : </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Status              : </source>
        <translation>Status              : </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Utilisé par         : </source>
        <translation>Used by             : </translation>
    </message>
    <message numerus="yes">
        <location line="+0"/>
        <source>%n paquet(s)</source>
        <translation>
            <numerusform>%n package</numerusform>
            <numerusform>%n packages</numerusform>
        </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Date d&apos;installation : </source>
        <translation>Installation date   : </translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Date de suppression : </source>
        <translation>Deletion date       : </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Téléchargement      : </source>
        <translation>Downloading         : </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Taille installée    : </source>
        <translation>Installed size      : </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Paquet source       : </source>
        <translation>Source package      : </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Adresse de l&apos;auteur : </source>
        <translation>Author&apos;s website    : </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Licence             : </source>
        <translation>License             : </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Mainteneur          : </source>
        <translation>Maintainer          : </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Description courte  : </source>
        <translation>Short description   : </translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Description longue : </source>
        <translation>Long description    : </translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Dépendances :</source>
        <translation>Dependencies        : </translation>
    </message>
    <message>
        <location line="+1"/>
        <location filename="../lpm/package.cpp" line="+30"/>
        <source>Légende : </source>
        <translation>Legend: </translation>
    </message>
    <message>
        <location line="+1"/>
        <location filename="../lpm/package.cpp" line="+1"/>
        <source>D: Dépend </source>
        <translation>D: Depend </translation>
    </message>
    <message>
        <location line="+1"/>
        <location filename="../lpm/package.cpp" line="+1"/>
        <source>S: Suggère </source>
        <translation>S: Suggest </translation>
    </message>
    <message>
        <location line="+1"/>
        <location filename="../lpm/package.cpp" line="+1"/>
        <source>C: Conflit </source>
        <translation>C: Conflict </translation>
    </message>
    <message>
        <location line="+1"/>
        <location filename="../lpm/package.cpp" line="+1"/>
        <source>P: Fourni </source>
        <translation>P: Provide </translation>
    </message>
    <message>
        <location line="+1"/>
        <location filename="../lpm/package.cpp" line="+1"/>
        <source>R: Remplace </source>
        <translation>R: Replace </translation>
    </message>
    <message>
        <location line="+1"/>
        <location filename="../lpm/package.cpp" line="+1"/>
        <source>N: Est requis par</source>
        <translation>N: Needed by</translation>
    </message>
    <message>
        <location line="+71"/>
        <source>Versions disponibles : </source>
        <translation>Available versions: </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Légende : * = Disponible, I = installée, R = supprimée</source>
        <translation>Legend: * = Available, I = installed, R = removed</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>Historique des versions : </source>
        <translation>Versions history: </translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Faible priorité : </source>
        <translation>Low priority: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Nouvelles fonctionnalités : </source>
        <translation>New features: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Correction de bogues : </source>
        <translation>Bugfixes: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Mise à jour de sécurité : </source>
        <translation>Security update: </translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Par %1 &lt;%2&gt;</source>
        <translation>By %1 &lt;%2&gt;</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Texte de la licence : </source>
        <translation>License text: </translation>
    </message>
    <message>
        <location filename="../lpm/communication.cpp" line="+45"/>
        <source>Question de </source>
        <translation>Question of </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Message de </source>
        <translation>Message of </translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Question générale : </source>
        <translation>General question: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Message général : </source>
        <translation>General message: </translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Appuyez sur Entrée pour continuer </source>
        <translation>Press Enter to continue </translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Entrez une chaîne de caractères : </source>
        <translation>Enter a character string: </translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Entrez un nombre entier : </source>
        <translation>Enter a number: </translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Entrez un nombre décimal : </source>
        <translation>Enter a decimal number: </translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Voici les choix possibles : </source>
        <translation>Here are the possible choices: </translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Entrez le numéro du choix que vous voulez : </source>
        <translation>Enter the number of the choice you want: </translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Entrez les numéros des choix, séparés par des virgules, sans espaces : </source>
        <translation>Enter the numbers of the choices, separated by commas, without any space: </translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Entrée invalide : </source>
        <translation>Invalid input: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Aucune information donnée</source>
        <translation>No informations given</translation>
    </message>
    <message>
        <location filename="../lpm/package.cpp" line="-69"/>
        <location line="+803"/>
        <source>Bug dans LPM !</source>
        <translation>Bug in LPM !</translation>
    </message>
    <message>
        <location line="-769"/>
        <source>Le paquet dont vous souhaitez récupérer la source doit provenir d&apos;un dépôt</source>
        <translation>The package from which you want to fetch the source must come from a repository</translation>
    </message>
    <message>
        <location line="+27"/>
        <source>Ce paquet pourrait nécessiter ceci pour être compilé :</source>
        <translation>This package may need this to compile:</translation>
    </message>
    <message>
        <location line="+57"/>
        <source>Il n&apos;y a aucun paquet orphelin dans votre système.</source>
        <translation>There are no orphan packages in your system.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Paquets installés automatiquement et qui ne sont plus nécessaires :</source>
        <translation>Automatically installed packages that are not needed:</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+104"/>
        <source>Entrez la liste des paquets à mettre à jour, ou &quot;n&quot; pour abandonner</source>
        <translation>Enter the list of the packages to update, or &quot;n&quot; to cancel</translation>
    </message>
    <message>
        <location line="-14"/>
        <source>Aucune mise à jour n&apos;est disponible. En cas de doutes, essayez de lancer l&apos;opération «update».</source>
        <translation>No update is available. If in doubt, try to launch the &quot;update&quot; operation.</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Paquets pour lesquels une mise à jour est disponible :</source>
        <translation>Packages for which there is an update:</translation>
    </message>
    <message>
        <location line="+198"/>
        <source>(licence à accepter)</source>
        <translation>(license to accept)</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>(redémarrage)</source>
        <translation>(reboot)</translation>
    </message>
    <message>
        <location line="+108"/>
        <source>Installer</source>
        <translation>Install</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Supprimer</source>
        <translation>Remove</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Mettre à jour</source>
        <translation>Update</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Supprimer totalement</source>
        <translation>Completely remove</translation>
    </message>
    <message>
        <location line="+34"/>
        <source>Erreur inconnue. Ajoutez le paramètre -G à LPM et postez sa sortie dans votre rapport de bug</source>
        <translation>Unknown error. Add the -G parameter to LPM and post its output in your bug report please</translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Impossible de trouver la dépendance correspondant à %1</source>
        <translation>Unable to find the dependency matching %1</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Aucun des choix nécessaires à l&apos;installation de %1 n&apos;est possible</source>
        <translation>No choice needed to install %1 is available</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Aucun des choix permettant d&apos;effectuer l&apos;opération que vous demandez n&apos;est possible</source>
        <translation>No choice allowing to perform the operation you want is possible</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Le paquet %1 devrait être installé et supprimé en même temps</source>
        <translation>The package %1 wants to be installed and removed at the same time</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Les paquets %1 et %2 devraient être installés en même temps</source>
        <translation>The packages %1 and %2 want to be installed at the same time</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Le paquet %1 ne peut être installé mais devrait être installé</source>
        <translation>The package %1 cannot be install but wants to be installed</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Le paquet %1 ne peut être supprimé mais devrait être supprimé</source>
        <translation>The package %1 cannot be removed but wants to be removed</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Le paquet %1 ne peut être mis à jour mais devrait être mis à jour</source>
        <translation>The package %1 cannot be updated but wants to be updated</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Erreur : </source>
        <translation>Error: </translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Erreur inconnue, sans doutes un bug</source>
        <translation>Unknown error, may be a bug</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Liste des paquets pouvant être installés :</source>
        <translation>List of the installable packages:</translation>
    </message>
    <message>
        <location line="+1"/>
        <location line="+149"/>
        <source>    Légende : </source>
        <translation>    Legend: </translation>
    </message>
    <message>
        <location line="-148"/>
        <location line="+149"/>
        <source>I: Installé </source>
        <translation>I: Installed</translation>
    </message>
    <message>
        <location line="-148"/>
        <location line="+149"/>
        <source>R: Supprimé </source>
        <translation>R: Removed </translation>
    </message>
    <message>
        <location line="-148"/>
        <location line="+149"/>
        <source>U: Mis à jour </source>
        <translation>U: Updated </translation>
    </message>
    <message>
        <location line="-148"/>
        <location line="+149"/>
        <source>P: Supprimé totalement </source>
        <translation>P: Completely removed </translation>
    </message>
    <message>
        <location line="-139"/>
        <source>Le paquet %1~%2 possède une dépendance permettant un choix :</source>
        <translation>The package %1~%2 has a dependency allowing a choice:</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Un des paquets que vous avez demandé peut être obtenu de plusieurs manières :</source>
        <translation>One of the packages you want to install can be obtained by several manners:</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>(ce choix n&apos;aura aucune influence sur le système)</source>
        <translation>(this choice will have no influence on your system)</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Poids entre %1 et %2 (téléchargement de %3 à %4, %5%6%7%8)</source>
        <translation>Weight between %1 and %2 (downloading of %3 to %4, %5%6%7%8)</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>suppression de </source>
        <translation>removal of </translation>
    </message>
    <message>
        <location line="+0"/>
        <source>installation de </source>
        <translation>installation of </translation>
    </message>
    <message>
        <location line="+2"/>
        <source> à </source>
        <translation> to </translation>
    </message>
    <message>
        <location line="+0"/>
        <source> à suppression de </source>
        <translation> to removal of </translation>
    </message>
    <message>
        <location line="+0"/>
        <source> à installation de </source>
        <translation> to installation of </translation>
    </message>
    <message>
        <location line="+26"/>
        <source>Solution à choisir (c : Annuler, u : Revenir au choix précédant) : </source>
        <translation>Solution to choose (c: Cancel, u: Return to the previous choice): </translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Pas de choix précédant</source>
        <translation>No previous choice</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Impossible de choisir cette entrée : </source>
        <translation>Unable to choose this entry: </translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Si vous avez essayé tous les choix, essayez de remonter (u).</source>
        <translation>If you have tried all the choices, try to go back (u).</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Les changements que vous demandez sont déjà appliqués. Essayez -nI -nD pour les forcer</source>
        <translation>The changes that you ask are already applied. Try -nI -nD to force them</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Paquets qui seront installés ou supprimés :</source>
        <translation>Packages that will be installed or removed:</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>%1 licence(s) à accepter, installation de %2, téléchargement de %3, accepter (y/n) ? </source>
        <translation>%1 license(s) to accept, installation of %2, downloading of %3, accept (y/n)? </translation>
    </message>
    <message>
        <location line="+7"/>
        <source>%1 licence(s) à accepter, suppression de %2, téléchargement de %3, accepter (y/n) ? </source>
        <translation>%1 license(s) to accept, removal of %2, downloading of %3, accept (y/n)? </translation>
    </message>
    <message>
        <location line="+25"/>
        <source>License du paquet %1</source>
        <translation>License of the package %1</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Accepter cette licence (y/n) ? </source>
        <translation>Accept this license (y/n)? </translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Installation des paquets...</source>
        <translation>Installation of the packages...</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Les paquets suivants ont été installés automatiquement et ne sont plus nécessaires :</source>
        <translation>The following packages were automatically installed and aren&apos;t needed:</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Pour une suppression totale de %1. Utilisez lpm autoremove pour supprimer ces paquets.</source>
        <translation>For a total removal of %1. Use lpm autoremove to remove these packages.</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Un ou plusieurs des paquets que vous avez installés nécessitent un redémarrage</source>
        <translation>One or more packages that you have installed need a reboot</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Opérations sur les paquets appliquées !</source>
        <translation>Operations on the packages applied !</translation>
    </message>
    <message>
        <location filename="../lpm/source.cpp" line="+87"/>
        <source>Remarques sur le paquet :</source>
        <translation>Remarks on the package:</translation>
    </message>
    <message>
        <location line="+23"/>
        <source>Information : </source>
        <translation>Information: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Attention   : </source>
        <translation>Warning    : </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Erreur      : </source>
        <translation>Error      : </translation>
    </message>
</context>
<context>
    <name>BranchePage</name>
    <message>
        <location filename="../pkgui/branchepage.cpp" line="+265"/>
        <source>Téléchargement de %1, suppression de %2, %3 licence(s) à accepter</source>
        <translation>Downloading of %1, deletion of %2, %3 license(s) to accept</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Téléchargement de %1, installation de %2, %3 licence(s) à accepter</source>
        <translation>Downloading of %1, installation of %2, %3 license(s) to accept</translation>
    </message>
    <message>
        <location line="+20"/>
        <location line="+2"/>
        <source>Entre %1 et %2</source>
        <translation>Between %1 and %2</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>(n&apos;ajoute pas de dépendance)</source>
        <translation>(doesn&apos;t add any dependency)</translation>
    </message>
</context>
<context>
    <name>CheckFiles</name>
    <message>
        <location filename="../libpackage/plugins/checkfiles/checkfiles.cpp" line="+64"/>
        <source>Le fichier %1 ne se trouve dans aucun paquet</source>
        <translation>The file %1 was not found in any package</translation>
    </message>
</context>
<context>
    <name>CommunicationDialog</name>
    <message>
        <location filename="../pkgui/communicationdialog.cpp" line="+64"/>
        <source>Entrez une chaîne de caractère :</source>
        <translation>Enter a character string :</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Entrez un nombre entier :</source>
        <translation>Enter a number :</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Entrez un nombre décimal :</source>
        <translation>Enter a decimal number :</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Sélectionner un choix :</source>
        <translation>Select one choice :</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Sélectionner un ou plusieurs choix :</source>
        <translation>Select one or more choices :</translation>
    </message>
</context>
<context>
    <name>DonePage</name>
    <message>
        <location filename="../pkgui/donepage.cpp" line="+68"/>
        <source>&lt;b&gt;Redémarrage de l&apos;ordinateur&lt;/b&gt;</source>
        <translation>&lt;b&gt;Reboot of the computer&lt;/b&gt;</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>Un redémarrage de l&apos;ordinateur est nécessaire pour que tous les paquets fonctionnent.</source>
        <translation>A reboot of the computer is needed for all the packages to function.</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>&lt;b&gt;Paquets orphelins&lt;/b&gt;</source>
        <translation>&lt;b&gt;Orphan packages&lt;/b&gt;</translation>
    </message>
    <message numerus="yes">
        <location line="+0"/>
        <source>%n paquet(s) ne sont plus nécessaires au fonctionnement des autres.</source>
        <translation>
            <numerusform>%n package is not needed for the others to function.</numerusform>
            <numerusform>%n packages are not needed for the others to function.</numerusform>
        </translation>
    </message>
    <message>
        <location line="+8"/>
        <source>&lt;b&gt;Installation des paquets&lt;/b&gt;</source>
        <translation>&lt;b&gt;Installation of the packages&lt;/b&gt;</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>L&apos;installation des paquets s&apos;est déroulée avec succès.</source>
        <translation>The installation of the packages was successfully executed.</translation>
    </message>
</context>
<context>
    <name>FileManyPackages</name>
    <message>
        <location filename="../libpackage/plugins/filemanypackages/filemanypackages.cpp" line="+76"/>
        <source>Le fichier %1 se trouve également dans %2</source>
        <translation>The file %1 was also found in %2</translation>
    </message>
</context>
<context>
    <name>Flags</name>
    <message>
        <location filename="../pkgui/flags.ui" line="+6"/>
        <source>Drapeaux du paquet</source>
        <translation>Flags of the package</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Les drapeaux permettent de spécifier à Logram comment gérer certains paquets. Ils vous permettent par exemple de ne jamais installer un paquet dont vous savez qu&apos;il pose problème.</source>
        <translation>The flags enable you to specify to Logram how to handle some packages. You can for example ask Logram to never install a package that you known is broken.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Drapeaux</source>
        <translation>Flags</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Intégration à l&apos;environnement :</source>
        <translation>Integration in the environment :</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Application graphique</source>
        <translation>Graphical application</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Paquet principal construit par sa source</source>
        <translation>Primary package built by its source</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Ne pas installer</source>
        <translation>Don&apos;t install</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Ne pas supprimer</source>
        <translation>Don&apos;t remove</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Ne pas mettre à jour</source>
        <translation>Don&apos;t update</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Nécessite l&apos;approbation d&apos;une licence</source>
        <translation>Needs the approval of a license</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Nécessite un redémarrage</source>
        <translation>Needs a reboot</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Installé</source>
        <translation>Installed</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Supprimé</source>
        <translation>Removed</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Conserver même si plus nécessaire</source>
        <translation>Keep even if not needed</translation>
    </message>
</context>
<context>
    <name>InstallPage</name>
    <message>
        <location filename="../pkgui/installpage.cpp" line="+138"/>
        <source>Téléchargement de %1</source>
        <translation>Downloading of %1</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Installation de %1</source>
        <translation>Installation of %1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Suppression de %1</source>
        <translation>Deletion of %1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Suppression totale de %1</source>
        <translation>Total deletion of %1</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Mise à jour de %1 vers %2</source>
        <translation>Updating of %1 to %2</translation>
    </message>
</context>
<context>
    <name>InstallWizard</name>
    <message>
        <location filename="../pkgui/branchepage.cpp" line="+69"/>
        <source>Installation de &lt;b&gt;%1&lt;/b&gt;~%2</source>
        <translation>Installation of &lt;b&gt;%1&lt;/b&gt;~%2</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Suppression de &lt;b&gt;%1&lt;/b&gt;~%2</source>
        <translation>Deletion of &lt;b&gt;%1&lt;/b&gt;~%2</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Suppression totale de &lt;b&gt;%1&lt;/b&gt;~%2</source>
        <translation>Total deletion of &lt;b&gt;%1&lt;/b&gt;~%2</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Mise à jour de &lt;b&gt;%1&lt;/b&gt;~%2 vers %3</source>
        <translation>Updating of &lt;b&gt;%1&lt;/b&gt;~%2 to %3</translation>
    </message>
    <message>
        <location line="+41"/>
        <source>Redémarrage</source>
        <translation>Reboot</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Licence à approuver</source>
        <translation>License to accept</translation>
    </message>
    <message>
        <location filename="../pkgui/installwizard.cpp" line="+60"/>
        <source>Installer et supprimer des paquets</source>
        <translation>Install and remove packages</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>« &amp;Précédant</source>
        <translation>« &amp;Previous</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&amp;Suivant »</source>
        <translation>&amp;Next »</translation>
    </message>
    <message>
        <location line="+94"/>
        <location line="+12"/>
        <location line="+41"/>
        <location line="+31"/>
        <source>Erreur lors de la résolution des dépendances</source>
        <translation>Error when solving the dependencies</translation>
    </message>
    <message>
        <location line="-84"/>
        <source>Impossible de résoudre les dépendances, mais le problème n&apos;est pas spécifié par la gestion des paquets de Logram. Veuillez rapporter le bug à l&apos;équipe de Logram en précisant ce que vous faisiez (ainsi que les paquets que vous installiez, les dépôts activés, la version de Pkgui, etc). Le message suivant pourrait les aider : «errorNode = 0»</source>
        <translation>Unable to solve the dependencies, but the problem isn&apos;t specified by the Logram&apos;s package management. Please report the bug to the Logram&apos;s staff, providing informations about what you did (and also the packages you were installing, the enabled repositories, the Pkgui&apos;s version, etc). The following message could help them : “errorNode = 0”</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Impossible de résoudre les dépendances, la structure interne de la gestion des paquets semble cassée. Veuillez rapporter le bug à l&apos;équipe de Logram en précisant ce que vous faisiez (ainsi que les paquets que vous installiez, les dépôts activés, la version de Pkgui, etc). Si vous avez les connaissances nécessaires, essayez de lancer «lpm add -G &lt;les paquets que vous installiez, ou préfixés de &quot;-&quot; si vous les supprimiez&gt;» dans une console, et postez la sortie complète de cette commande dans le rapport de bug</source>
        <translation>Unable to solve the dependencies, the internal structure of the package management is broken. Please report the bug to the Logram&apos;s staff, providing informations about what you did (and also the packages you were installing, the enabled repositories, the Pkgui&apos;s version, etc). If you have the necessary skills, please try to launch “lpm add -G &lt;the packages you were installing, or prefixed by &quot;-&quot; if you were removing them&gt;” in a console, and post the entire output of this command in your bug report</translation>
    </message>
    <message>
        <location line="+28"/>
        <source>Impossible de trouver la dépendance correspondant à %1</source>
        <translation>Unable to find the dependency matching %1</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Impossible de résoudre les dépendances, mais le problème n&apos;est pas spécifié par la gestion des paquets de Logram. Veuillez rapporter le bug à l&apos;équipe de Logram en précisant ce que vous faisiez (ainsi que les paquets que vous installiez, les dépôts activés, la version de Pkgui, etc). Le message suivant pourrait les aider : «InternalError, ps-&gt;lastError() = 0»</source>
        <translation>Unable to solve the dependencies, but the problem isn&apos;t specified by the Logram&apos;s package management. Please report the bug to the Logram&apos;s staff, providing informations about what you did (and also the packages you were installing, the enabled repositories, the Pkgui&apos;s version, etc). The following message could help them : “InternalError, ps-&gt;lastError() = 0”</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Aucun des choix nécessaires à l&apos;installation de %1 n&apos;est possible</source>
        <translation>No choice needed to install %1 is available</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Aucun des choix permettant d&apos;effectuer l&apos;opération que vous demandez n&apos;est possible</source>
        <translation>No choice allowing to perform the operation you want is possible</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Le paquet %1 devrait être installé et supprimé en même temps</source>
        <translation>The package %1 wants to be installed and removed at the same time</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Les paquets %1 et %2 devraient être installés en même temps</source>
        <translation>The packages %1 and %2 want to be installed at the same time</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Le paquet %1 ne peut être installé mais devrait être installé</source>
        <translation>The package %1 cannot be install but wants to be installed</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Le paquet %1 ne peut être supprimé mais devrait être supprimé</source>
        <translation>The package %1 cannot be removed but wants to be removed</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Le paquet %1 ne peut être mis à jour mais devrait être mis à jour</source>
        <translation>The package %1 cannot be updated but wants to be updated</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Impossible de résoudre les dépendances : %1. Si cette erreur est survenue juste après que vous ayiez choisi un choix dans la liste, essayez de cliquer sur le bouton «Remonter»</source>
        <translation>Unable to solve the dependencies : %1. If this error was raised just after you have selected a choice in the list, try to click on the button “Go up” and choose another choice</translation>
    </message>
</context>
<context>
    <name>LicensePage</name>
    <message>
        <location filename="../pkgui/licensepage.cpp" line="+74"/>
        <source>Accepter la licence</source>
        <translation>Accept the license</translation>
    </message>
</context>
<context>
    <name>Logram::DatabaseWriter</name>
    <message>
        <location filename="../libpackage/databasewriter.cpp" line="+140"/>
        <source>Impossible de créer le tampon pour la signature.</source>
        <translation>Unable to create the buffer for the signature.</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Impossible de créer le tampon pour les données.</source>
        <translation>Unable to create the buffer for the data.</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Impossible de vérifier la signature.</source>
        <translation>Unable to check the signature.</translation>
    </message>
    <message>
        <location line="+241"/>
        <source>Lecture des listes</source>
        <translation>Reading of the lists</translation>
    </message>
    <message>
        <location line="+816"/>
        <source>Génération de la liste des paquets</source>
        <translation>Generation of the list of the packages</translation>
    </message>
    <message>
        <location line="+30"/>
        <source>Enregistrement de la liste des fichiers</source>
        <translation>Writing of the list of the files</translation>
    </message>
    <message>
        <location line="+73"/>
        <source>Écriture des chaînes de caractère</source>
        <translation>Writing of the character strings</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Écriture des traductions</source>
        <translation>Writing of the translations</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Enregistrement des dépendances</source>
        <translation>Writing of the dependencies</translation>
    </message>
    <message>
        <location line="+49"/>
        <source>Enregistrement des données supplémentaires</source>
        <translation>Writing of other data</translation>
    </message>
</context>
<context>
    <name>Logram::PackageSource</name>
    <message>
        <location filename="../libpackage/packagesource.cpp" line="+474"/>
        <source>Le plugin %1 est activé pour ce paquet mais n&apos;existe pas</source>
        <translation>The plugin %1 is enabled for this package but doesn&apos;t exist</translation>
    </message>
</context>
<context>
    <name>Logram::PackageSystem</name>
    <message>
        <location filename="../libpackage/packagesystem.cpp" line="+241"/>
        <source>fr</source>
        <comment>Langue par défaut pour les paquets</comment>
        <translation>en</translation>
    </message>
</context>
<context>
    <name>Logram::ProcessThread</name>
    <message>
        <location filename="../libpackage/processthread.cpp" line="+207"/>
        <source>Erreur lors de la lecture de l&apos;archive</source>
        <translation>Error when reading the archive</translation>
    </message>
    <message>
        <location line="+35"/>
        <source>Aucun fichier dans la base de donnée LPM ne correspond à %1</source>
        <translation>No file in the LPM database matches %1</translation>
    </message>
    <message>
        <location line="+71"/>
        <source>Impossible d&apos;installer le fichier %1</source>
        <translation>Unable to install the file %1</translation>
    </message>
    <message>
        <location line="+112"/>
        <source>Impossible de lire le fichier metadata.xml du paquet</source>
        <translation>Unable to read the metadata.xml file of the package</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Paquet invalide : son premier fichier n&apos;est pas control/metadata.xml</source>
        <translation>Invalid package: its first file isn&apos;t control/metadata.xml</translation>
    </message>
</context>
<context>
    <name>Logram::RepositoryManager</name>
    <message>
        <location filename="../libpackage/repositorymanager.cpp" line="+1589"/>
        <source>Impossible de trouver la clef %1</source>
        <translation>Unable to find the key %1</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Impossible d&apos;ajouter la clef %1 pour signature</source>
        <translation>Unable to add the key %1 for the signature</translation>
    </message>
    <message>
        <location line="+333"/>
        <source>Impossible de créer le tampon mémoire pour la signature.</source>
        <translation>Unable to create the memory buffer for the signature.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Impossible de créer le tampon mémoire de sortie.</source>
        <translation>Unable to create the memory buffer for the output.</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Impossible de signer le fichier %1</source>
        <translation>Unable to sign the file %1</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Mauvais signataires</source>
        <translation>Wrong signers</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Pas de signatures dans le résultat</source>
        <translation>No signature in the result</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../pkgui/mainwindow.ui" line="+14"/>
        <source>Gestion des paquets</source>
        <translation>Package management</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Tous les paquets</source>
        <translation>All the packages</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Paquets installés</source>
        <translation>Installed packages</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Paquets non-installés</source>
        <translation>Non-installed packages</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Paquets pouvant être mis à jour</source>
        <translation>Updateable packages</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Paquets orphelins</source>
        <translation>Orphan packages</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>&amp;Rechercher</source>
        <translation>&amp;Search</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>&amp;Simplifié</source>
        <translation>&amp;Simplified</translation>
    </message>
    <message>
        <location line="+22"/>
        <location line="+416"/>
        <source>Nom</source>
        <translation>Name</translation>
    </message>
    <message>
        <location line="-411"/>
        <source>Version</source>
        <translation>Version</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Taille</source>
        <translation>Size</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Dépôt</source>
        <translation>Repository</translation>
    </message>
    <message>
        <location line="+5"/>
        <location line="+340"/>
        <source>Description</source>
        <translation>Description</translation>
    </message>
    <message>
        <location line="-322"/>
        <source>&amp;?</source>
        <translation>&amp;?</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>&amp;Paquets</source>
        <translation>&amp;Packages</translation>
    </message>
    <message>
        <location line="+37"/>
        <source>Informations de base</source>
        <translation>Base informations</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Titre</source>
        <translation>Title</translation>
    </message>
    <message>
        <location line="+57"/>
        <source>Description courte</source>
        <translation>Short description</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Section :</source>
        <translation>Section :</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Dépôt :</source>
        <translation>Repository :</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Téléchargement :</source>
        <translation>Downloading :</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Installation :</source>
        <translation>Installation :</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Site web :</source>
        <translation>Website :</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Licence :</source>
        <translation>License :</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Drapeaux :</source>
        <translation>Flags :</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>&amp;Voir et modifier</source>
        <translation>&amp;View and modify</translation>
    </message>
    <message>
        <location line="+56"/>
        <source>Versions disponibles :</source>
        <translation>Available versions :</translation>
    </message>
    <message>
        <location line="+32"/>
        <location filename="../pkgui/packageinfo.cpp" line="+144"/>
        <source>Dépendances</source>
        <translation>Dependencies</translation>
    </message>
    <message>
        <location line="+22"/>
        <source>Historique</source>
        <translation>History</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Fichiers</source>
        <translation>Files</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Attributs</source>
        <translation>Attributes</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Actions à effectuer</source>
        <translation>Pending actions</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Paquet</source>
        <translation>Package</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Nom et version du paquet dans la liste</source>
        <translation>Name and version of the package in the list</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Action</source>
        <translation>Action</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Téléchargement</source>
        <translation>Downloading</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Taille à télécharger et à installer</source>
        <translation>Amont of data to download and install</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>&amp;Appliquer</source>
        <translation>&amp;Apply</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>&amp;Vider</source>
        <translation>&amp;Clean</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Sélectionner pour &amp;installation</source>
        <translation>Select to &amp;install</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Sélectionner pour &amp;suppression</source>
        <translation>Select to &amp;remove</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>&amp;Appliquer les changements</source>
        <translation>&amp;Apply the changes</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Abandonner les &amp;changements</source>
        <translation>Cancel the &amp;changes</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>&amp;Quitter</source>
        <translation>&amp;Quit</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>À &amp;propos de PkgUi</source>
        <translation>&amp;About PkgUi</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>À propos de &amp;Qt</source>
        <translation>About &amp;Qt</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Sélectionner pour suppression &amp;totale</source>
        <translation>Select for complete &amp;removing</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>&amp;Désélectionner</source>
        <translation>&amp;Deselect</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>&amp;Mettre à jour la liste des paquets...</source>
        <translation>&amp;Update the list of the packages...</translation>
    </message>
    <message>
        <location filename="../pkgui/mainwindow.cpp" line="+213"/>
        <source>Mise à jour de la base de donnée</source>
        <translation>Updating of the database</translation>
    </message>
    <message>
        <location line="+0"/>
        <source>La base de donnée a été mise à jour avec succès.</source>
        <translation>The database was successfully updated.</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Pas d&apos;erreur ou erreur inconnue</source>
        <translation>No error or unknown error</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Impossible d&apos;ouvrir le fichier </source>
        <translation>Unable to open the file </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Impossible de mapper le fichier </source>
        <translation>Unable to map the file </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Erreur dans la commande </source>
        <translation>Error in the command </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Impossible de télécharger </source>
        <translation>Unable to download </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Erreur dans le QtScript </source>
        <translation>Error in the QtScript </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Mauvaise signature GPG du fichier </source>
        <translation>Bad GPG signature of the file </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Mauvaise somme SHA1, fichier corrompu : </source>
        <translation>Bad SHA1 sum, file corrupted: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Paquet inexistant : </source>
        <translation>Package not found:</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Mauvais type de téléchargement, vérifier sources.list : </source>
        <translation>Bad download type, please check sources.list: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Impossible d&apos;ouvrir la base de donnée : </source>
        <translation>Unable to open the database: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Erreur dans la requête : </source>
        <translation>Error in the query: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Impossible de vérifier la signature : </source>
        <translation>Unable to check the signature: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Impossible d&apos;installer le paquet </source>
        <translation>Unable to install the package </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Opération annulée : </source>
        <translation>Operation canceled: </translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Erreur</source>
        <translation>Error</translation>
    </message>
    <message>
        <location filename="../pkgui/packageinfo.cpp" line="+3"/>
        <source>Conflict</source>
        <translation>Conflict</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Fournit</source>
        <translation>Provide</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Suggère</source>
        <translation>Suggest</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Requis par</source>
        <translation>Needed by</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Remplace</source>
        <translation>Replace</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>&lt;b&gt;%1&lt;/b&gt; (par &lt;a href=&quot;mailto:%2&quot;&gt;%3&lt;/a&gt; le %4 dans %5)</source>
        <translation>&lt;b&gt;%1&lt;/b&gt; (by &lt;a href=&quot;mailto:%2&quot;&gt;%3&lt;/a&gt; on %4 in %5)</translation>
    </message>
    <message>
        <location line="+18"/>
        <source>Faible priorité</source>
        <translation>Low priority</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Fonctionnalité</source>
        <translation>Feature</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Correction de bug</source>
        <translation>Bug fix</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Sécurité</source>
        <translation>Security</translation>
    </message>
    <message>
        <location line="+83"/>
        <source>Dossier</source>
        <translation>Directory</translation>
    </message>
    <message>
        <location line="+25"/>
        <source>Installé</source>
        <translation>Installed</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Ne pas supprimer</source>
        <translation>Don&apos;t remove</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Ne pas purger</source>
        <translation>Don&apos;t purge</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Sauvegarder</source>
        <translation>Save</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Sauvegarder si modifié</source>
        <translation>Save if modified</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Écraser</source>
        <translation>Overwrite</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Virtuel</source>
        <translation>Virtual</translation>
    </message>
    <message>
        <location line="+45"/>
        <source>Pas intégré</source>
        <translation>Not integrated</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Utilisable</source>
        <translation>Useable</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Bien intégré</source>
        <translation>Well integrated</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Parfaitement intégré</source>
        <translation>Perfectly integrated</translation>
    </message>
    <message>
        <location filename="../pkgui/packageitem.cpp" line="+38"/>
        <source>Installer</source>
        <translation>Install</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Supprimer</source>
        <translation>Remove</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Supprimer totalement</source>
        <translation>Completely remove</translation>
    </message>
</context>
<context>
    <name>NoInfoDir</name>
    <message>
        <location filename="../libpackage/plugins/noinfodir/noinfodir.cpp" line="+70"/>
        <source>Le fichier /usr/share/info/dir a été trouvé alors qu&apos;il ne peut être présent. Supprimé.</source>
        <translation>The file /usr/share/info/dir was found while it cannot be present. Removed.</translation>
    </message>
</context>
<context>
    <name>ProgressDialog</name>
    <message>
        <location filename="../pkgui/progressdialog.cpp" line="+44"/>
        <source>&amp;Annuler</source>
        <translation>&amp;Cancel</translation>
    </message>
</context>
<context>
    <name>ProgressList</name>
    <message>
        <location filename="../pkgui/progresslist.cpp" line="+85"/>
        <source>Progression : </source>
        <translation>Progression: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Téléchargement de </source>
        <translation>Downloading of </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Mise à jour de la base de donnée : </source>
        <translation>Updating of the database: </translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Exécution du trigger </source>
        <translation>Executing trigger </translation>
    </message>
</context>
<context>
    <name>ShLibDeps</name>
    <message>
        <location filename="../libpackage/plugins/shlibdeps/shlibdeps.cpp" line="+271"/>
        <source>Impossible d&apos;initialiser la gestion des paquets pour trouver les dépendances automatiques</source>
        <translation>Unable to initialize the package management to find the automatic dependencies</translation>
    </message>
    <message>
        <location line="+115"/>
        <source>Impossible de trouver le paquet contenant %1</source>
        <translation>Unable to find the package containing %1</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>Ajout de %1%2%3 comme dépendance automatique</source>
        <translation>Adding of %1%2%3 as automatic dependency</translation>
    </message>
</context>
<context>
    <name>actionPage</name>
    <message>
        <location filename="../pkgui/actionpage.ui" line="+6"/>
        <source>Validation de la liste des paquets</source>
        <translation>Validation of the list of the packages</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Avant de commencer la sélection des dépendances et l&apos;installation des paquets, veuillez commencer par vérifier que la liste des modifications à apporter vous convient.</source>
        <translation>Before the beginning of the selection of the dependence and the installation of the packages, please begin by verifying that the list of the changes to apply is correct.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Liste des actions à effectuer :</source>
        <translation>List of the actions to apply :</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Nom</source>
        <translation>Name</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Version</source>
        <translation>Version</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Action</source>
        <translation>Action</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Téléchargement</source>
        <translation>Downloading</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Installation</source>
        <translation>Installation</translation>
    </message>
    <message>
        <location line="+5"/>
        <source>Description</source>
        <translation>Description</translation>
    </message>
</context>
<context>
    <name>branchePage</name>
    <message>
        <location filename="../pkgui/branchepage.ui" line="+6"/>
        <source>Sélection des dépendances</source>
        <translation>Selection of the dependencies</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Voici la liste des paquets qui devront être installés ou supprimés en plus de ceux que vous avez choisi pour garantir l&apos;intégrité du système. Il se peut que vous ayez des choix à faire.</source>
        <translation>Here is the list of the packages that will be installed or removed at the same time as those you chosen to keep the system stable. You may have choices to do.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Paquets pouvant être installés ou supprimés :</source>
        <translation>Packages that will be installed or removed :</translation>
    </message>
    <message>
        <location line="+31"/>
        <source>&amp;Remonter</source>
        <translation>&amp;Go up</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>&amp;Utiliser ce choix</source>
        <translation>&amp;Use this choice</translation>
    </message>
</context>
<context>
    <name>donePage</name>
    <message>
        <location filename="../pkgui/donepage.ui" line="+6"/>
        <source>WizardPage</source>
        <translation>WizardPage</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Installation terminée</source>
        <translation>Installation done</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>L&apos;installation des paquets est terminée ! Voici éventuellement des informations que certains paquets peuvent vous avoir laissé.</source>
        <translation>The installation of the packages is done ! Here is a possible list of the informations left by some packages.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Informations laissées par les paquets :</source>
        <translation>Informations left by the packages :</translation>
    </message>
</context>
<context>
    <name>installPage</name>
    <message>
        <location filename="../pkgui/installpage.ui" line="+6"/>
        <source>Installation</source>
        <translation>Installation</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Les paquets sont maintenant en cours d&apos;installation. Ceci peut prendre quelques secondes ou quelques minutes, suivant ce que vous avez demandé.</source>
        <translation>The packages are now being installed. This can take some seconds or some minutes, depending of what you asked.</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>%p% (paquet %v sur %m)</source>
        <translation>%p% (package %v of %m)</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>Informations détaillées</source>
        <translation>Detailed informations</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Téléchargement des paquets...</source>
        <translation>Downloading of the packages...</translation>
    </message>
</context>
<context>
    <name>licensePage</name>
    <message>
        <location filename="../pkgui/licensepage.ui" line="+6"/>
        <source>Licences des paquets</source>
        <translation>Licenses of the packages</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Les paquets suivants possèdent des licences non-libres, ou ne respectant pas celles que vous avez accepté en installant Logram. Il vous faut les accepter maintenant pour installer certains paquets.</source>
        <translation>The followinf packages have non-free licenses, or licenses not matching what you accepted by installing Logram. You have to accept them now to install some packages.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Licences à accepter :</source>
        <translation>Licenses to accept :</translation>
    </message>
</context>
</TS>
