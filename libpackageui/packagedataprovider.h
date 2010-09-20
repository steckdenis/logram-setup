/*
 * packagedataprovider.h
 * This file is part of Logram
 *
 * Copyright (C) 2010 - Denis Steckelmacher <steckdenis@logram-project.org>
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

#ifndef __PACKAGEDATAPROVIDER_H__
#define __PACKAGEDATAPROVIDER_H__

/**
 * @file packagedataprovider.h
 * @brief Fournisseur d'informations pour les paquets LPM
 */

#include "packagedataproviderinterface.h"

namespace Logram
{
    class PackageMetaData;
    class PackageSystem;
}

namespace LogramUi
{

/**
 * @brief Fournisseur d'informations sur les paquets LPM
 * 
 * La classe PackageDataProviderInterface est virtuelle pure et ne sert que
 * d'interface. Sachant qu'il est très courant d'afficher des informations sur
 * les paquets en provenance dLogram::Package::Flag flags de donnée LPM, cette classe fournit
 * l'implémentation remplissant ce rôle.
 * 
 * @sa PackageDataProviderInterface
 */
class PackageDataProvider : public PackageDataProviderInterface
{
    public:
        /**
         * @brief Constructeur
         * @param package Paquet pour lequel afficher des informations
         * @param _ps PackageSystem courant
         */
        PackageDataProvider(Logram::Package *package, Logram::PackageSystem *_ps);
        ~PackageDataProvider();
        
        virtual QString name() const;
        virtual QString version() const;
        
        virtual int flags() const;
        virtual void setFlags(Logram::Package::Flag flags);
        virtual bool flagsEditable() const;
        
        virtual QString website() const;
        virtual QString title() const;
        virtual QString shortDesc() const;
        virtual QString longDesc() const;
        virtual QString license() const;
        
        virtual QByteArray iconData() const;
        
        virtual QString repository() const;
        virtual QString distribution() const;
        virtual QString section() const;
        
        virtual int downloadSize() const;
        virtual int installSize() const;
        
        virtual QVector<PackageDataProviderInterface *> versions() const;
        virtual QVector<Logram::Depend *> depends() const;
        virtual QVector<Logram::ChangeLogEntry *> changelog() const;
        virtual QVector<Logram::PackageFile *> files() const;
        
    private:
        Logram::Package *pkg;
        Logram::PackageMetaData *md;
        Logram::PackageSystem *ps;
};

}

#endif