/*      Script de "pesage" des paquets.     */

/*  Ce fichier contient une fonction nommée "weight" qui renvoie un nombre entre 0 et MAX_INT (32-bit, 4 milliards).
    Plus ce nombre est faible, plus la liste passée en paramètre à cette fonction a de chances d'être installée.

    Ainsi, l'utilisateur se voit toujours présenter la liste qui lui convient le mieux, plusieurs
    exemplaires de ce script étant disponibles.

    Cette fonction prend une liste de Packages (voir documentation API des bibliothèques Logram). Du coté
    C++, ces listes sont déclarées «QList<QPackage *>;».

    Elle prend également un deuxième paramètre, «action», qui définit l'action principale demandée (installation/ajout,
    mise à jour sure, mise à jour complète, changement de distribution, retour à une version stable, etc).

    Ce fichier contient du script QtScript, aux normes ECMAScript. Si vous savez coder en Javascript, il n'y a
    aucun problème.
*/

var DOWNLOAD_FACTOR=20;
var INSTALL_FACTOR=15;

function weight(list, act)
{
    var w;
    w = 0;
    
    for (var i=0; i<list.length; i++)
    {
        pkg = list[i];
        
        w += (pkg.downloadSize * DOWNLOAD_FACTOR / 1048576);
        w += (pkg.installSize * INSTALL_FACTOR / 1048576);
    }
    
    return w;
}