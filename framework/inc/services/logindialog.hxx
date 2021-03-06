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

#ifndef INCLUDED_FRAMEWORK_INC_SERVICES_LOGINDIALOG_HXX
#define INCLUDED_FRAMEWORK_INC_SERVICES_LOGINDIALOG_HXX

#include <threadhelp/threadhelpbase.hxx>
#include <macros/generic.hxx>
#include <macros/xinterface.hxx>
#include <macros/xtypeprovider.hxx>
#include <macros/xserviceinfo.hxx>

#include <services/logindialog.hrc>

#include <com/sun/star/awt/XDialog.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/beans/XPropertySetInfo.hpp>
#include <com/sun/star/beans/Property.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/lang/Locale.hpp>
#include <com/sun/star/util/XFlushable.hpp>

#include <tools/config.hxx>
#include <cppuhelper/weak.hxx>
#include <cppuhelper/propshlp.hxx>
#include <vcl/dialog.hxx>
#include <vcl/fixed.hxx>
#include <vcl/edit.hxx>
#include <vcl/combobox.hxx>
#include <vcl/button.hxx>
#include <vcl/morebtn.hxx>

namespace framework{

#ifdef WNT
    #define ININAME                             DECLARE_ASCII("login.ini")
#elif defined UNIX
    #define ININAME                             DECLARE_ASCII("loginrc")
#else
    #error "name of login profile unknown!"
#endif

#define UNCPATHSEPARATOR                        sal_Unicode(0x002F)

//  Use follow keys in follow order.
//  [Global]
//  UserName=as
//  ActiveServer=2
//  ConnectionType=https
//  Language=en-US
//  UseProxy=[browser|custom|none]
//  SecurityProxy=so-webcache:3128
//  dialog=[big|small]
//  [DefaultPorts]
//  https=8445
//  http=8090
//  [ServerHistory]
//  Server_1=localhost
//  Server_2=munch:7202
//  Server_3=www.xxx.com:8000

#define SECTION_GLOBAL                          "Global"
#define SECTION_DEFAULTPORTS                    "DefaultPorts"
#define SECTION_SERVERHISTORY                   "ServerHistory"

struct tIMPL_DialogData
{
    OUString         sUserName               ;
    OUString         sPassword               ;
    css::uno::Sequence< OUString > seqServerList;
    sal_Int32               nActiveServer           ;
    OUString         sConnectionType         ;
    css::lang::Locale       aLanguage               ;
    sal_Int32               nPortHttp               ;
    sal_Int32               nPortHttps              ;
    css::uno::Any           aParentWindow           ;
    OUString         sSecurityProxy          ;
    OUString         sUseProxy               ;
    OUString         sDialog                 ;
    sal_Bool                bProxyChanged           ;

    // default ctor to initialize empty structure.
    tIMPL_DialogData()
        :   sUserName               ( OUString()                     )
        ,   sPassword               ( OUString()                     )
        ,   seqServerList           ( css::uno::Sequence< OUString >() )
        ,   nActiveServer           ( 1                                     )
        ,   sConnectionType         ( OUString()                     )
        ,   aLanguage               ( OUString(), OUString(), OUString() )
        ,   nPortHttp               ( 0                                     )
        ,   nPortHttps              ( 0                                     )
        ,   aParentWindow           (                                       )
        ,   sSecurityProxy          ( OUString()                     )
        ,   sUseProxy               ( OUString()                     )
        ,   sDialog                 ( OUString()                     )
        ,   bProxyChanged           ( sal_False                             )
    {
    }

    // copy ctor to initialize structure with values from another one.
    tIMPL_DialogData( const tIMPL_DialogData& aCopyDataSet )
        :   sUserName               ( aCopyDataSet.sUserName                )
        ,   sPassword               ( aCopyDataSet.sPassword                )
        ,   seqServerList           ( aCopyDataSet.seqServerList            )
        ,   nActiveServer           ( aCopyDataSet.nActiveServer            )
        ,   sConnectionType         ( aCopyDataSet.sConnectionType          )
        ,   aLanguage               ( aCopyDataSet.aLanguage                )
        ,   nPortHttp               ( aCopyDataSet.nPortHttp                )
        ,   nPortHttps              ( aCopyDataSet.nPortHttps               )
        ,   aParentWindow           ( aCopyDataSet.aParentWindow            )
        ,   sSecurityProxy          ( aCopyDataSet.sSecurityProxy           )
        ,   sUseProxy               ( aCopyDataSet.sUseProxy                )
        ,   sDialog                 ( aCopyDataSet.sDialog                  )
        ,   bProxyChanged           ( aCopyDataSet.bProxyChanged            )
    {
    }

    // assignment operator to cop values from another struct to this one.
    tIMPL_DialogData& operator=( const tIMPL_DialogData& aCopyDataSet )
    {
        sUserName               = aCopyDataSet.sUserName                ;
        sPassword               = aCopyDataSet.sPassword                ;
        seqServerList           = aCopyDataSet.seqServerList            ;
        nActiveServer           = aCopyDataSet.nActiveServer            ;
        sConnectionType         = aCopyDataSet.sConnectionType          ;
        aLanguage               = aCopyDataSet.aLanguage                ;
        nPortHttp               = aCopyDataSet.nPortHttp                ;
        nPortHttps              = aCopyDataSet.nPortHttps               ;
        aParentWindow           = aCopyDataSet.aParentWindow            ;
        sSecurityProxy          = aCopyDataSet.sSecurityProxy           ;
        sUseProxy               = aCopyDataSet.sUseProxy                ;
        sDialog                 = aCopyDataSet.sDialog                  ;
        bProxyChanged           = aCopyDataSet.bProxyChanged            ;
        return *this;
    }
};

/*-************************************************************************************************************//**
    @short      implements an "private inline" dialog class used by follow class LoginDialog to show the dialog
    @descr      This is a VCL- modal dialog and not threadsafe! We use it as private definition in the context of login dialog only!

    @implements -

    @base       ModalDialog
*//*-*************************************************************************************************************/

class cIMPL_Dialog  :   public ModalDialog
{
    //-------------------------------------------------------------------------------------------------------------
    //  public methods
    //-------------------------------------------------------------------------------------------------------------

    public:

        /*-****************************************************************************************************//**
            @short      default ctor
            @descr      This ctor initialize the dialog, load resources but not set values on edits or check boxes!
                        These is implemented by setValues() on the same class.
                        You must give us a language identifier to describe which resource should be used!

            @seealso    method setValues()

            @param      "aLanguage" , identifier to describe resource language
            @param      "pParent"   , parent window handle for dialog! If is it NULL -> no parent exist ...
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        cIMPL_Dialog( css::lang::Locale aLocale, Window* pParent );

        /*-****************************************************************************************************//**
            @short      default dtor
            @descr      This dtor deinitialize the dialog and free all used resources.
                        But you can't get the values of the dialog. Use getValues() to do this.

            @seealso    method getValues()

            @param      -
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        ~cIMPL_Dialog();

        /*-****************************************************************************************************//**
            @short      set new values on dialog to show
            @descr      Use this to initialize the dialg with new values for showing before execute.

            @seealso    method getValues()

            @param      "aDataSet"; struct of variables to set it on dialog controls
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        void setValues( const tIMPL_DialogData& aDataSet );

        /*-****************************************************************************************************//**
            @short      get current values from dialog controls
            @descr      Use this if you will get all values of dialog after execute.

            @seealso    method setValues()

            @param      "aDataSet"; struct of variables filled by dialog
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        tIMPL_DialogData getValues();

        /*-****************************************************************************************************/
        /* handler
        */

        DECL_LINK( ClickHdl, void* );

    //-------------------------------------------------------------------------------------------------------------
    //  private methods
    //-------------------------------------------------------------------------------------------------------------

    private:
        void            setCustomSettings();

        void            showDialogExpanded();
        void            showDialogCollapsed();

        /*-****************************************************************************************************//**
            @short      get a host and port from a concated string form <host>:<port>

            @param      "aProxyHostPort" ; a string with the following format <host>:<port>
            @param      "aHost"          ; a host string
            @param      "aPort"          ; a port string
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        void            getProxyHostPort( const OUString& aProxyHostPort, OUString& aHost, OUString& aPort );

        /*-****************************************************************************************************//**
            @short      get a resource for given id from right resource file
            @descr      This dialog need his own resource. We can't use the global resource manager!
                        We must use our own.
                        You must give us the resource language. If no right resource could be found -
                        any  existing one is used automaticly!

            @seealso    method setValues()

            @param      "nId"       ; id to convert it in right resource id
            @param      "aLanguage" ; type of resource language
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        static ResId impl_getResId( sal_uInt16      nId         ,
                                    css::lang::Locale  aLocale );

    //-------------------------------------------------------------------------------------------------------------
    //  private member
    //-------------------------------------------------------------------------------------------------------------

    private:

        FixedImage          m_imageHeader               ;
        FixedText           m_textLoginText             ;
        FixedText           m_textUserName              ;
        Edit                m_editUserName              ;
        FixedText           m_textPassword              ;
        Edit                m_editPassword              ;
        FixedLine           m_fixedLineServer           ;
        FixedText           m_textServer                ;
        ComboBox            m_comboServer               ;
        FixedLine           m_fixedLineProxySettings    ;
        RadioButton         m_radioNoProxy              ;
        RadioButton         m_radioBrowserProxy         ;
        RadioButton         m_radioCustomProxy          ;
        FixedText           m_textSecurityProxy         ;
        FixedText           m_textSecurityProxyHost     ;
        Edit                m_editSecurityProxyHost     ;
        FixedText           m_textSecurityProxyPort     ;
        Edit                m_editSecurityProxyPort     ;
        FixedLine           m_fixedLineButtons          ;
        OKButton            m_buttonOK                  ;
        CancelButton        m_buttonCancel              ;
        PushButton          m_buttonAdditionalSettings  ;
        Size                m_expandedDialogSize        ;
        Size                m_collapsedDialogSize       ;
        Point               m_expOKButtonPos            ;
        Point               m_expCancelButtonPos        ;
        Point               m_expAdditionalButtonPos    ;
        Point               m_colOKButtonPos            ;
        Point               m_colCancelButtonPos        ;
        Point               m_colAdditionalButtonPos    ;
        OUString     m_colButtonAddText          ;
        OUString     m_expButtonAddText          ;
        tIMPL_DialogData    m_aDataSet                  ;
};

/*-************************************************************************************************************//**
    @short

    @descr      -

    @implements XInterface
                XTypeProvider
                XServiceInfo
                XDialog

    @base       ThreadHelpBase
                OWeakObject
*//*-*************************************************************************************************************/

class LoginDialog   :   public css::lang::XTypeProvider     ,
                        public css::lang::XServiceInfo      ,
                        public css::awt::XDialog            ,
                        public css::util::XFlushable        ,
                        private ThreadHelpBase              ,   // Order of baseclasses is necessary for right initialization!
                        public ::cppu::OBroadcastHelper     ,
                        public ::cppu::OPropertySetHelper   ,
                        public ::cppu::OWeakObject
{
    //-------------------------------------------------------------------------------------------------------------
    //  public methods
    //-------------------------------------------------------------------------------------------------------------

    public:

        //---------------------------------------------------------------------------------------------------------
        //  constructor / destructor
        //---------------------------------------------------------------------------------------------------------

        /*-****************************************************************************************************//**
            @short      -
            @descr      -

            @seealso    -

            @param      -
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

         LoginDialog( const css::uno::Reference< css::lang::XMultiServiceFactory >& sFactory );

        /*-****************************************************************************************************//**
            @short      -
            @descr      -

            @seealso    -

            @param      -
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        virtual ~LoginDialog();

        //---------------------------------------------------------------------------------------------------------
        //  XInterface, XTypeProvider, XServiceInfo
        //---------------------------------------------------------------------------------------------------------

        DECLARE_XINTERFACE
        DECLARE_XTYPEPROVIDER
        DECLARE_XSERVICEINFO

        //---------------------------------------------------------------------------------------------------------
        //  XFlushable
        //---------------------------------------------------------------------------------------------------------

        /*-****************************************************************************************************//**
            @short      write changed values to configuration
            @descr      Normaly the dialog returns with an OK or ERROR value. If OK occurs - we flush data
                        automaticly. But otherwise we do nothing. If user of this service wishes to use property set
                        only without any UI(!) - he must call "flush()" explicitly to write data!

            @seealso    -

            @param      -
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        virtual void SAL_CALL flush() throw( css::uno::RuntimeException );
        virtual void SAL_CALL addFlushListener( const css::uno::Reference< css::util::XFlushListener >& xListener ) throw( css::uno::RuntimeException );
        virtual void SAL_CALL removeFlushListener( const css::uno::Reference< css::util::XFlushListener >& xListener ) throw( css::uno::RuntimeException );

        //---------------------------------------------------------------------------------------------------------
        //  XDialog
        //---------------------------------------------------------------------------------------------------------

        /*-****************************************************************************************************//**
            @short      set new title of dialog
            @descr      -

            @seealso    -

            @param      -
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        virtual void SAL_CALL setTitle( const OUString& sTitle ) throw( css::uno::RuntimeException );

        /*-****************************************************************************************************//**
            @short      return the current title of this dialog
            @descr      -

            @seealso    -

            @param      -
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        virtual OUString SAL_CALL getTitle() throw( css::uno::RuntimeException );

        /*-****************************************************************************************************//**
            @short      show the dialog and return user reaction
            @descr      If user close dialog with OK we return 1 else
                        user has cancelled this dialog and we return 0.
                        You can use this return value directly as boolean.

            @seealso    -

            @param      -
            @return     1; if closed with OK
            @return     0; if cancelled

            @onerror    We return 0(sal_False).
        *//*-*****************************************************************************************************/

        virtual sal_Int16 SAL_CALL execute() throw( css::uno::RuntimeException );

        /*-****************************************************************************************************//**
            @short      not implemented yet!
            @descr      -

            @seealso    -

            @param      -
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        virtual void SAL_CALL endExecute() throw( css::uno::RuntimeException );

    //-------------------------------------------------------------------------------------------------------------
    //  protected methods
    //-------------------------------------------------------------------------------------------------------------

    protected:

        //---------------------------------------------------------------------------
        //  OPropertySetHelper
        //---------------------------------------------------------------------------

        /*-****************************************************************************************************//**
            @short      try to convert a property value
            @descr      This method is calling from helperclass "OPropertySetHelper".
                        Don't use this directly!
                        You must try to convert the value of given propertyhandle and
                        return results of this operation. This will be use to ask vetoable
                        listener. If no listener have a veto, we will change value really!
                        ( in method setFastPropertyValue_NoBroadcast(...) )

            @seealso    class OPropertySetHelper
            @seealso    method setFastPropertyValue_NoBroadcast()
            @seealso    method impl_tryToChangeProperty()

            @param      "aConvertedValue"   new converted value of property
            @param      "aOldValue"         old value of property
            @param      "nHandle"           handle of property
            @param      "aValue"            new value of property
            @return     sal_True if value will be changed, sal_FALSE otherway

            @onerror    IllegalArgumentException, if you call this with an invalid argument
        *//*-*****************************************************************************************************/

        virtual sal_Bool SAL_CALL convertFastPropertyValue( css::uno::Any&          aConvertedValue ,
                                                            css::uno::Any&          aOldValue       ,
                                                            sal_Int32               nHandle         ,
                                                            const css::uno::Any&    aValue          ) throw( css::lang::IllegalArgumentException );

        /*-****************************************************************************************************//**
            @short      set value of a transient property
            @descr      This method is calling from helperclass "OPropertySetHelper".
                        Don't use this directly!
                        Handle and value are valid everyway! You must set the new value only.
                        After this, baseclass send messages to all listener automaticly.

            @seealso    OPropertySetHelper

            @param      "nHandle"   handle of property to change
            @param      "aValue"    new value of property
            @return     -

            @onerror    An exception is thrown.
        *//*-*****************************************************************************************************/

        virtual void SAL_CALL setFastPropertyValue_NoBroadcast( sal_Int32                   nHandle ,
                                                                const css::uno::Any&        aValue  ) throw( css::uno::Exception );

        /*-****************************************************************************************************//**
            @short      get value of a transient property
            @descr      This method is calling from helperclass "OPropertySetHelper".
                        Don't use this directly!

            @seealso    OPropertySetHelper

            @param      "nHandle"   handle of property to change
            @param      "aValue"    current value of property
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        virtual void SAL_CALL getFastPropertyValue( css::uno::Any&      aValue  ,
                                                    sal_Int32           nHandle ) const;

        /*-****************************************************************************************************//**
            @short      return structure and information about transient properties
            @descr      This method is calling from helperclass "OPropertySetHelper".
                        Don't use this directly!

            @seealso    OPropertySetHelper

            @param      -
            @return     structure with property-information

            @onerror    -
        *//*-*****************************************************************************************************/

        virtual ::cppu::IPropertyArrayHelper& SAL_CALL getInfoHelper();

        /*-****************************************************************************************************//**
            @short      return propertysetinfo
            @descr      You can call this method to get information about transient properties
                        of this object.

            @seealso    OPropertySetHelper
            @seealso    XPropertySet
            @seealso    XMultiPropertySet

            @param      -
            @return     reference to object with information [XPropertySetInfo]

            @onerror    -
        *//*-*****************************************************************************************************/

        virtual css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo() throw (css::uno::RuntimeException);

    //-------------------------------------------------------------------------------------------------------------
    //  private methods
    //-------------------------------------------------------------------------------------------------------------

    private:

        /*-****************************************************************************************************//**
            @short      return table of all supported properties
            @descr      We need this table to initialize our helper baseclass OPropertySetHelper

            @seealso    -

            @param      -
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        const css::uno::Sequence< css::beans::Property > impl_getStaticPropertyDescriptor();

        /*-****************************************************************************************************//**
            @short      helper method to check if a property will change his value
            @descr      Is necessary for vetoable listener mechanism of OPropertySethelper.

            @seealso    -

            @param      -
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        sal_Bool impl_tryToChangeProperty(  const   OUString&                  sProperty       ,
                                            const   css::uno::Any&                    aValue          ,
                                                    css::uno::Any&                    aOldValue       ,
                                                    css::uno::Any&                    aConvertedValue ) throw( css::lang::IllegalArgumentException );

        sal_Bool impl_tryToChangeProperty(  const   css::uno::Sequence< OUString >& seqProperty,
                                            const   css::uno::Any&                    aValue          ,
                                                    css::uno::Any&                    aOldValue       ,
                                                    css::uno::Any&                    aConvertedValue ) throw( css::lang::IllegalArgumentException );

        sal_Bool impl_tryToChangeProperty(  const   sal_Int32&                        nProperty       ,
                                            const   css::uno::Any&                    aValue          ,
                                                    css::uno::Any&                    aOldValue       ,
                                                    css::uno::Any&                    aConvertedValue ) throw( css::lang::IllegalArgumentException );

        sal_Bool impl_tryToChangeProperty(  const   css::lang::Locale&                aProperty       ,
                                            const   css::uno::Any&                    aValue          ,
                                                    css::uno::Any&                    aOldValue       ,
                                                    css::uno::Any&                    aConvertedValue ) throw( css::lang::IllegalArgumentException );

        sal_Bool impl_tryToChangeProperty(  const   css::uno::Any&                    aProperty       ,
                                            const   css::uno::Any&                    aValue          ,
                                                    css::uno::Any&                    aOldValue       ,
                                                    css::uno::Any&                    aConvertedValue ) throw( css::lang::IllegalArgumentException );

        /*-****************************************************************************************************//**
            @short      search and open profile
            @descr      This method search and open the ini file. It initialize some member too.

            @seealso    -

            @param      -
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        void impl_openProfile();

        /*-****************************************************************************************************//**
            @short      close profile and free some member
            @descr      This method close current opened ini file and deinitialize some member too.

            @seealso    -

            @param      -
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/

        void impl_closeProfile();

        /*-****************************************************************************************************//**
            @short      write profile and free some member
            @descr      This method writes current settings and deinitialize some member too.

            @seealso    -

            @param      -
            @return     -

            @onerror    -
        *//*-*****************************************************************************************************/
        void impl_writeProfile();

        /*-****************************************************************************************************//**
            @short      check current server history
            @descr      Our current server history implementation can handle 10 elements as maximum.
                        If more then 10 elements exist; old ones will be deleted.

            @seealso    -

            @param      "seqHistory"; current history
            @return     Sequence< OUString >; checked and repaired history

            @onerror    -
        *//*-*****************************************************************************************************/

        void impl_addServerToHistory( css::uno::Sequence< OUString >&    seqHistory,
                                      sal_Int32&                                nActiveServer   ,
                                      const OUString&                    sServer         );

        /*-****************************************************************************************************//**
            @short      helper methods to read/write  properties from/to ini file
            @descr      Using of Config-Class isn't easy everytime :-(
                        Thats the reason for these helper. State of operation isn't really important ..
                        but we assert impossible cases or occurred errors!

            @seealso    -

            @param      -
            @return     -

            @onerror    Assertions are shown.
        *//*-*****************************************************************************************************/

        void                    impl_writeUserName              (   const   OUString&        sUserName       );
        void                    impl_writeActiveServer          (           sal_Int32               nActiveServer   );
        void                    impl_writeServerHistory         (   const   css::uno::Sequence< OUString >& lHistory   );
        void                    impl_writeConnectionType        (   const   OUString&        sConnectionType );
        void                    impl_writeLanguage              (   const   css::lang::Locale&      aLanguage       );
        void                    impl_writePortHttp              (           sal_Int32               nPort           );
        void                    impl_writePortHttps             (           sal_Int32               nPort           );
        void                    impl_writeSecurityProxy         (   const   OUString&        sSecurityProxy  );
        void                    impl_writeUseProxy              (   const   OUString&        sUseProxy       );
        void                    impl_writeDialog                (   const   OUString&        sDialog         );

        OUString         impl_readUserName               (                                                   );
        sal_Int32               impl_readActiveServer           (                                                   );
        css::uno::Sequence< OUString > impl_readServerHistory (                                              );
        OUString         impl_readConnectionType         (                                                   );
        css::lang::Locale       impl_readLanguage               (                                                   );
        sal_Int32               impl_readPortHttp               (                                                   );
        sal_Int32               impl_readPortHttps              (                                                   );
        OUString         impl_readSecurityProxy          (                                                   );
        OUString         impl_readUseProxy               (                                                   );
        OUString         impl_readDialog                 (                                                   );

    //-------------------------------------------------------------------------------------------------------------
    //  variables
    //  (should be private everyway!)
    //-------------------------------------------------------------------------------------------------------------

    private:

        css::uno::Reference< css::lang::XMultiServiceFactory >       m_xFactory          ;   /// reference to factory, which has created this instance
        OUString                         m_sININame          ;   /// full qualified path to profile UNC-notation
        Config*                                 m_pINIManager       ;   /// manager for full access to ini file
        sal_Bool                                m_bInExecuteMode    ;   /// protection against setting of properties during showing of dialog
        cIMPL_Dialog*                           m_pDialog           ;   /// VCL dialog
        tIMPL_DialogData                        m_aPropertySet      ;

};      //  class LoginDialog

}       //  namespace framework

#endif // INCLUDED_FRAMEWORK_INC_SERVICES_LOGINDIALOG_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
