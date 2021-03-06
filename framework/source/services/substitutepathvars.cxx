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

#include <config_folders.h>

#include "services/substitutepathvars.hxx"
#include <threadhelp/resetableguard.hxx>
#include <helper/networkdomain.hxx>
#include "services.h"

#include <com/sun/star/beans/XPropertySet.hpp>

#include <unotools/configitem.hxx>
#include <unotools/localfilehelper.hxx>
#include <unotools/configmgr.hxx>

#include <unotools/bootstrap.hxx>
#include <osl/mutex.hxx>
#include <osl/file.hxx>
#include <osl/security.hxx>
#include <osl/socket.hxx>
#include <osl/process.h>
#include <i18nlangtag/languagetag.hxx>
#include <tools/urlobj.hxx>
#include <tools/resmgr.hxx>
#include <tools/wldcrd.hxx>
#include <rtl/ustrbuf.hxx>
#include <rtl/bootstrap.hxx>

#include <officecfg/Office/Paths.hxx>

#include <com/sun/star/container/XHierarchicalNameAccess.hpp>


#include <string.h>

using namespace com::sun::star::uno;
using namespace com::sun::star::beans;
using namespace com::sun::star::util;
using namespace com::sun::star::lang;
using namespace com::sun::star::container;

//_________________________________________________________________________________________________________________
//      Namespace
//_________________________________________________________________________________________________________________

namespace framework
{

struct FixedVariable
{
    const char*     pVarName;
    sal_Int32       nStrLen;
    PreDefVariable  nEnumValue;
    bool            bAbsPath;
};

struct TableEntry
{
    const char* pOSString;
    sal_Int32   nStrLen;
};

// Table with valid operating system strings
// Name of the os as char* and the length
// of the string
static const TableEntry aOSTable[OS_COUNT] =
{
    { RTL_CONSTASCII_STRINGPARAM("WINDOWS") },
    { RTL_CONSTASCII_STRINGPARAM("UNIX") },
    { RTL_CONSTASCII_STRINGPARAM("SOLARIS") },
    { RTL_CONSTASCII_STRINGPARAM("LINUX") },
    { RTL_CONSTASCII_STRINGPARAM("") } // unknown
};

// Table with valid environment variables
// Name of the environment type as a char* and
// the length of the string.
static const TableEntry aEnvTable[ET_COUNT] =
{
    { RTL_CONSTASCII_STRINGPARAM("HOST") },
    { RTL_CONSTASCII_STRINGPARAM("YPDOMAIN") },
    { RTL_CONSTASCII_STRINGPARAM("DNSDOMAIN") },
    { RTL_CONSTASCII_STRINGPARAM("NTDOMAIN") },
    { RTL_CONSTASCII_STRINGPARAM("OS") },
    { RTL_CONSTASCII_STRINGPARAM("") } // unknown
};

// Priority table for the environment types. Lower numbers define
// a higher priority. Equal numbers has the same priority that means
// that the first match wins!!
static const sal_Int16 aEnvPrioTable[ET_COUNT] =
{
    1,      // ET_HOST
    2,      // ET_IPDOMAIN
    2,      // ET_DNSDOMAIN
    2,      // ET_NTDOMAIN
    3,      // ET_OS
    99,     // ET_UNKNOWN
};

// Table with all fixed/predefined variables supported.
static const FixedVariable aFixedVarTable[] =
{
    { RTL_CONSTASCII_STRINGPARAM("$(inst)"),        PREDEFVAR_INST,         true  },
    { RTL_CONSTASCII_STRINGPARAM("$(prog)"),        PREDEFVAR_PROG,         true  },
    { RTL_CONSTASCII_STRINGPARAM("$(user)"),        PREDEFVAR_USER,         true  },
    { RTL_CONSTASCII_STRINGPARAM("$(work)"),        PREDEFVAR_WORK,         true  },      // Special variable (transient)!
    { RTL_CONSTASCII_STRINGPARAM("$(home)"),        PREDEFVAR_HOME,         true  },
    { RTL_CONSTASCII_STRINGPARAM("$(temp)"),        PREDEFVAR_TEMP,         true  },
    { RTL_CONSTASCII_STRINGPARAM("$(path)"),        PREDEFVAR_PATH,         true  },
    { RTL_CONSTASCII_STRINGPARAM("$(lang)"),        PREDEFVAR_LANG,         false },
    { RTL_CONSTASCII_STRINGPARAM("$(langid)"),      PREDEFVAR_LANGID,       false },
    { RTL_CONSTASCII_STRINGPARAM("$(vlang)"),       PREDEFVAR_VLANG,        false },
    { RTL_CONSTASCII_STRINGPARAM("$(instpath)"),    PREDEFVAR_INSTPATH,     true  },
    { RTL_CONSTASCII_STRINGPARAM("$(progpath)"),    PREDEFVAR_PROGPATH,     true  },
    { RTL_CONSTASCII_STRINGPARAM("$(userpath)"),    PREDEFVAR_USERPATH,     true  },
    { RTL_CONSTASCII_STRINGPARAM("$(insturl)"),     PREDEFVAR_INSTURL,      true  },
    { RTL_CONSTASCII_STRINGPARAM("$(progurl)"),     PREDEFVAR_PROGURL,      true  },
    { RTL_CONSTASCII_STRINGPARAM("$(userurl)"),     PREDEFVAR_USERURL,      true  },
    { RTL_CONSTASCII_STRINGPARAM("$(workdirurl)"),  PREDEFVAR_WORKDIRURL,   true  },  // Special variable (transient) and don't use for resubstitution!
    { RTL_CONSTASCII_STRINGPARAM("$(baseinsturl)"), PREDEFVAR_BASEINSTURL,  true  },
    { RTL_CONSTASCII_STRINGPARAM("$(userdataurl)"), PREDEFVAR_USERDATAURL,  true  },
    { RTL_CONSTASCII_STRINGPARAM("$(brandbaseurl)"),PREDEFVAR_BRANDBASEURL, true  }
};

//_________________________________________________________________________________________________________________
//      Implementation helper classes
//_________________________________________________________________________________________________________________

OperatingSystem SubstitutePathVariables_Impl::GetOperatingSystemFromString( const OUString& aOSString )
{
    for ( int i = 0; i < OS_COUNT; i++ )
    {
        if ( aOSString.equalsIgnoreAsciiCaseAsciiL( aOSTable[i].pOSString, aOSTable[i].nStrLen ))
            return (OperatingSystem)i;
    }

    return OS_UNKNOWN;
}

EnvironmentType SubstitutePathVariables_Impl::GetEnvTypeFromString( const OUString& aEnvTypeString )
{
    for ( int i = 0; i < ET_COUNT; i++ )
    {
        if ( aEnvTypeString.equalsIgnoreAsciiCaseAsciiL( aEnvTable[i].pOSString, aEnvTable[i].nStrLen ))
            return (EnvironmentType)i;
    }

    return ET_UNKNOWN;
}

SubstitutePathVariables_Impl::SubstitutePathVariables_Impl( const Link& aNotifyLink ) :
    utl::ConfigItem( OUString( "Office.Substitution" )),
    m_bYPDomainRetrieved( false ),
    m_bDNSDomainRetrieved( false ),
    m_bNTDomainRetrieved( false ),
    m_bHostRetrieved( false ),
    m_aListenerNotify( aNotifyLink ),
    m_aSharePointsNodeName( OUString( "SharePoints" )),
    m_aDirPropertyName( OUString( "/Directory" )),
    m_aEnvPropertyName( OUString( "/Environment" )),
    m_aLevelSep( OUString(  "/" ))
{
    // Enable notification mechanism
    // We need it to get information about changes outside these class on our configuration branch
    Sequence< OUString > aNotifySeq( 1 );
    aNotifySeq[0] = "SharePoints";
    EnableNotification( aNotifySeq, sal_True );
}

SubstitutePathVariables_Impl::~SubstitutePathVariables_Impl()
{
}

void SubstitutePathVariables_Impl::GetSharePointsRules( SubstituteVariables& aSubstVarMap )
{
    Sequence< OUString > aSharePointNames;
    ReadSharePointsFromConfiguration( aSharePointNames );

    if ( aSharePointNames.getLength() > 0 )
    {
        sal_Int32 nSharePoints = 0;

        // Read SharePoints container from configuration
        while ( nSharePoints < aSharePointNames.getLength() )
        {
            OUString aSharePointNodeName( m_aSharePointsNodeName );
            aSharePointNodeName += "/";
            aSharePointNodeName += aSharePointNames[ nSharePoints ];

            SubstituteRuleVector aRuleSet;
            ReadSharePointRuleSetFromConfiguration( aSharePointNames[ nSharePoints ], aSharePointNodeName, aRuleSet );
            if ( !aRuleSet.empty() )
            {
                // We have at minimum one rule. Filter the correct rule out of the rule set
                // and put into our SubstituteVariable map
                SubstituteRule aActiveRule;
                if ( FilterRuleSet( aRuleSet, aActiveRule ))
                {
                    // We have found an active rule
                    aActiveRule.aSubstVariable = aSharePointNames[ nSharePoints ];
                    aSubstVarMap.insert( SubstituteVariables::value_type(
                    aActiveRule.aSubstVariable, aActiveRule ));
                }
            }
            ++nSharePoints;
        }
    }
}

void SubstitutePathVariables_Impl::Notify( const com::sun::star::uno::Sequence< OUString >& /*aPropertyNames*/ )
{
    // NOT implemented yet!
}

void SubstitutePathVariables_Impl::Commit()
{
}


//_________________________________________________________________________________________________________________
//      private methods
//_________________________________________________________________________________________________________________

OperatingSystem SubstitutePathVariables_Impl::GetOperatingSystem()
{
#ifdef SOLARIS
    return OS_SOLARIS;
#elif defined LINUX
    return OS_LINUX;
#elif defined WIN32
    return OS_WINDOWS;
#elif defined UNIX
    return OS_UNIX;
#else
    return OS_UNKNOWN;
#endif
}

const OUString& SubstitutePathVariables_Impl::GetYPDomainName()
{
    if ( !m_bYPDomainRetrieved )
    {
        m_aYPDomain = NetworkDomain::GetYPDomainName().toAsciiLowerCase();
        m_bYPDomainRetrieved = sal_True;
    }

    return m_aYPDomain;
}

const OUString& SubstitutePathVariables_Impl::GetDNSDomainName()
{
    if ( !m_bDNSDomainRetrieved )
    {
        OUString   aTemp;
        osl::SocketAddr aSockAddr;
        oslSocketResult aResult;

        OUString aHostName = GetHostName();
        osl::SocketAddr::resolveHostname( aHostName, aSockAddr );
        aTemp = aSockAddr.getHostname( &aResult );

        // DNS domain name begins after the first "."
        sal_Int32 nIndex = aTemp.indexOf( '.' );
        if ( nIndex >= 0 && aTemp.getLength() > nIndex+1 )
            m_aDNSDomain = aTemp.copy( nIndex+1 ).toAsciiLowerCase();
        else
            m_aDNSDomain = "";

        m_bDNSDomainRetrieved = sal_True;
    }

    return m_aDNSDomain;
}

const OUString& SubstitutePathVariables_Impl::GetNTDomainName()
{
    if ( !m_bNTDomainRetrieved )
    {
        m_aNTDomain = NetworkDomain::GetNTDomainName().toAsciiLowerCase();
        m_bNTDomainRetrieved = sal_True;
    }

    return m_aNTDomain;
}

const OUString& SubstitutePathVariables_Impl::GetHostName()
{
    if (!m_bHostRetrieved)
    {
        oslSocketResult aSocketResult;
        m_aHost = osl::SocketAddr::getLocalHostname( &aSocketResult ).toAsciiLowerCase();
    }

    return m_aHost;
}

bool SubstitutePathVariables_Impl::FilterRuleSet( const SubstituteRuleVector& aRuleSet, SubstituteRule& aActiveRule )
{
    bool bResult = sal_False;

    if ( !aRuleSet.empty() )
    {
        const sal_uInt32 nCount = aRuleSet.size();

        sal_Int16 nPrioCurrentRule = aEnvPrioTable[ ET_UNKNOWN ];
        for ( sal_uInt32 nIndex = 0; nIndex < nCount; nIndex++ )
        {
            const SubstituteRule& aRule = aRuleSet[nIndex];
            EnvironmentType eEnvType        = aRule.aEnvType;

            // Check if environment type has a higher priority than current one!
            if ( nPrioCurrentRule > aEnvPrioTable[eEnvType] )
            {
                switch ( eEnvType )
                {
                    case ET_HOST:
                    {
                        OUString aHost = GetHostName();
                        OUString aHostStr;
                        aRule.aEnvValue >>= aHostStr;
                        aHostStr = aHostStr.toAsciiLowerCase();

                        // Pattern match if domain environment match
                        WildCard aPattern(aHostStr);
                        bool bMatch = aPattern.Matches(aHost);
                        if ( bMatch )
                        {
                            aActiveRule      = aRule;
                            bResult          = true;
                            nPrioCurrentRule = aEnvPrioTable[eEnvType];
                        }
                    }
                    break;

                    case ET_YPDOMAIN:
                    case ET_DNSDOMAIN:
                    case ET_NTDOMAIN:
                    {
                        OUString   aDomain;
                        OUString   aDomainStr;
                        aRule.aEnvValue >>= aDomainStr;
                        aDomainStr = aDomainStr.toAsciiLowerCase();

                        // Retrieve the correct domain value
                        if ( eEnvType == ET_YPDOMAIN )
                            aDomain = GetYPDomainName();
                        else if ( eEnvType == ET_DNSDOMAIN )
                            aDomain = GetDNSDomainName();
                        else
                            aDomain = GetNTDomainName();

                        // Pattern match if domain environment match
                        WildCard aPattern(aDomainStr);
                        bool bMatch = aPattern.Matches(aDomain);
                        if ( bMatch )
                        {
                            aActiveRule      = aRule;
                            bResult          = true;
                            nPrioCurrentRule = aEnvPrioTable[eEnvType];
                        }
                    }
                    break;

                    case ET_OS:
                    {
                        // No pattern matching for OS type
                        OperatingSystem eOSType = GetOperatingSystem();

                        sal_Int16 nValue = 0;
                        aRule.aEnvValue >>= nValue;

                        bool            bUnix = ( eOSType == OS_LINUX ) || ( eOSType == OS_SOLARIS );
                        OperatingSystem eRuleOSType = (OperatingSystem)nValue;

                        // Match if OS identical or rule is set to UNIX and OS is LINUX/SOLARIS!
                        if (( eRuleOSType == eOSType ) || ( eRuleOSType == OS_UNIX && bUnix ))
                        {
                            aActiveRule      = aRule;
                            bResult          = true;
                            nPrioCurrentRule = aEnvPrioTable[eEnvType];
                        }
                    }
                    break;

                    case ET_UNKNOWN: // nothing to do
                        break;

                    default:
                        break;
                }
            }
        }
    }

    return bResult;
}

void SubstitutePathVariables_Impl::ReadSharePointsFromConfiguration( Sequence< OUString >& aSharePointsSeq )
{
    //returns all the names of all share point nodes
    aSharePointsSeq = GetNodeNames( m_aSharePointsNodeName );
}

void SubstitutePathVariables_Impl::ReadSharePointRuleSetFromConfiguration(
        const OUString& aSharePointName,
        const OUString& aSharePointNodeName,
        SubstituteRuleVector& rRuleSet )
{
    Sequence< OUString > aSharePointMappingsNodeNames = GetNodeNames( aSharePointNodeName, utl::CONFIG_NAME_LOCAL_PATH );

    sal_Int32 nSharePointMapping = 0;
    while ( nSharePointMapping < aSharePointMappingsNodeNames.getLength() )
    {
        OUString aSharePointMapping( aSharePointNodeName );
        aSharePointMapping += m_aLevelSep;
        aSharePointMapping += aSharePointMappingsNodeNames[ nSharePointMapping ];

        // Read SharePointMapping
        OUString aDirValue;
        OUString aDirProperty( aSharePointMapping );
        aDirProperty += m_aDirPropertyName;

        // Read only the directory property
        Sequence< OUString > aDirPropertySeq( 1 );
        aDirPropertySeq[0] = aDirProperty;

        Sequence< Any > aValueSeq = GetProperties( aDirPropertySeq );
        if ( aValueSeq.getLength() == 1 )
            aValueSeq[0] >>= aDirValue;

        // Read the environment setting
        OUString aEnvUsed;
        OUString aEnvProperty( aSharePointMapping );
        aEnvProperty += m_aEnvPropertyName;
        Sequence< OUString > aEnvironmentVariable = GetNodeNames( aEnvProperty );

        // Filter the property which has a value set
        Sequence< OUString > aEnvUsedPropertySeq( aEnvironmentVariable.getLength() );

        OUString aEnvUsePropNameTemplate( aEnvProperty );
        aEnvUsePropNameTemplate += m_aLevelSep;

        for ( sal_Int32 nProperty = 0; nProperty < aEnvironmentVariable.getLength(); nProperty++ )
            aEnvUsedPropertySeq[nProperty] = aEnvUsePropNameTemplate + aEnvironmentVariable[nProperty];

        Sequence< Any > aEnvUsedValueSeq;
        aEnvUsedValueSeq = GetProperties( aEnvUsedPropertySeq );

        OUString aEnvUsedValue;
        for ( sal_Int32 nIndex = 0; nIndex < aEnvironmentVariable.getLength(); nIndex++ )
        {
            if ( aEnvUsedValueSeq[nIndex] >>= aEnvUsedValue )
            {
                aEnvUsed = aEnvironmentVariable[nIndex];
                break;
            }
        }

        // Decode the environment and optional the operatng system settings
        Any                             aEnvValue;
        EnvironmentType eEnvType = GetEnvTypeFromString( aEnvUsed );
        if ( eEnvType == ET_OS )
        {
            OperatingSystem eOSType = GetOperatingSystemFromString( aEnvUsedValue );
            aEnvValue <<= (sal_Int16)eOSType;
        }
        else
            aEnvValue <<= aEnvUsedValue;

        // Create rule struct and push it into the rule set
        SubstituteRule aRule( aSharePointName, aDirValue, aEnvValue, eEnvType );
        rRuleSet.push_back( aRule );

        ++nSharePointMapping;
    }
}

//*****************************************************************************************************************
//      XInterface, XTypeProvider, XServiceInfo
//*****************************************************************************************************************
DEFINE_XSERVICEINFO_ONEINSTANCESERVICE_2( SubstitutePathVariables                     ,
                                          ::cppu::OWeakObject                         ,
                                          "com.sun.star.util.PathSubstitution",
                                          IMPLEMENTATIONNAME_SUBSTITUTEPATHVARIABLES    )

DEFINE_INIT_SERVICE                     (   SubstitutePathVariables, {} )


SubstitutePathVariables::SubstitutePathVariables( const Reference< XComponentContext >& xContext ) :
    ThreadHelpBase(),
    m_aImpl( LINK( this, SubstitutePathVariables, implts_ConfigurationNotify )),
    m_xContext( xContext )
{
    int i;

    SetPredefinedPathVariables( m_aPreDefVars );
    m_aImpl.GetSharePointsRules( m_aSubstVarMap );

    // Init the predefined/fixed variable to index hash map
    for ( i = 0; i < PREDEFVAR_COUNT; i++ )
    {
        // Store variable name into struct of predefined/fixed variables
        m_aPreDefVars.m_FixedVarNames[i] = OUString::createFromAscii( aFixedVarTable[i].pVarName );

        // Create hash map entry
        m_aPreDefVarMap.insert( VarNameToIndexMap::value_type(
            m_aPreDefVars.m_FixedVarNames[i], aFixedVarTable[i].nEnumValue ) );
    }

    // Sort predefined/fixed variable to path length
    for ( i = 0; i < PREDEFVAR_COUNT; i++ )
    {
        if (( i != PREDEFVAR_WORKDIRURL ) && ( i != PREDEFVAR_PATH ))
        {
            // Special path variables, don't include into automatic resubstituion search!
            // $(workdirurl) is not allowed to resubstitute! This variable is the value of path settings entry
            // and it could be possible that it will be resubstituted by itself!!
            // Example: WORK_PATH=c:\test, $(workdirurl)=WORK_PATH => WORK_PATH=$(workdirurl) and this cannot be substituted!
            ReSubstFixedVarOrder aFixedVar;
            aFixedVar.eVariable       = aFixedVarTable[i].nEnumValue;
            aFixedVar.nVarValueLength = m_aPreDefVars.m_FixedVar[(sal_Int32)aFixedVar.eVariable].getLength();
            m_aReSubstFixedVarOrder.push_back( aFixedVar );
        }
    }
    m_aReSubstFixedVarOrder.sort();

    // Sort user variables to path length
    SubstituteVariables::const_iterator pIter;
    for ( pIter = m_aSubstVarMap.begin(); pIter != m_aSubstVarMap.end(); ++pIter )
    {
        ReSubstUserVarOrder aUserOrderVar;
        aUserOrderVar.aVarName = "$(" + pIter->second.aSubstVariable + ")";
        aUserOrderVar.nVarValueLength = pIter->second.aSubstVariable.getLength();
        m_aReSubstUserVarOrder.push_back( aUserOrderVar );
    }
    m_aReSubstUserVarOrder.sort();
}

SubstitutePathVariables::~SubstitutePathVariables()
{
}

// XStringSubstitution
OUString SAL_CALL SubstitutePathVariables::substituteVariables( const OUString& aText, sal_Bool bSubstRequired )
throw ( NoSuchElementException, RuntimeException )
{
    ResetableGuard aLock( m_aLock );
    return impl_substituteVariable( aText, bSubstRequired );
}

OUString SAL_CALL SubstitutePathVariables::reSubstituteVariables( const OUString& aText )
throw ( RuntimeException )
{
    ResetableGuard aLock( m_aLock );
    return impl_reSubstituteVariables( aText );
}

OUString SAL_CALL SubstitutePathVariables::getSubstituteVariableValue( const OUString& aVariable )
throw ( NoSuchElementException, RuntimeException )
{
    ResetableGuard aLock( m_aLock );
    return impl_getSubstituteVariableValue( aVariable );
}

//_________________________________________________________________________________________________________________
//      protected methods
//_________________________________________________________________________________________________________________

IMPL_LINK_NOARG(SubstitutePathVariables, implts_ConfigurationNotify)
{
    /* SAFE AREA ----------------------------------------------------------------------------------------------- */
    ResetableGuard aLock( m_aLock );

    return 0;
}

OUString SubstitutePathVariables::ConvertOSLtoUCBURL( const OUString& aOSLCompliantURL ) const
{
    OUString aResult;
    OUString   aTemp;

    osl::FileBase::getSystemPathFromFileURL( aOSLCompliantURL, aTemp );
    utl::LocalFileHelper::ConvertPhysicalNameToURL( aTemp, aResult );

    // Not all OSL URL's can be mapped to UCB URL's!
    if ( aResult.isEmpty() )
        return aOSLCompliantURL;
    else
        return aResult;
}

OUString SubstitutePathVariables::GetWorkPath() const
{
    OUString aWorkPath;
    css::uno::Reference< css::container::XHierarchicalNameAccess > xPaths(officecfg::Office::Paths::Paths::get(m_xContext), css::uno::UNO_QUERY_THROW);
    OUString xWork;
    if (!(xPaths->getByHierarchicalName("['Work']/WritePath") >>= xWork))
        // fallback in case config layer does not return an useable work dir value.
        aWorkPath = GetWorkVariableValue();

    return aWorkPath;
}

OUString SubstitutePathVariables::GetWorkVariableValue() const
{
    OUString aWorkPath;
    boost::optional<OUString> x(officecfg::Office::Paths::Variables::Work::get(m_xContext));
    if (!x)
    {
        // fallback to $HOME in case platform dependend config layer does not return
        // an usuable work dir value.
        osl::Security aSecurity;
        aSecurity.getHomeDir( aWorkPath );
    }
    else
        aWorkPath = x.get();
    return ConvertOSLtoUCBURL( aWorkPath );
}

OUString SubstitutePathVariables::GetHomeVariableValue() const
{
    osl::Security   aSecurity;
    OUString   aHomePath;

    aSecurity.getHomeDir( aHomePath );
    return ConvertOSLtoUCBURL( aHomePath );
}

OUString SubstitutePathVariables::GetPathVariableValue() const
{

    OUString aRetStr;
    const char*   pEnv = getenv( "PATH" );

    if ( pEnv )
    {
        const int PATH_EXTEND_FACTOR = 120;
        OUString       aTmp;
        OUString       aPathList( pEnv, strlen( pEnv ), osl_getThreadTextEncoding() );
        OUStringBuffer aPathStrBuffer( aPathList.getLength() * PATH_EXTEND_FACTOR / 100 );

        bool      bAppendSep = false;
        sal_Int32 nToken = 0;
        do
        {
            OUString sToken = aPathList.getToken(0, SAL_PATHSEPARATOR, nToken);
            if (!sToken.isEmpty())
            {
                osl::FileBase::getFileURLFromSystemPath( sToken, aTmp );
                if ( bAppendSep )
                    aPathStrBuffer.appendAscii( ";" ); // Office uses ';' as path separator
                aPathStrBuffer.append( aTmp );
                bAppendSep = true;
            }
        }
        while(nToken>=0);

        aRetStr = aPathStrBuffer.makeStringAndClear();
    }

    return aRetStr;
}

OUString SubstitutePathVariables::impl_substituteVariable( const OUString& rText, bool bSubstRequired )
throw ( NoSuchElementException, RuntimeException )
{
    // This is maximal recursive depth supported!
    const sal_Int32 nMaxRecursiveDepth = 8;

    OUString   aWorkText = rText;
    OUString   aResult;

    // Use vector with strings to detect endless recursions!
    std::vector< OUString > aEndlessRecursiveDetector;

    // Search for first occurrence of "$(...".
    sal_Int32   nDepth = 0;
    bool        bSubstitutionCompleted = false;
    sal_Int32   nPosition = aWorkText.indexOf( "$(" );
    sal_Int32   nLength = 0; // = count of letters from "$(" to ")" in string
    bool        bVarNotSubstituted = false;

    // Have we found any variable like "$(...)"?
    if ( nPosition != -1 )
    {
        // Yes; Get length of found variable.
        // If no ")" was found - nLength is set to 0 by default! see before.
        sal_Int32 nEndPosition = aWorkText.indexOf( ')', nPosition );
        if ( nEndPosition != -1 )
            nLength = nEndPosition - nPosition + 1;
    }

    // Is there something to replace ?
    bool bWorkRetrieved       = false;
    bool bWorkDirURLRetrieved = false;
    while ( !bSubstitutionCompleted && nDepth < nMaxRecursiveDepth )
    {
        while ( ( nPosition != -1 ) && ( nLength > 3 ) ) // "$(" ")"
        {
            // YES; Get the next variable for replace.
            sal_Int32     nReplaceLength  = 0;
            OUString aReplacement;
            OUString aSubString      = aWorkText.copy( nPosition, nLength );
            OUString aSubVarString;

            // Path variables are not case sensitive!
            aSubVarString = aSubString.toAsciiLowerCase();
            VarNameToIndexMap::const_iterator pNTOIIter = m_aPreDefVarMap.find( aSubVarString );
            if ( pNTOIIter != m_aPreDefVarMap.end() )
            {
                // Fixed/Predefined variable found
                PreDefVariable nIndex = (PreDefVariable)pNTOIIter->second;

                // Determine variable value and length from array/table
                if ( nIndex == PREDEFVAR_WORK && !bWorkRetrieved )
                {
                    // Transient value, retrieve it again
                    m_aPreDefVars.m_FixedVar[ (PreDefVariable)nIndex ] = GetWorkVariableValue();
                    bWorkRetrieved = true;
                }
                else if ( nIndex == PREDEFVAR_WORKDIRURL && !bWorkDirURLRetrieved )
                {
                    // Transient value, retrieve it again
                    m_aPreDefVars.m_FixedVar[ (PreDefVariable)nIndex ] = GetWorkPath();
                    bWorkDirURLRetrieved = true;
                }

                // Check preconditions to substitue path variables.
                // 1. A path variable can only be substituted if it follows a ';'!
                // 2. It's located exactly at the start of the string being substituted!
                if (( aFixedVarTable[ int( nIndex ) ].bAbsPath && (( nPosition == 0 ) || (( nPosition > 0 ) && ( aWorkText[nPosition-1] == ';')))) ||
            ( !aFixedVarTable[ int( nIndex ) ].bAbsPath ))
        {
                    aReplacement = m_aPreDefVars.m_FixedVar[ (PreDefVariable)nIndex ];
                    nReplaceLength = nLength;
                }
            }
            else
            {
                // Extract the variable name and try to find in the user defined variable set
                OUString aVarName = aSubString.copy( 2, nLength-3 );
                SubstituteVariables::const_iterator pIter = m_aSubstVarMap.find( aVarName );
                if ( pIter != m_aSubstVarMap.end() )
                {
                    // Found.
                    aReplacement = pIter->second.aSubstValue;
                    nReplaceLength = nLength;
                }
            }

            // Have we found something to replace?
            if ( nReplaceLength > 0 )
            {
                // Yes ... then do it.
                aWorkText = aWorkText.replaceAt( nPosition, nReplaceLength, aReplacement );
            }
            else
            {
                // Variable not known
                bVarNotSubstituted = false;
                nPosition += nLength;
            }

            // Step after replaced text! If no text was replaced (unknown variable!),
            // length of aReplacement is 0 ... and we don't step then.
            nPosition += aReplacement.getLength();

            // We must control index in string before call something at OUString!
            // The OUString-implementation don't do it for us :-( but the result is not defined otherwise.
            if ( nPosition + 1 > aWorkText.getLength() )
            {
                // Position is out of range. Break loop!
                nPosition = -1;
                nLength = 0;
            }
            else
            {
                // Else; Position is valid. Search for next variable to replace.
                nPosition = aWorkText.indexOf( "$(", nPosition );
                // Have we found any variable like "$(...)"?
                if ( nPosition != -1 )
                {
                    // Yes; Get length of found variable. If no ")" was found - nLength must set to 0!
                    nLength = 0;
                    sal_Int32 nEndPosition = aWorkText.indexOf( ')', nPosition );
                    if ( nEndPosition != -1 )
                        nLength = nEndPosition - nPosition + 1;
                }
            }
        }

        nPosition = aWorkText.indexOf( "$(" );
        if ( nPosition == -1 )
        {
            bSubstitutionCompleted = true;
            break; // All variables are substituted
        }
        else
        {
            // Check for recursion
            const sal_uInt32 nCount = aEndlessRecursiveDetector.size();
            for ( sal_uInt32 i=0; i < nCount; i++ )
            {
                if ( aEndlessRecursiveDetector[i] == aWorkText )
                {
                    if ( bVarNotSubstituted )
                        break; // Not all variables could be substituted!
                    else
                    {
                        nDepth = nMaxRecursiveDepth;
                        break; // Recursion detected!
                    }
                }
            }

            aEndlessRecursiveDetector.push_back( aWorkText );

            // Initialize values for next
            sal_Int32 nEndPosition = aWorkText.indexOf( ')', nPosition );
            if ( nEndPosition != -1 )
                nLength = nEndPosition - nPosition + 1;
            bVarNotSubstituted = sal_False;
            ++nDepth;
        }
    }

    // Fill return value with result
    if ( bSubstitutionCompleted )
    {
        // Substitution successful!
        aResult = aWorkText;
    }
    else
    {
        // Substitution not successful!
        if ( nDepth == nMaxRecursiveDepth )
        {
            // recursion depth reached!
            if ( bSubstRequired )
            {
                OUString aMsg( "Endless recursion detected. Cannot substitute variables!" );
                throw NoSuchElementException( aMsg, (cppu::OWeakObject *)this );
            }
            else
                aResult = rText;
        }
        else
        {
            // variable in text but unknown!
            if ( bSubstRequired )
            {
                OUString aMsg( "Unknown variable found!" );
                throw NoSuchElementException( aMsg, (cppu::OWeakObject *)this );
            }
            else
                aResult = aWorkText;
        }
    }

    return aResult;
}

OUString SubstitutePathVariables::impl_reSubstituteVariables( const OUString& rURL )
throw ( RuntimeException )
{
    OUString aURL;

    INetURLObject aUrl( rURL );
    if ( !aUrl.HasError() )
        aURL = aUrl.GetMainURL( INetURLObject::NO_DECODE );
    else
    {
        // Convert a system path to a UCB compliant URL before resubstitution
        OUString aTemp;
        if ( osl::FileBase::getFileURLFromSystemPath( rURL, aTemp ) == osl::FileBase::E_None )
        {
            aTemp = ConvertOSLtoUCBURL( aTemp );
            if ( !aTemp.isEmpty() )
            {
                aURL = INetURLObject( aTemp ).GetMainURL( INetURLObject::NO_DECODE );
                if( aURL.isEmpty() )
                    return rURL;
            }
            else
                return rURL;
        }
        else
        {
            // rURL is not a valid URL nor a osl system path. Give up and return error!
            return rURL;
        }
    }

    // Get transient predefined path variable $(work) value before starting resubstitution
    m_aPreDefVars.m_FixedVar[ PREDEFVAR_WORK ] = GetWorkVariableValue();

    for (;;)
    {
        bool bVariableFound = false;

        for (ReSubstFixedVarOrderVector::const_iterator i(
                 m_aReSubstFixedVarOrder.begin());
             i != m_aReSubstFixedVarOrder.end(); ++i)
        {
            OUString aValue = m_aPreDefVars.m_FixedVar[i->eVariable];
            sal_Int32 nPos = aURL.indexOf( aValue );
            if ( nPos >= 0 )
            {
                bool bMatch = true;
                if ( i->eVariable == PREDEFVAR_LANG ||
                     i->eVariable == PREDEFVAR_LANGID ||
                     i->eVariable == PREDEFVAR_VLANG )
                {
                    // Special path variables as they can occur in the middle of a path. Only match if they
                    // describe a whole directory and not only a substring of a directory!
                    const sal_Unicode* pStr = aURL.getStr();

                    if ( nPos > 0 )
                        bMatch = ( aURL[ nPos-1 ] == '/' );

                    if ( bMatch )
                    {
                        if ( nPos + aValue.getLength() < aURL.getLength() )
                            bMatch = ( pStr[ nPos + aValue.getLength() ] == '/' );
                    }
                }

                if ( bMatch )
                {
                    aURL = aURL.replaceAt(
                        nPos, aValue.getLength(),
                        m_aPreDefVars.m_FixedVarNames[i->eVariable]);
                    bVariableFound = true; // Resubstitution not finished yet!
                    break;
                }
            }
        }

        // This part can be iteratered more than one time as variables can contain variables again!
        for (ReSubstUserVarOrderVector::const_iterator i(
                 m_aReSubstUserVarOrder.begin());
             i != m_aReSubstUserVarOrder.end(); ++i)
        {
            OUString aVarValue = i->aVarName;
            sal_Int32 nPos = aURL.indexOf( aVarValue );
            if ( nPos >= 0 )
            {
                aURL = aURL.replaceAt(
                    nPos, aVarValue.getLength(), "$(" + aVarValue + ")");
                bVariableFound = true;  // Resubstitution not finished yet!
            }
        }

        if ( !bVariableFound )
        {
            return aURL;
        }
    }
}

// This method support both request schemes "$("<varname>")" or "<varname>".
OUString SubstitutePathVariables::impl_getSubstituteVariableValue( const OUString& rVariable )
throw ( NoSuchElementException, RuntimeException )
{
    OUString aVariable;

    sal_Int32 nPos = rVariable.indexOf( "$(" );
    if ( nPos == -1 )
    {
        // Prepare variable name before hash map access
        aVariable = "$(" + rVariable + ")";
    }

    VarNameToIndexMap::const_iterator pNTOIIter = m_aPreDefVarMap.find( ( nPos == -1 ) ? aVariable : rVariable );

    // Fixed/Predefined variable
    if ( pNTOIIter != m_aPreDefVarMap.end() )
    {
        PreDefVariable nIndex = (PreDefVariable)pNTOIIter->second;
        return m_aPreDefVars.m_FixedVar[(sal_Int32)nIndex];
    }
    else
    {
        // Prepare variable name before hash map access
        if ( nPos >= 0 )
        {
            if ( rVariable.getLength() > 3 )
                aVariable = rVariable.copy( 2, rVariable.getLength() - 3 );
            else
            {
                OUString aExceptionText("Unknown variable!");
                throw NoSuchElementException(aExceptionText, (cppu::OWeakObject *)this);
            }
        }
        else
            aVariable = rVariable;

        // User defined variable
        SubstituteVariables::const_iterator pIter = m_aSubstVarMap.find( aVariable );
        if ( pIter != m_aSubstVarMap.end() )
        {
            // found!
            return pIter->second.aSubstValue;
        }

        OUString aExceptionText("Unknown variable!");
        throw NoSuchElementException(aExceptionText, (cppu::OWeakObject *)this);
    }
}

void SubstitutePathVariables::SetPredefinedPathVariables( PredefinedPathVariables& aPreDefPathVariables )
{

    aPreDefPathVariables.m_FixedVar[PREDEFVAR_BRANDBASEURL] = "$BRAND_BASE_DIR";
    rtl::Bootstrap::expandMacros(
        aPreDefPathVariables.m_FixedVar[PREDEFVAR_BRANDBASEURL]);

    // Get inspath and userpath from bootstrap mechanism in every case as file URL
    ::utl::Bootstrap::PathStatus aState;
    OUString              sVal  ;

    aState = utl::Bootstrap::locateUserData( sVal );
    //There can be the valid case that there is no user installation.
    //TODO: Is that still the case? (With OOo 3.4, "unopkg sync" was run as part
    // of the setup. Then no user installation was required.)
    //Therefore we do not assert here.
    if( aState == ::utl::Bootstrap::PATH_EXISTS ) {
        aPreDefPathVariables.m_FixedVar[ PREDEFVAR_USERPATH ] = ConvertOSLtoUCBURL( sVal );
    }

    // Set $(inst), $(instpath), $(insturl)
    aPreDefPathVariables.m_FixedVar[ PREDEFVAR_INSTPATH ] = aPreDefPathVariables.m_FixedVar[PREDEFVAR_BRANDBASEURL];
    aPreDefPathVariables.m_FixedVar[ PREDEFVAR_INSTURL ]    = aPreDefPathVariables.m_FixedVar[ PREDEFVAR_INSTPATH ];
    aPreDefPathVariables.m_FixedVar[ PREDEFVAR_INST ]       = aPreDefPathVariables.m_FixedVar[ PREDEFVAR_INSTPATH ];
    // New variable of hierachy service (#i32656#)
    aPreDefPathVariables.m_FixedVar[ PREDEFVAR_BASEINSTURL ]= aPreDefPathVariables.m_FixedVar[ PREDEFVAR_INSTPATH ];

    // Set $(user), $(userpath), $(userurl)
    aPreDefPathVariables.m_FixedVar[ PREDEFVAR_USERURL ]    = aPreDefPathVariables.m_FixedVar[ PREDEFVAR_USERPATH ];
    aPreDefPathVariables.m_FixedVar[ PREDEFVAR_USER ]       = aPreDefPathVariables.m_FixedVar[ PREDEFVAR_USERPATH ];
    // New variable of hierachy service (#i32656#)
    aPreDefPathVariables.m_FixedVar[ PREDEFVAR_USERDATAURL ]= aPreDefPathVariables.m_FixedVar[ PREDEFVAR_USERPATH ];

    // Detect the program directory
    // Set $(prog), $(progpath), $(progurl)
    INetURLObject aProgObj(
        aPreDefPathVariables.m_FixedVar[PREDEFVAR_BRANDBASEURL] );
    if ( !aProgObj.HasError() && aProgObj.insertName( OUString(LIBO_BIN_FOLDER) ) )
    {
        aPreDefPathVariables.m_FixedVar[ PREDEFVAR_PROGPATH ] = aProgObj.GetMainURL(INetURLObject::NO_DECODE);
        aPreDefPathVariables.m_FixedVar[ PREDEFVAR_PROGURL ]  = aPreDefPathVariables.m_FixedVar[ PREDEFVAR_PROGPATH ];
        aPreDefPathVariables.m_FixedVar[ PREDEFVAR_PROG ]     = aPreDefPathVariables.m_FixedVar[ PREDEFVAR_PROGPATH ];
    }

    // Detect the language type of the current office
    aPreDefPathVariables.m_eLanguageType = LANGUAGE_ENGLISH_US;
    OUString aLocaleStr( utl::ConfigManager::getLocale() );
    aPreDefPathVariables.m_eLanguageType = LanguageTag::convertToLanguageTypeWithFallback( aLocaleStr );
    // We used to have an else branch here with a SAL_WARN, but that
    // always fired in some unit tests when this code was built with
    // debug=t, so it seems fairly pointless, especially as
    // aPreDefPathVariables.m_eLanguageType has been initialized to a
    // default value above anyway.

    // Set $(lang)
    aPreDefPathVariables.m_FixedVar[ PREDEFVAR_LANG ] = ConvertOSLtoUCBURL(
    OUString::createFromAscii( ResMgr::GetLang( aPreDefPathVariables.m_eLanguageType, 0 ) ));

    // Set $(vlang)
    aPreDefPathVariables.m_FixedVar[ PREDEFVAR_VLANG ] = aLocaleStr;

    // Set $(langid)
    aPreDefPathVariables.m_FixedVar[ PREDEFVAR_LANGID ] = OUString::number( aPreDefPathVariables.m_eLanguageType );

    // Set the other pre defined path variables
    // Set $(work)
    aPreDefPathVariables.m_FixedVar[ PREDEFVAR_WORK ] = GetWorkVariableValue();
    aPreDefPathVariables.m_FixedVar[ PREDEFVAR_HOME ] = GetHomeVariableValue();

    // Set $(workdirurl) this is the value of the path PATH_WORK which doesn't make sense
    // anymore because the path settings service has this value! It can deliver this value more
    // quickly than the substitution service!
    aPreDefPathVariables.m_FixedVar[ PREDEFVAR_WORKDIRURL ] = GetWorkPath();

    // Set $(path) variable
    aPreDefPathVariables.m_FixedVar[ PREDEFVAR_PATH ] = GetPathVariableValue();

    // Set $(temp)
    OUString aTmp;
    osl::FileBase::getTempDirURL( aTmp );
    aPreDefPathVariables.m_FixedVar[ PREDEFVAR_TEMP ] = ConvertOSLtoUCBURL( aTmp );
}

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
