function weight(list)
{
    var node;
    
    for (var i=1; i<list.length; i++)
    {
        node = list[i];
        
        node.weight = 0; /* On n'utilise que les versions */
    }
}