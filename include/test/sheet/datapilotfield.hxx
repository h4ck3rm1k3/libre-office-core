/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <com/sun/star/uno/Reference.hxx>
#include <test/testdllapi.hxx>

namespace apitest {

class OOO_DLLPUBLIC_TEST DataPilotField
{
public:
    virtual css::uno::Reference< css::uno::XInterface > init() = 0;

    void testSortInfo();
    void testLayoutInfo();
    void testAutoShowInfo();
    void testReference();
    void testIsGroupField();

protected:
    ~DataPilotField() {}
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
