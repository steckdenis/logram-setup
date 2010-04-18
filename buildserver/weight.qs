function weight(list)
{
    var w;
    w = 0;
    
    regex = /\D+/g
    regex.compile(regex)
    
    for (var i=0; i<list.length; i++)
    {
        pkg = list[i];
        
        // Ajouter le poids de la version (plus c'est récent, mieux c'est
        str = pkg.version
        parts = str.split(regex)
        var summ = 0;

        for (var i=0; i<parts.length; i++)
        {
            part = parts[i];

            part *= 100;

            for (var j=0; j<i; j++)
            {
                part /= 100;
            }

            summ += part;
        }

        w += summ;
    }
    
    // Moyenne
    w = w / list.length;
    
    return -w;
}