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
#ifndef INCLUDED_SW_INC_UNOIDXCOLL_HXX
#define INCLUDED_SW_INC_UNOIDXCOLL_HXX

#include <unocoll.hxx>

class SwXDocumentIndexes
    : public SwCollectionBaseClass
    , public SwUnoCollection
{

private:

    virtual ~SwXDocumentIndexes();

public:

    SwXDocumentIndexes(SwDoc *const pDoc);

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName()
        throw (::com::sun::star::uno::RuntimeException);
    virtual sal_Bool SAL_CALL supportsService(
            const OUString& rServiceName)
        throw (::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Sequence< OUString > SAL_CALL
        getSupportedServiceNames()
        throw (::com::sun::star::uno::RuntimeException);

    // XElementAccess
    virtual ::com::sun::star::uno::Type SAL_CALL getElementType()
        throw (::com::sun::star::uno::RuntimeException);
    virtual sal_Bool SAL_CALL hasElements()
        throw (::com::sun::star::uno::RuntimeException);

    // XIndexAccess
    virtual sal_Int32 SAL_CALL getCount()
        throw (::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Any SAL_CALL getByIndex(sal_Int32 nIndex)
        throw (::com::sun::star::lang::IndexOutOfBoundsException,
                ::com::sun::star::lang::WrappedTargetException,
                ::com::sun::star::uno::RuntimeException);

    // XNameAccess
    virtual ::com::sun::star::uno::Any SAL_CALL getByName(
            const OUString& rName)
        throw (::com::sun::star::container::NoSuchElementException,
                ::com::sun::star::lang::WrappedTargetException,
                ::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Sequence< OUString > SAL_CALL
        getElementNames() throw (::com::sun::star::uno::RuntimeException);
    virtual sal_Bool SAL_CALL hasByName(const OUString& rName)
        throw (::com::sun::star::uno::RuntimeException);

};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
