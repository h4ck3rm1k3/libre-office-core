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


#include "pppoptimizer.hxx"
#include "impoptimizer.hxx"
#include <osl/file.hxx>

using namespace ::rtl;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::beans;

// ----------------
// - PPPOptimizer -
// ----------------

PPPOptimizer::PPPOptimizer(
    css::uno::Reference<css::uno::XComponentContext> const & xContext,
    css::uno::Reference< css::frame::XFrame > const & xFrame):
    mxContext( xContext ),
    mxController( xFrame->getController() )
{
}

// -----------------------------------------------------------------------------

PPPOptimizer::~PPPOptimizer()
{
}

// -----------------------------------------------------------------------------
// XDispatchProvider
// -----------------------------------------------------------------------------

Reference< com::sun::star::frame::XDispatch > SAL_CALL PPPOptimizer::queryDispatch(
    const URL& aURL, const OUString& /* aTargetFrameName */, sal_Int32 /* nSearchFlags */ ) throw( RuntimeException )
{
    Reference < XDispatch > xRet;
    if ( aURL.Protocol.equalsAscii( "vnd.com.sun.star.comp.PPPOptimizer:" ) )
    {
//      if ( aURL.Path.equalsAscii( "Function1" ) )
        xRet = this;
    }
    return xRet;
}

//------------------------------------------------------------------------------

Sequence< Reference< com::sun::star::frame::XDispatch > > SAL_CALL PPPOptimizer::queryDispatches(
    const Sequence< com::sun::star::frame::DispatchDescriptor >& aDescripts ) throw( RuntimeException )
{
    Sequence< Reference< com::sun::star::frame::XDispatch> > aReturn( aDescripts.getLength() );
    Reference< com::sun::star::frame::XDispatch>* pReturn = aReturn.getArray();
    const com::sun::star::frame::DispatchDescriptor* pDescripts = aDescripts.getConstArray();
    for (sal_Int16 i = 0; i < aDescripts.getLength(); ++i, ++pReturn, ++pDescripts )
    {
        *pReturn = queryDispatch( pDescripts->FeatureURL, pDescripts->FrameName, pDescripts->SearchFlags );
    }
    return aReturn;
}

// -----------------------------------------------------------------------------
// XDispatch
// -----------------------------------------------------------------------------

void SAL_CALL PPPOptimizer::dispatch( const URL& rURL, const Sequence< PropertyValue >& lArguments )
    throw( RuntimeException )
{
    if ( mxController.is() && rURL.Protocol.equalsAscii( "vnd.com.sun.star.comp.PPPOptimizer:" ) )
    {
        if ( rURL.Path.equalsAscii( "optimize" ) )
        {
            Reference< XModel > xModel( mxController->getModel() );
            if ( xModel.is() )
            {
                try
                {
                    ImpOptimizer aOptimizer( mxContext, xModel );
                    aOptimizer.Optimize( lArguments );
                }
                catch( Exception& )
                {
                }
            }
        }
    }
}

//===============================================
void SAL_CALL PPPOptimizer::addStatusListener( const Reference< XStatusListener >&, const URL& )
    throw( RuntimeException )
{
    // TODO
    OSL_FAIL( "PPPOptimizer::addStatusListener()\nNot implemented yet!" );
}

//===============================================
void SAL_CALL PPPOptimizer::removeStatusListener( const Reference< XStatusListener >&, const URL& )
    throw( RuntimeException )
{
    // TODO
    OSL_FAIL( "PPPOptimizer::removeStatusListener()\nNot implemented yet!" );
}

// -----------------------------------------------------------------------------
// returning filesize, on error zero is returned
sal_Int64 PPPOptimizer::GetFileSize( const OUString& rURL )
{
    sal_Int64 nFileSize = 0;
    osl::DirectoryItem aItem;
    if ( osl::DirectoryItem::get( rURL, aItem ) == osl::FileBase::E_None )
    {
        osl::FileStatus aStatus( osl_FileStatus_Mask_FileSize );
        if ( aItem.getFileStatus( aStatus ) == osl::FileBase::E_None )
        {
            nFileSize = aStatus.getFileSize();
        }
    }
    return nFileSize;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
