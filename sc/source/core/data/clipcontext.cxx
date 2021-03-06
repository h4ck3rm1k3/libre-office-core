/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "clipcontext.hxx"
#include "document.hxx"
#include "mtvelements.hxx"

namespace sc {

ClipContextBase::ClipContextBase(ScDocument& rDoc) :
    mpSet(new ColumnBlockPositionSet(rDoc)) {}

ClipContextBase::~ClipContextBase() {}

ColumnBlockPosition* ClipContextBase::getBlockPosition(SCTAB nTab, SCCOL nCol)
{
    return mpSet->getBlockPosition(nTab, nCol);
}

CopyFromClipContext::CopyFromClipContext(ScDocument& rDoc,
    ScDocument* pRefUndoDoc, ScDocument* pClipDoc, sal_uInt16 nInsertFlag,
    bool bAsLink, bool bSkipAttrForEmptyCells) :
    ClipContextBase(rDoc),
    mnTabStart(-1), mnTabEnd(-1),
    mpRefUndoDoc(pRefUndoDoc), mpClipDoc(pClipDoc), mnInsertFlag(nInsertFlag),
    mbAsLink(bAsLink), mbSkipAttrForEmptyCells(bSkipAttrForEmptyCells),
    mbCloneNotes (mnInsertFlag & (IDF_NOTE|IDF_ADDNOTES) )
{
}

CopyFromClipContext::~CopyFromClipContext()
{
}

void CopyFromClipContext::setTabRange(SCTAB nStart, SCTAB nEnd)
{
    mnTabStart = nStart;
    mnTabEnd = nEnd;
}

SCTAB CopyFromClipContext::getTabStart() const
{
    return mnTabStart;
}

SCTAB CopyFromClipContext::getTabEnd() const
{
    return mnTabEnd;
}

ScDocument* CopyFromClipContext::getUndoDoc()
{
    return mpRefUndoDoc;
}

ScDocument* CopyFromClipContext::getClipDoc()
{
    return mpClipDoc;
}

sal_uInt16 CopyFromClipContext::getInsertFlag() const
{
    return mnInsertFlag;
}

bool CopyFromClipContext::isAsLink() const
{
    return mbAsLink;
}

bool CopyFromClipContext::isSkipAttrForEmptyCells() const
{
    return mbSkipAttrForEmptyCells;
}

bool CopyFromClipContext::isCloneNotes() const
{
    return mbCloneNotes;
}

CopyToClipContext::CopyToClipContext(
    ScDocument& rDoc, bool bKeepScenarioFlags, bool bCloneNotes) :
    ClipContextBase(rDoc), mbKeepScenarioFlags(bKeepScenarioFlags), mbCloneNotes(bCloneNotes) {}

CopyToClipContext::~CopyToClipContext() {}

bool CopyToClipContext::isKeepScenarioFlags() const
{
    return mbKeepScenarioFlags;
}

bool CopyToClipContext::isCloneNotes() const
{
    return mbCloneNotes;
}

CopyToDocContext::CopyToDocContext(ScDocument& rDoc) : ClipContextBase(rDoc) {}
CopyToDocContext::~CopyToDocContext() {}

MixDocContext::MixDocContext(ScDocument& rDoc) : ClipContextBase(rDoc) {}
MixDocContext::~MixDocContext() {}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
