/*      Script de "pesage" des paquets.     */

/*  Ce fichier contient une fonction weight, chargée de peser les paquets.

    Elle prend comme entrée une liste de noeuds (QList<ScriptNode *>, voir libpackage/solver_p.h).
    Chaque noeud possède un paquet, ainsi qu'éventuellement une erreur, et des enfants.
    
    Cette fonction doit placer à 1 le flag Weighted de tous les noeuds qu'elle pèse. Pour cela, 
    quelque-chose comme :
    
        node = list[0];
        node.flags = (node.flags | 2);
        
    Convient parfaitement.
    
    Plus un noeud a un poids élevé, moins il risque d'être installé.
    
    Note: Si vous souhaitez explorer tous les noeuds, sans gérer leurs enfants (ce qui est plus rapide),
          n'oubliez pas de commencer par l'index 1, car le noeud 0 est la racine, un noeud spécial.
          Son membre .package est inutilisable :
          
              for (var i=1; i<list.length; i++) { }
              
    Note: Un objet global, solver, est disponible. Vous pouvez par exemple avoir quelque-chose comme ceci :
    
              if (pkg.action == solver.Install) { }
*/

function weight(list)
{
    var node;
    var pkg;
    var w;
    
    for (var i=1; i<list.length; i++)
    {
        node = list[i];
        pkg = node.package;
        
        // On a pesé le noeud
        node.flags = (node.flags | 2);
        
        // Poids du noeud calculé en fonction de l'action du paquet
        if (pkg.action == solver.Install)
        {
            w = 20;
            
            // Plus on télécharge, au moins bien c'est (1 point par 100Kio)
            w += (pkg.downloadSize / 102400)
            
            // Plus on installe, au moins bien c'est (1 point par Mio)
            w += (pkg.installSize / (1024 * 1024))
        }
        else if (pkg.action == solver.Remove || pkg.action == solver.Purge)
        {
            w = 30;
            
            // Plus on libère de la place, au mieux c'est (Note: += car installSize < 0)
            w += (pkg.installSize / (1024 * 1024))
        }
        else
        {
            // Update
            w = 10;
            
            // Plus on télécharge, au moins bien c'est (1 point par 100Kio)
            w += (pkg.downloadSize / 102400)
        }
        
        node.weight = w;
    }
}