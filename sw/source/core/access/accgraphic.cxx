/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <vcl/svapp.hxx>
#include <com/sun/star/accessibility/AccessibleRole.hpp>
#include <com/sun/star/uno/RuntimeException.hpp>
#include <comphelper/servicehelper.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <flyfrm.hxx>
#include <fmturl.hxx>
#include "accgraphic.hxx"

using namespace ::com::sun::star;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::accessibility;

SwAccessibleGraphic::SwAccessibleGraphic(
        SwAccessibleMap* pInitMap,
        const SwFlyFrm* pFlyFrm  ) :
    SwAccessibleNoTextFrame( pInitMap, AccessibleRole::GRAPHIC, pFlyFrm )
{
}

SwAccessibleGraphic::~SwAccessibleGraphic()
{
}

OUString SAL_CALL SwAccessibleGraphic::getImplementationName()
        throw( RuntimeException )
{
    return OUString("com.sun.star.comp.Writer.SwAccessibleGraphic");
}

sal_Bool SAL_CALL SwAccessibleGraphic::supportsService(const OUString& sTestServiceName)
    throw (uno::RuntimeException)
{
    return cppu::supportsService(this, sTestServiceName);
}

Sequence< OUString > SAL_CALL SwAccessibleGraphic::getSupportedServiceNames()
        throw( uno::RuntimeException )
{
    Sequence< OUString > aRet(2);
    OUString* pArray = aRet.getArray();
    pArray[0] = "com.sun.star.text.AccessibleTextGraphicObject";
    pArray[1] = OUString( sAccessibleServiceName );
    return aRet;
}

namespace
{
    class theSwAccessibleGraphicImplementationId : public rtl::Static< UnoTunnelIdInit, theSwAccessibleGraphicImplementationId > {};
}

Sequence< sal_Int8 > SAL_CALL SwAccessibleGraphic::getImplementationId()
        throw(RuntimeException)
{
    return theSwAccessibleGraphicImplementationId::get().getSeq();
}

//  Return this object's role.
sal_Int16 SAL_CALL SwAccessibleGraphic::getAccessibleRole (void)
        throw (::com::sun::star::uno::RuntimeException)
{
    SolarMutexGuard g;

    SwFmtURL aURL( ((SwLayoutFrm*)GetFrm())->GetFmt()->GetURL() );

    if (aURL.GetMap())
        return AccessibleRole::IMAGE_MAP ;
    return AccessibleRole::GRAPHIC ;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
