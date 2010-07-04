<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0" language="en_US">
<defaultcodec>UTF-8</defaultcodec>
<context>
    <name>App</name>
    <message>
        <location filename="../setup/app.cpp" line="+459"/>
        <source>Utilisation : setup [options] &lt;action&gt; [arguments]
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
                       de vérification des paquets (setup binaries).
    -C                 Affiche l&apos;historique des modifications d&apos;un paquet
                       quand utilisé avec showpkg.
    -L                 Affiche la licence d&apos;un paquet quand utilisé avec showpkg.
    -T                 Désactive l&apos;exécution des triggers.
    -W                 Désactive les couleurs dans la sortie de Setup.
</source>
        <translation>Usage: setup [options] &lt;action&gt; [arguments]
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
    -W                 Disable the colored output of Setup.
</translation>
    </message>
    <message>
        <location line="+65"/>
        <source>Logram Setup </source>
        <translation>Logram Setup </translation>
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
        <location filename="../setup/package.cpp" line="+67"/>
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
        <location filename="../setup/cache.cpp" line="+151"/>
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
        <location line="+40"/>
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
        <location filename="../setup/package.cpp" line="+30"/>
        <source>Légende : </source>
        <translation>Legend: </translation>
    </message>
    <message>
        <location line="+1"/>
        <location filename="../setup/package.cpp" line="+1"/>
        <source>D: Dépend </source>
        <translation>D: Depend </translation>
    </message>
    <message>
        <location line="+1"/>
        <location filename="../setup/package.cpp" line="+1"/>
        <source>S: Suggère </source>
        <translation>S: Suggest </translation>
    </message>
    <message>
        <location line="+1"/>
        <location filename="../setup/package.cpp" line="+1"/>
        <source>C: Conflit </source>
        <translation>C: Conflict </translation>
    </message>
    <message>
        <location line="+1"/>
        <location filename="../setup/package.cpp" line="+1"/>
        <source>P: Fourni </source>
        <translation>P: Provide </translation>
    </message>
    <message>
        <location line="+1"/>
        <location filename="../setup/package.cpp" line="+1"/>
        <source>R: Remplace </source>
        <translation>R: Replace </translation>
    </message>
    <message>
        <location line="+1"/>
        <location filename="../setup/package.cpp" line="+1"/>
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
        <location filename="../setup/communication.cpp" line="+45"/>
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
        <location filename="../setup/package.cpp" line="-69"/>
        <location line="+803"/>
        <source>Bug dans Setup !</source>
        <translation>Bug in Setup !</translation>
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
        <source>Erreur inconnue. Ajoutez le paramètre -G à Setup et postez sa sortie dans votre rapport de bug</source>
        <translation>Unknown error. Add the -G parameter to Setup and post its output in your bug report please</translation>
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
        <source>Pour une suppression totale de %1. Utilisez setup autoremove pour supprimer ces paquets.</source>
        <translation>For a total removal of %1. Use setup autoremove to remove these packages.</translation>
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
        <location filename="../setup/source.cpp" line="+87"/>
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
    <name>CheckFiles</name>
    <message>
        <location filename="../libpackage/plugins/checkfiles/checkfiles.cpp" line="+64"/>
        <source>Le fichier %1 ne se trouve dans aucun paquet</source>
        <translation>The file %1 was not found in any package</translation>
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
        <source>Aucun fichier dans la base de donnée Setup ne correspond à %1</source>
        <translation>No file in the Setup database matches %1</translation>
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
        <location filename="../libpackage/repositorymanager.cpp" line="+1547"/>
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
    <name>NoInfoDir</name>
    <message>
        <location filename="../libpackage/plugins/noinfodir/noinfodir.cpp" line="+70"/>
        <source>Le fichier /usr/share/info/dir a été trouvé alors qu&apos;il ne peut être présent. Supprimé.</source>
        <translation>The file /usr/share/info/dir was found while it cannot be present. Removed.</translation>
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
</TS>
