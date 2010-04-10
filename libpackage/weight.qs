/*      Script de "pesage" des paquets.     */

/*  Ce fichier contient une fonction nommée "weight" qui renvoie un nombre entre 0 et MAX_INT (32-bit, 4 milliards).
    Plus ce nombre est faible, plus la liste passée en paramètre à cette fonction a de chances d'être installée.

    Ainsi, l'utilisateur se voit toujours présenter la liste qui lui convient le mieux, plusieurs
    exemplaires de ce script étant disponibles.

    Cette fonction prend une liste de Packages (voir documentation API des bibliothèques Logram). Du coté
    C++, ces listes sont déclarées «QList<QPackage *>;».

    Ce fichier contient du script QtScript, aux normes ECMAScript. Si vous savez coder en Javascript, il n'y a
    aucun problème.
*/

function weight(list)
{
    var w;
    w = 0;
    
    for (var i=0; i<list.length; i++)
    {
        pkg = list[i];
        
        if (w.action == 1)
        {
            // Installation
            w += 1
        }
        else if (w.action == 2 || w.action == 3)
        {
            // Suppression
            w += 2
        }
        else
        {
            // Mise à jour
            w += 1
        }
    }
    
    return w;
}