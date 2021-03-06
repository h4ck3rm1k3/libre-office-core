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
#ifndef INCLUDED_CUI_SOURCE_INC_MULTIFIL_HXX
#define INCLUDED_CUI_SOURCE_INC_MULTIFIL_HXX

#include "multipat.hxx"

// #97807# ----------------------------------------------------
#include <ucbhelper/content.hxx>
#include <map>

// class SvxMultiFileDialog ----------------------------------------------

class SvxMultiFileDialog : public SvxMultiPathDialog
{
private:
    // #97807# -------------------------------------
    std::map< OUString, ::ucbhelper::Content >   aFileContentMap;

    DECL_LINK( AddHdl_Impl, PushButton * );
    DECL_LINK(DelHdl_Impl, void *);

public:
    SvxMultiFileDialog( Window* pParent, sal_Bool bEmptyAllowed = sal_False );
    ~SvxMultiFileDialog();

    OUString  GetFiles() const { return SvxMultiPathDialog::GetPath(); }
    void      SetFiles( const OUString& rPath ) { SvxMultiPathDialog::SetPath(rPath); aDelBtn.Enable(); }
};


#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
