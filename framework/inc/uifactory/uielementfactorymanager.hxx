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

#ifndef INCLUDED_FRAMEWORK_INC_UIFACTORY_UIELEMENTFACTORYMANAGER_HXX
#define INCLUDED_FRAMEWORK_INC_UIFACTORY_UIELEMENTFACTORYMANAGER_HXX

/** Attention: stl headers must(!) be included at first. Otherwise it can make trouble
               with solaris headers ...
*/
#include <vector>
#include <list>

#include <threadhelp/threadhelpbase.hxx>
#include <macros/generic.hxx>
#include <macros/xinterface.hxx>
#include <macros/xtypeprovider.hxx>
#include <macros/xserviceinfo.hxx>
#include <stdtypes.h>

#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/ui/XUIElementFactoryManager.hpp>
#include <com/sun/star/container/XContainerListener.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include "com/sun/star/frame/XModuleManager2.hpp"

#include <cppuhelper/implbase1.hxx>
#include <cppuhelper/implbase2.hxx>
#include <rtl/ustring.hxx>

namespace framework
{

    class ConfigurationAccess_FactoryManager : // interfaces
                                                    // baseclasses
                                                    // Order is necessary for right initialization!
                                                    private ThreadHelpBase                           ,
                                                    public  ::cppu::WeakImplHelper1< ::com::sun::star::container::XContainerListener>
{
    public:
                      ConfigurationAccess_FactoryManager( const ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext>& rxContext, const OUString& _sRoot );
        virtual       ~ConfigurationAccess_FactoryManager();

        void          readConfigurationData();

        OUString                           getFactorySpecifierFromTypeNameModule( const OUString& rType, const OUString& rName, const OUString& rModule ) const;
        void                                    addFactorySpecifierToTypeNameModule( const OUString& rType, const OUString& rName, const OUString& rModule, const OUString& aServiceSpecifier );
        void                                    removeFactorySpecifierFromTypeNameModule( const OUString& rType, const OUString& rName, const OUString& rModule );
        ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue > >   getFactoriesDescription() const;

        // container.XContainerListener
    virtual void SAL_CALL elementInserted( const ::com::sun::star::container::ContainerEvent& Event ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL elementRemoved( const ::com::sun::star::container::ContainerEvent& Event ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL elementReplaced( const ::com::sun::star::container::ContainerEvent& Event ) throw (::com::sun::star::uno::RuntimeException);

    // lang.XEventListener
    virtual void SAL_CALL disposing( const ::com::sun::star::lang::EventObject& Source ) throw (::com::sun::star::uno::RuntimeException);

    private:
        class FactoryManagerMap : public boost::unordered_map< OUString,
                                                     OUString,
                                                     OUStringHash,
                                                     ::std::equal_to< OUString > >
        {
            inline void free()
            {
                FactoryManagerMap().swap( *this );
            }
        };

        sal_Bool impl_getElementProps( const ::com::sun::star::uno::Any& rElement, OUString& rType, OUString& rName, OUString& rModule, OUString& rServiceSpecifier ) const;

        OUString                     m_aPropType;
        OUString                     m_aPropName;
        OUString                     m_aPropModule;
        OUString                     m_aPropFactory;
        OUString                   m_sRoot;
        FactoryManagerMap                 m_aFactoryManagerMap;
        ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory > m_xConfigProvider;
        ::com::sun::star::uno::Reference< ::com::sun::star::container::XNameAccess >     m_xConfigAccess;
        ::com::sun::star::uno::Reference< ::com::sun::star::container::XContainerListener > m_xConfigListener;
        sal_Bool                          m_bConfigAccessInitialized;
};


class UIElementFactoryManager :  private ThreadHelpBase                                             ,   // Struct for right initalization of mutex member! Must be first of baseclasses.
                                 public ::cppu::WeakImplHelper2< ::com::sun::star::lang::XServiceInfo,
                                                                 ::com::sun::star::ui::XUIElementFactoryManager>
{
    public:
        UIElementFactoryManager( const ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext >& rxContext );
        virtual ~UIElementFactoryManager();

        //  XInterface, XTypeProvider, XServiceInfo
        DECLARE_XSERVICEINFO

        // XUIElementFactory
        virtual ::com::sun::star::uno::Reference< ::com::sun::star::ui::XUIElement > SAL_CALL createUIElement( const OUString& ResourceURL, const ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue >& Args ) throw (::com::sun::star::container::NoSuchElementException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::uno::RuntimeException);

        // XUIElementFactoryRegistration
        virtual ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue > > SAL_CALL getRegisteredFactories(  ) throw (::com::sun::star::uno::RuntimeException);
        virtual ::com::sun::star::uno::Reference< ::com::sun::star::ui::XUIElementFactory > SAL_CALL getFactory( const OUString& ResourceURL, const OUString& ModuleIdentifier ) throw (::com::sun::star::uno::RuntimeException);
        virtual void SAL_CALL registerFactory( const OUString& aType, const OUString& aName, const OUString& aModuleIdentifier, const OUString& aFactoryImplementationName ) throw (::com::sun::star::container::ElementExistException, ::com::sun::star::uno::RuntimeException);
        virtual void SAL_CALL deregisterFactory( const OUString& aType, const OUString& aName, const OUString& aModuleIdentifier ) throw (::com::sun::star::container::NoSuchElementException, ::com::sun::star::uno::RuntimeException);

    private:

        sal_Bool                                                                            m_bConfigRead;
        ::com::sun::star::uno::Reference< ::com::sun::star::uno::XComponentContext >        m_xContext;
        ::com::sun::star::uno::Reference< ::com::sun::star::frame::XModuleManager2 >        m_xModuleManager;
        ConfigurationAccess_FactoryManager*                                        m_pConfigAccess;
};

} // namespace framework

#endif // INCLUDED_FRAMEWORK_INC_UIFACTORY_UIELEMENTFACTORYMANAGER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
