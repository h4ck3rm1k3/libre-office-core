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
#ifndef INCLUDED_CUI_SOURCE_INC_CFG_HXX
#define INCLUDED_CUI_SOURCE_INC_CFG_HXX

#include <vcl/fixed.hxx>
#include <vcl/group.hxx>
#include <vcl/layout.hxx>
#include <vcl/lstbox.hxx>
#include <vcl/menubtn.hxx>
#include <vcl/toolbox.hxx>
#include <svtools/treelistbox.hxx>
#include <svtools/svmedit2.hxx>
#include <svtools/svmedit.hxx>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/container/XIndexContainer.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/frame/XModuleManager.hpp>
#include <com/sun/star/ui/XUIConfigurationListener.hpp>
#include <com/sun/star/ui/XUIConfigurationManager.hpp>
#include <com/sun/star/ui/XImageManager.hpp>
#include <com/sun/star/graphic/XGraphicProvider.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/lang/XSingleComponentFactory.hpp>

#include <sfx2/tabdlg.hxx>
#include <vector>
#include <vcl/msgbox.hxx>

#include "selector.hxx"

class SvxConfigEntry;
class SvxConfigPage;
class SvxMenuConfigPage;
class SvxToolbarConfigPage;

typedef std::vector< SvxConfigEntry* > SvxEntries;

class SvxConfigDialog : public SfxTabDialog
{
private:
    ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame > m_xFrame;

public:
    SvxConfigDialog( Window*, const SfxItemSet* );
    ~SvxConfigDialog();

    virtual void                PageCreated( sal_uInt16 nId, SfxTabPage &rPage );
    virtual short               Ok();

    void SetFrame(const ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& xFrame);
};

class SaveInData : public ImageProvider
{
private:

    bool        bModified;

    bool        bDocConfig;
    bool        bReadOnly;

    ::com::sun::star::uno::Reference
        < com::sun::star::ui::XUIConfigurationManager > m_xCfgMgr;

    ::com::sun::star::uno::Reference
        < com::sun::star::ui::XUIConfigurationManager > m_xParentCfgMgr;

    ::com::sun::star::uno::Reference
        < com::sun::star::ui::XImageManager > m_xImgMgr;

    ::com::sun::star::uno::Reference
        < com::sun::star::ui::XImageManager > m_xParentImgMgr;

    static ::com::sun::star::uno::Reference
        < com::sun::star::ui::XImageManager >* xDefaultImgMgr;

public:

    SaveInData(
        const ::com::sun::star::uno::Reference <
            ::com::sun::star::ui::XUIConfigurationManager >& xCfgMgr,
        const ::com::sun::star::uno::Reference <
            ::com::sun::star::ui::XUIConfigurationManager >& xParentCfgMgr,
        const OUString& aModuleId,
        bool docConfig );

    ~SaveInData() {}

    bool PersistChanges(
        const com::sun::star::uno::Reference
            < com::sun::star::uno::XInterface >& xManager );

    void SetModified( bool bValue = sal_True ) { bModified = bValue; }
    bool IsModified( ) { return bModified; }

    bool IsReadOnly( ) { return bReadOnly; }
    bool IsDocConfig( ) { return bDocConfig; }

    ::com::sun::star::uno::Reference
        < ::com::sun::star::ui::XUIConfigurationManager >
            GetConfigManager() { return m_xCfgMgr; };

    ::com::sun::star::uno::Reference
        < ::com::sun::star::ui::XUIConfigurationManager >
            GetParentConfigManager() { return m_xParentCfgMgr; };

    ::com::sun::star::uno::Reference
        < ::com::sun::star::ui::XImageManager >
            GetImageManager() { return m_xImgMgr; };

    ::com::sun::star::uno::Reference
        < ::com::sun::star::ui::XImageManager >
            GetParentImageManager() { return m_xParentImgMgr; };

    ::com::sun::star::uno::Reference
        < com::sun::star::container::XNameAccess > m_xCommandToLabelMap;

    com::sun::star::uno::Sequence
        < com::sun::star::beans::PropertyValue > m_aSeparatorSeq;

    Image GetImage( const OUString& rCommandURL );

    virtual bool HasURL( const OUString& aURL ) = 0;
    virtual bool HasSettings() = 0;
    virtual SvxEntries* GetEntries() = 0;
    virtual void SetEntries( SvxEntries* ) = 0;
    virtual void Reset() = 0;
    virtual bool Apply() = 0;
};

class MenuSaveInData : public SaveInData
{
private:

    OUString               m_aMenuResourceURL;
    OUString               m_aDescriptorContainer;

    ::com::sun::star::uno::Reference
        < com::sun::star::container::XIndexAccess > m_xMenuSettings;

    SvxConfigEntry* pRootEntry;


    static MenuSaveInData* pDefaultData;    ///< static holder of the default menu data

    static void SetDefaultData( MenuSaveInData* pData ) {pDefaultData = pData;}
    static MenuSaveInData* GetDefaultData() { return pDefaultData; }

    void        Apply( bool bDefault );

    void        Apply(
        SvxConfigEntry* pRootEntry,
        com::sun::star::uno::Reference<
            com::sun::star::container::XIndexContainer >& rNewMenuBar,
        com::sun::star::uno::Reference<
            com::sun::star::lang::XSingleComponentFactory >& rFactory,
        SvTreeListEntry *pParent = NULL );

    void        ApplyMenu(
        com::sun::star::uno::Reference<
            com::sun::star::container::XIndexContainer >& rNewMenuBar,
        com::sun::star::uno::Reference<
            com::sun::star::lang::XSingleComponentFactory >& rFactory,
        SvxConfigEntry *pMenuData = NULL );

    bool        LoadSubMenus(
        const ::com::sun::star::uno::Reference<
            com::sun::star::container::XIndexAccess >& xMenuBarSettings,
        const OUString& rBaseTitle, SvxConfigEntry* pParentData );

public:

    MenuSaveInData(
        const ::com::sun::star::uno::Reference <
            ::com::sun::star::ui::XUIConfigurationManager >&,
        const ::com::sun::star::uno::Reference <
            ::com::sun::star::ui::XUIConfigurationManager >&,
        const OUString& aModuleId,
        bool docConfig );

    ~MenuSaveInData();

    /// methods inherited from SaveInData
    SvxEntries*         GetEntries();
    void                SetEntries( SvxEntries* );
    bool                HasURL( const OUString& URL ) { (void)URL; return sal_False; }
    bool                HasSettings() { return m_xMenuSettings.is(); }
    void                Reset();
    bool                Apply();
};

class SvxConfigEntry
{
private:

    /// common properties
    sal_uInt16                      nId;
    OUString             aHelpText;
    OUString             aLabel;
    OUString             aCommand;
    OUString             aHelpURL;

    bool                        bPopUp;
    bool                        bStrEdited;
    bool                        bIsUserDefined;
    bool                        bIsMain;
    bool                        bIsParentData;

    /// toolbar specific properties
    bool                        bIsVisible;
    sal_Int32                   nStyle;

    ::com::sun::star::uno::Reference<
        ::com::sun::star::graphic::XGraphic > xBackupGraphic;

    SvxEntries                  *pEntries;

public:

    SvxConfigEntry( const OUString& rDisplayName,
                    const OUString& rCommandURL,
                    bool bPopup = sal_False,
                    bool bParentData = sal_False );

    SvxConfigEntry()
        :
            nId( 0 ),
            bPopUp( sal_False ),
            bStrEdited( sal_False ),
            bIsUserDefined( sal_False ),
            bIsMain( sal_False ),
            bIsParentData( sal_False ),
            bIsVisible( sal_True ),
            nStyle( 0 ),
            pEntries( 0 )
    {}

    ~SvxConfigEntry();

    const OUString&      GetCommand() const { return aCommand; }
    void    SetCommand( const OUString& rCmd ) { aCommand = rCmd; }

    const OUString&      GetName() const { return aLabel; }
    void    SetName( const OUString& rStr ) { aLabel = rStr; bStrEdited = sal_True; }
    bool    HasChangedName() const { return bStrEdited; }

    const OUString&      GetHelpText() ;
    void    SetHelpText( const OUString& rStr ) { aHelpText = rStr; }

    const OUString&      GetHelpURL() const { return aHelpURL; }
    void    SetHelpURL( const OUString& rStr ) { aHelpURL = rStr; }

    void    SetPopup( bool bOn = sal_True ) { bPopUp = bOn; }
    bool    IsPopup() const { return bPopUp; }

    void    SetUserDefined( bool bOn = sal_True ) { bIsUserDefined = bOn; }
    bool    IsUserDefined() const { return bIsUserDefined; }

    bool    IsBinding() const { return !bPopUp; }
    bool    IsSeparator() const { return nId == 0; }

    SvxEntries* GetEntries() const { return pEntries; }
    void    SetEntries( SvxEntries* entries ) { pEntries = entries; }
    bool    HasEntries() const { return pEntries != NULL; }

    void    SetMain( bool bValue = sal_True ) { bIsMain = bValue; }
    bool    IsMain() { return bIsMain; }

    void    SetParentData( bool bValue = sal_True ) { bIsParentData = bValue; }
    bool    IsParentData() { return bIsParentData; }

    bool    IsMovable();
    bool    IsDeletable();
    bool    IsRenamable();

    void    SetVisible( bool b ) { bIsVisible = b; }
    bool    IsVisible() const { return bIsVisible; }

    void    SetBackupGraphic(
        ::com::sun::star::uno::Reference<
            ::com::sun::star::graphic::XGraphic > graphic )
                { xBackupGraphic = graphic; }

    ::com::sun::star::uno::Reference<
        ::com::sun::star::graphic::XGraphic >
            GetBackupGraphic()
                { return xBackupGraphic; }

    bool    IsIconModified() { return xBackupGraphic.is(); }

    sal_Int32   GetStyle() { return nStyle; }
    void        SetStyle( sal_Int32 style ) { nStyle = style; }
};

class SvxMenuEntriesListBox : public SvTreeListBox
{
private:
    SvxConfigPage*      pPage;

protected:
    bool                m_bIsInternalDrag;

public:
    SvxMenuEntriesListBox( Window*, const ResId& );
    ~SvxMenuEntriesListBox();

    virtual sal_Int8    AcceptDrop( const AcceptDropEvent& rEvt );

    virtual sal_Bool        NotifyAcceptDrop( SvTreeListEntry* pEntry );

    virtual sal_Bool        NotifyMoving( SvTreeListEntry*, SvTreeListEntry*,
                                      SvTreeListEntry*&, sal_uLong& );

    virtual sal_Bool        NotifyCopying( SvTreeListEntry*, SvTreeListEntry*,
                                       SvTreeListEntry*&, sal_uLong&);

    virtual DragDropMode    NotifyStartDrag(
        TransferDataContainer&, SvTreeListEntry* );

    virtual void        DragFinished( sal_Int8 );

    void                KeyInput( const KeyEvent& rKeyEvent );
};

class SvxDescriptionEdit : public ExtMultiLineEdit
{
private:
    Rectangle           m_aRealRect;

public:
    SvxDescriptionEdit( Window* pParent, const ResId& _rId );
    inline ~SvxDescriptionEdit() {}

    void                SetNewText( const OUString& _rText );
    inline void         Clear() { SetNewText( OUString() ); }
};

class SvxConfigPage : public SfxTabPage
{
private:

    bool                                bInitialised;
    SaveInData*                         pCurrentSaveInData;

    DECL_LINK(  SelectSaveInLocation, ListBox * );
    DECL_LINK(  AsyncInfoMsg, OUString* );

    bool        SwapEntryData( SvTreeListEntry* pSourceEntry, SvTreeListEntry* pTargetEntry );
    void        AlignControls();

protected:

    // the top section of the tab page where top level menus and toolbars
    //  are displayed in a listbox
    FixedLine                           aTopLevelSeparator;
    FixedText                           aTopLevelLabel;
    ListBox                             aTopLevelListBox;
    PushButton                          aNewTopLevelButton;
    MenuButton                          aModifyTopLevelButton;

    // the contents section where the contents of the selected
    // menu or toolbar are displayed
    FixedLine                           aContentsSeparator;
    FixedText                           aContentsLabel;
    SvTreeListBox*                      aContentsListBox;

    PushButton                          aAddCommandsButton;
    MenuButton                          aModifyCommandButton;

    ImageButton                         aMoveUpButton;
    ImageButton                         aMoveDownButton;

    FixedText                           aSaveInText;
    ListBox                             aSaveInListBox;

    FixedText                           aDescriptionLabel;
    SvxDescriptionEdit                  aDescriptionField;

    SvxScriptSelectorDialog*            pSelectorDlg;

    /// the ResourceURL to select when opening the dialog
    OUString                       m_aURLToSelect;

    ::com::sun::star::uno::Reference
        < ::com::sun::star::frame::XFrame > m_xFrame;

    SvxConfigPage( Window*, const SfxItemSet& );
    virtual ~SvxConfigPage();

    DECL_LINK( MoveHdl, Button * );

    virtual SaveInData* CreateSaveInData(
        const ::com::sun::star::uno::Reference <
            ::com::sun::star::ui::XUIConfigurationManager >&,
        const ::com::sun::star::uno::Reference <
            ::com::sun::star::ui::XUIConfigurationManager >&,
        const OUString& aModuleId,
        bool docConfig ) = 0;

    virtual void            Init() = 0;
    virtual void            UpdateButtonStates() = 0;
    virtual short           QueryReset() = 0;

    void            PositionContentsListBox();

    SvTreeListEntry*    InsertEntry(        SvxConfigEntry* pNewEntryData,
                                        SvTreeListEntry* pTarget = NULL,
                                        bool bFront = sal_False );

    void            AddSubMenusToUI(    const OUString& rBaseTitle,
                                        SvxConfigEntry* pParentData );

    SvTreeListEntry*    InsertEntryIntoUI ( SvxConfigEntry* pNewEntryData,
                                        sal_uLong nPos = LIST_APPEND );

    SvxEntries*     FindParentForChild( SvxEntries* pParentEntries,
                                        SvxConfigEntry* pChildData );

    void            ReloadTopLevelListBox( SvxConfigEntry* pSelection = NULL );

public:

    static bool     CanConfig( const OUString& rModuleId );

    SaveInData*     GetSaveInData() { return pCurrentSaveInData; }

    SvTreeListEntry*    AddFunction( SvTreeListEntry* pTarget = NULL,
                                 bool bFront = sal_False,
                                 bool bAllowDuplicates = sal_False );

    virtual void    MoveEntry( bool bMoveUp );

    bool            MoveEntryData(  SvTreeListEntry* pSourceEntry,
                                    SvTreeListEntry* pTargetEntry );

    sal_Bool            FillItemSet( SfxItemSet& );
    void            Reset( const SfxItemSet& );

    virtual bool    DeleteSelectedContent() = 0;
    virtual void    DeleteSelectedTopLevel() = 0;

    SvxConfigEntry* GetTopLevelSelection()
    {
        return (SvxConfigEntry*) aTopLevelListBox.GetEntryData(
            aTopLevelListBox.GetSelectEntryPos() );
    }

    /** identifies the module in the given frame. If the frame is <NULL/>, a default
        frame will be determined beforehand.

        If the given frame is <NULL/>, a default frame will be used: The method the active
        frame of the desktop, then the current frame. If both are <NULL/>,
        the SfxViewFrame::Current's XFrame is used. If this is <NULL/>, too, an empty string is returned.

        If the given frame is not <NULL/>, or an default frame could be successfully determined, then
        the ModuleManager is asked for the module ID of the component in the frame.
    */
    static OUString
        GetFrameWithDefaultAndIdentify( ::com::sun::star::uno::Reference< ::com::sun::star::frame::XFrame >& _inout_rxFrame );
};

class SvxMenuConfigPage : public SvxConfigPage
{
private:

    DECL_LINK( SelectMenu, ListBox * );
    DECL_LINK( SelectMenuEntry, Control * );
    DECL_LINK( NewMenuHdl, Button * );
    DECL_LINK( MenuSelectHdl, MenuButton * );
    DECL_LINK( EntrySelectHdl, MenuButton * );
    DECL_LINK( AddCommandsHdl, Button * );
    DECL_LINK( AddFunctionHdl, SvxScriptSelectorDialog * );

    void            Init();
    void            UpdateButtonStates();
    short           QueryReset();
    bool            DeleteSelectedContent();
    void            DeleteSelectedTopLevel();

public:
    SvxMenuConfigPage( Window *pParent, const SfxItemSet& rItemSet );
    ~SvxMenuConfigPage();

    SaveInData* CreateSaveInData(
        const ::com::sun::star::uno::Reference <
            ::com::sun::star::ui::XUIConfigurationManager >&,
        const ::com::sun::star::uno::Reference <
            ::com::sun::star::ui::XUIConfigurationManager >&,
        const OUString& aModuleId,
        bool docConfig );
};

class SvxMainMenuOrganizerDialog : public ModalDialog
{
    VclContainer*   m_pMenuBox;
    Edit*           m_pMenuNameEdit;
    SvTreeListBox*  m_pMenuListBox;
    PushButton*     m_pMoveUpButton;
    PushButton*     m_pMoveDownButton;

    SvxEntries*     pEntries;
    SvTreeListEntry*    pNewMenuEntry;
    bool            bModified;

    void UpdateButtonStates();

    DECL_LINK( MoveHdl, Button * );
    DECL_LINK( ModifyHdl, Edit * );
    DECL_LINK( SelectHdl, Control* );

public:
    SvxMainMenuOrganizerDialog (
        Window*, SvxEntries*,
        SvxConfigEntry*, bool bCreateMenu = sal_False );

    ~SvxMainMenuOrganizerDialog ();

    SvxEntries*     GetEntries();
    void            SetEntries( SvxEntries* );
    SvxConfigEntry* GetSelectedEntry();
};

class SvxToolbarEntriesListBox : public SvxMenuEntriesListBox
{
    Size            m_aCheckBoxImageSizePixel;
    Link            m_aChangedListener;
    SvLBoxButtonData*   m_pButtonData;
    SvxConfigPage*  pPage;

    void            ChangeVisibility( SvTreeListEntry* pEntry );

protected:

    virtual void    CheckButtonHdl();
    virtual void    DataChanged( const DataChangedEvent& rDCEvt );
    void            BuildCheckBoxButtonImages( SvLBoxButtonData* );
    Image           GetSizedImage(
        VirtualDevice& aDev, const Size& aNewSize, const Image& aImage );

public:

    SvxToolbarEntriesListBox(
        Window* pParent, const ResId& );

    ~SvxToolbarEntriesListBox();

    void            SetChangedListener( const Link& aChangedListener )
        { m_aChangedListener = aChangedListener; }

    const Link&     GetChangedListener() const { return m_aChangedListener; }

    Size            GetCheckBoxPixelSize() const
        { return m_aCheckBoxImageSizePixel; }

    virtual sal_Bool    NotifyMoving(
        SvTreeListEntry*, SvTreeListEntry*, SvTreeListEntry*&, sal_uLong& );

    virtual sal_Bool    NotifyCopying(
        SvTreeListEntry*, SvTreeListEntry*, SvTreeListEntry*&, sal_uLong&);

    void            KeyInput( const KeyEvent& rKeyEvent );
};

class SvxToolbarConfigPage : public SvxConfigPage
{
private:

    DECL_LINK( SelectToolbar, ListBox * );
    DECL_LINK( SelectToolbarEntry, Control * );
    DECL_LINK( ToolbarSelectHdl, MenuButton * );
    DECL_LINK( EntrySelectHdl, MenuButton * );
    DECL_LINK( NewToolbarHdl, Button * );
    DECL_LINK( AddCommandsHdl, Button * );
    DECL_LINK( AddFunctionHdl, SvxScriptSelectorDialog * );
    DECL_LINK( MoveHdl, Button * );

    void            UpdateButtonStates();
    short           QueryReset();
    void            Init();
    bool            DeleteSelectedContent();
    void            DeleteSelectedTopLevel();

public:
    SvxToolbarConfigPage( Window *pParent, const SfxItemSet& rItemSet );
    ~SvxToolbarConfigPage();

    SvTreeListEntry*    AddFunction( SvTreeListEntry* pTarget = NULL,
                                             bool bFront = sal_False,
                                             bool bAllowDuplicates = sal_True );

    void            MoveEntry( bool bMoveUp );

    SaveInData*     CreateSaveInData(
        const ::com::sun::star::uno::Reference <
            ::com::sun::star::ui::XUIConfigurationManager >&,
        const ::com::sun::star::uno::Reference <
            ::com::sun::star::ui::XUIConfigurationManager >&,
        const OUString& aModuleId,
        bool docConfig );
};

class ToolbarSaveInData : public SaveInData
{
private:

    SvxConfigEntry*                                pRootEntry;
    OUString                                  m_aDescriptorContainer;

    ::com::sun::star::uno::Reference
        < com::sun::star::container::XNameAccess > m_xPersistentWindowState;

    bool        LoadToolbar(
        const ::com::sun::star::uno::Reference<
            com::sun::star::container::XIndexAccess >& xToolBarSettings,
        SvxConfigEntry* pParentData );

    void        ApplyToolbar(
        com::sun::star::uno::Reference<
            com::sun::star::container::XIndexContainer >& rNewToolbarBar,
        com::sun::star::uno::Reference<
            com::sun::star::lang::XSingleComponentFactory >& rFactory,
        SvxConfigEntry *pToolbar = NULL );

public:

    ToolbarSaveInData(
        const ::com::sun::star::uno::Reference <
            ::com::sun::star::ui::XUIConfigurationManager >&,
            const ::com::sun::star::uno::Reference <
            ::com::sun::star::ui::XUIConfigurationManager >&,
        const OUString& aModuleId,
        bool docConfig );

    ~ToolbarSaveInData();

    void            CreateToolbar( SvxConfigEntry* pToolbar );
    void            RestoreToolbar( SvxConfigEntry* pToolbar );
    void            RemoveToolbar( SvxConfigEntry* pToolbar );
    void            ApplyToolbar( SvxConfigEntry* pToolbar );

    OUString   GetSystemUIName( const OUString& rResourceURL );

    sal_Int32       GetSystemStyle( const OUString& rResourceURL );

    void            SetSystemStyle(
        const OUString& rResourceURL, sal_Int32 nStyle );

    void            SetSystemStyle(
        ::com::sun::star::uno::Reference
            < ::com::sun::star::frame::XFrame > xFrame,
        const OUString& rResourceURL, sal_Int32 nStyle );

    SvxEntries*     GetEntries();
    void            SetEntries( SvxEntries* );
    bool            HasSettings();
    bool            HasURL( const OUString& rURL );
    void            Reset();
    bool            Apply();
};

class SvxNewToolbarDialog : public ModalDialog
{
private:
    Edit*           m_pEdtName;
    OKButton*       m_pBtnOK;

    Link            aCheckNameHdl;

    DECL_LINK(ModifyHdl, Edit*);

public:
    SvxNewToolbarDialog(Window* pWindow, const OUString& rName);

    ListBox*        m_pSaveInListBox;

    OUString GetName()
    {
        return m_pEdtName->GetText();
    }

    void SetCheckNameHdl( const Link& rLink, bool bCheckImmediately = false )
    {
        aCheckNameHdl = rLink;
        if ( bCheckImmediately )
            m_pBtnOK->Enable( rLink.Call( this ) > 0 );
    }

    void SetEditHelpId( const OString& aHelpId)
    {
        m_pEdtName->SetHelpId(aHelpId);
    }
};

class SvxIconSelectorDialog : public ModalDialog
{
private:
    FixedText       aFtDescription;
    ToolBox         aTbSymbol;
    FixedText       aFtNote;
    OKButton        aBtnOK;
    CancelButton    aBtnCancel;
    HelpButton      aBtnHelp;
    PushButton      aBtnImport;
    PushButton      aBtnDelete;
    FixedLine       aFlSeparator;
    sal_uInt16      m_nNextId;

    sal_Int32       m_nExpectedSize;

    ::com::sun::star::uno::Reference<
        ::com::sun::star::ui::XImageManager > m_xImageManager;

    ::com::sun::star::uno::Reference<
        ::com::sun::star::ui::XImageManager > m_xParentImageManager;

    ::com::sun::star::uno::Reference<
        ::com::sun::star::ui::XImageManager > m_xImportedImageManager;

    ::com::sun::star::uno::Reference<
        ::com::sun::star::graphic::XGraphicProvider > m_xGraphProvider;

    bool ReplaceGraphicItem( const OUString& aURL );

    bool ImportGraphic( const OUString& aURL );

    void ImportGraphics(
        const com::sun::star::uno::Sequence< OUString >& aURLs );

public:

    SvxIconSelectorDialog(
        Window *pWindow,
        const ::com::sun::star::uno::Reference<
            ::com::sun::star::ui::XImageManager >& rXImageManager,
        const ::com::sun::star::uno::Reference<
            ::com::sun::star::ui::XImageManager >& rXParentImageManager
            );

    ~SvxIconSelectorDialog();

    ::com::sun::star::uno::Reference< ::com::sun::star::graphic::XGraphic >
        GetSelectedIcon();

    DECL_LINK( SelectHdl, ToolBox * );
    DECL_LINK( ImportHdl, PushButton * );
    DECL_LINK( DeleteHdl, PushButton * );
};

class SvxIconReplacementDialog : public MessBox
{
public:
    SvxIconReplacementDialog(
        Window *pWindow,
        const OUString& aMessage,
        bool aYestoAll);

    SvxIconReplacementDialog(
        Window *pWindow,
        const OUString& aMessage );

    OUString ReplaceIconName( const OUString& );
    sal_uInt16 ShowDialog();
};
//added for issue83555
class SvxIconChangeDialog : public ModalDialog
{
private:
    FixedImage      aFImageInfo;
    OKButton        aBtnOK;
    FixedText         aDescriptionLabel;
    SvxDescriptionEdit aLineEditDescription;
public:
    SvxIconChangeDialog(Window *pWindow, const OUString& aMessage);
};
#endif // INCLUDED_CUI_SOURCE_INC_CFG_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
