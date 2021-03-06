/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "charttest.hxx"

#include <com/sun/star/chart/ErrorBarStyle.hpp>
#include <com/sun/star/chart2/XRegressionCurveContainer.hpp>
#include <com/sun/star/lang/XServiceName.hpp>
#include <com/sun/star/packages/zip/ZipFileAccess.hpp>

#include <unotools/ucbstreamhelper.hxx>
#include <rtl/strbuf.hxx>

#include <libxml/xpathInternals.h>
#include <libxml/parserInternals.h>

#include <algorithm>

using uno::Reference;
using beans::XPropertySet;

class Chart2ExportTest : public ChartTest
{
public:
    Chart2ExportTest() : ChartTest() {}
    void test();
    void testErrorBarXLSX();
    void testTrendline();
    void testStockChart();
    void testBarChart();
    void testCrosses();
    void testChartDataTable();

    CPPUNIT_TEST_SUITE(Chart2ExportTest);
    CPPUNIT_TEST(test);
    CPPUNIT_TEST(testErrorBarXLSX);
    CPPUNIT_TEST(testTrendline);
    CPPUNIT_TEST(testStockChart);
    CPPUNIT_TEST(testBarChart);
    CPPUNIT_TEST(testCrosses);
    CPPUNIT_TEST(testChartDataTable);
    CPPUNIT_TEST_SUITE_END();

protected:
    /**
     * Given that some problem doesn't affect the result in the importer, we
     * test the resulting file directly, by opening the zip file, parsing an
     * xml stream, and asserting an XPath expression. This method returns the
     * xml stream, so that you can do the asserting.
     */
    xmlDocPtr parseExport(const OUString& rDir, const OUString& rFilterFormat);

    /**
     * Helper method to return nodes represented by rXPath.
     */
    xmlNodeSetPtr getXPathNode(xmlDocPtr pXmlDoc, const OString& rXPath);

    /**
     * Assert that rXPath exists, and returns exactly one node.
     * In case rAttribute is provided, the rXPath's attribute's value must
     * equal to the rExpected value.
     */
    void assertXPath(xmlDocPtr pXmlDoc, const OString& rXPath, const OString& rAttribute = OString(), const OUString& rExpectedValue = OUString());

    /**
     * Assert that rXPath exists, and returns exactly nNumberOfNodes nodes.
     * Useful for checking that we do _not_ export some node (nNumberOfNodes == 0).
     */
    void assertXPath(xmlDocPtr pXmlDoc, const OString& rXPath, int nNumberOfNodes);

    /**
     * Same as the assertXPath(), but don't assert: return the string instead.
     */
    OUString getXPath(xmlDocPtr pXmlDoc, const OString& rXPath, const OString& rAttribute);

private:
};

void Chart2ExportTest::test()
{
    load("/chart2/qa/extras/data/ods/", "simple_export_chart.ods");
    reload("Calc Office Open XML");
}

struct CheckForChartName
{
private:
    OUString aDir;

public:
    CheckForChartName( const OUString& rDir ):
        aDir(rDir) {}

    bool operator()(const OUString& rName)
    {
        if(!rName.startsWith(aDir))
            return false;

        if(!rName.endsWith(".xml"))
            return false;

        return true;
    }
};

OUString findChartFile(const OUString& rDir, uno::Reference< container::XNameAccess > xNames )
{
    uno::Sequence<OUString> rNames = xNames->getElementNames();
    OUString* pElement = std::find_if(rNames.begin(), rNames.end(), CheckForChartName(rDir));

    CPPUNIT_ASSERT(pElement);
    CPPUNIT_ASSERT(pElement != rNames.end());
    return *pElement;
}

xmlDocPtr Chart2ExportTest::parseExport(const OUString& rDir, const OUString& rFilterFormat)
{
    boost::shared_ptr<utl::TempFile> pTempFile = reload(rFilterFormat);

    // Read the XML stream we're interested in.
    uno::Reference<packages::zip::XZipFileAccess2> xNameAccess = packages::zip::ZipFileAccess::createWithURL(comphelper::getComponentContext(m_xSFactory), pTempFile->GetURL());
    uno::Reference<io::XInputStream> xInputStream(xNameAccess->getByName(findChartFile(rDir, xNameAccess)), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xInputStream.is());
    boost::shared_ptr<SvStream> pStream(utl::UcbStreamHelper::CreateStream(xInputStream, sal_True));
    pStream->Seek(STREAM_SEEK_TO_END);
    sal_Size nSize = pStream->Tell();
    pStream->Seek(0);
    OStringBuffer aDocument(nSize);
    char ch;
    for (sal_Size i = 0; i < nSize; ++i)
    {
        *pStream >> ch;
        aDocument.append(ch);
    }

    // Parse the XML.
    return xmlParseMemory((const char*)aDocument.getStr(), aDocument.getLength());
}

xmlNodeSetPtr Chart2ExportTest::getXPathNode(xmlDocPtr pXmlDoc, const OString& rXPath)
{
    xmlXPathContextPtr pXmlXpathCtx = xmlXPathNewContext(pXmlDoc);
    xmlXPathRegisterNs(pXmlXpathCtx, BAD_CAST("w"), BAD_CAST("http://schemas.openxmlformats.org/wordprocessingml/2006/main"));
    xmlXPathRegisterNs(pXmlXpathCtx, BAD_CAST("v"), BAD_CAST("urn:schemas-microsoft-com:vml"));
    xmlXPathRegisterNs(pXmlXpathCtx, BAD_CAST("c"), BAD_CAST("http://schemas.openxmlformats.org/drawingml/2006/chart"));
    xmlXPathObjectPtr pXmlXpathObj = xmlXPathEvalExpression(BAD_CAST(rXPath.getStr()), pXmlXpathCtx);
    return pXmlXpathObj->nodesetval;
}

void Chart2ExportTest::assertXPath(xmlDocPtr pXmlDoc, const OString& rXPath, const OString& rAttribute, const OUString& rExpectedValue)
{
    OUString aValue = getXPath(pXmlDoc, rXPath, rAttribute);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        OString("Attribute '" + rAttribute + "' of '" + rXPath + "' incorrect value.").getStr(),
        rExpectedValue, aValue);
}

void Chart2ExportTest::assertXPath(xmlDocPtr pXmlDoc, const OString& rXPath, int nNumberOfNodes)
{
    xmlNodeSetPtr pXmlNodes = getXPathNode(pXmlDoc, rXPath);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        OString("XPath '" + rXPath + "' number of nodes is incorrect").getStr(),
        nNumberOfNodes, xmlXPathNodeSetGetLength(pXmlNodes));
}

OUString Chart2ExportTest::getXPath(xmlDocPtr pXmlDoc, const OString& rXPath, const OString& rAttribute)
{
    xmlNodeSetPtr pXmlNodes = getXPathNode(pXmlDoc, rXPath);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
        OString("XPath '" + rXPath + "' number of nodes is incorrect").getStr(),
        1, xmlXPathNodeSetGetLength(pXmlNodes));
    if (rAttribute.isEmpty())
        return OUString();
    xmlNodePtr pXmlNode = pXmlNodes->nodeTab[0];
    return OUString::createFromAscii((const char*)xmlGetProp(pXmlNode, BAD_CAST(rAttribute.getStr())));
}

namespace {

void testErrorBar( Reference< XPropertySet > xErrorBar )
{
    sal_Int32 nErrorBarStyle;
    CPPUNIT_ASSERT(
        xErrorBar->getPropertyValue("ErrorBarStyle") >>= nErrorBarStyle);
    CPPUNIT_ASSERT_EQUAL(nErrorBarStyle, chart::ErrorBarStyle::RELATIVE);
    bool bShowPositive = bool(), bShowNegative = bool();
    CPPUNIT_ASSERT(
        xErrorBar->getPropertyValue("ShowPositiveError") >>= bShowPositive);
    CPPUNIT_ASSERT(bShowPositive);
    CPPUNIT_ASSERT(
        xErrorBar->getPropertyValue("ShowNegativeError") >>= bShowNegative);
    CPPUNIT_ASSERT(bShowNegative);
    double nVal = 0.0;
    CPPUNIT_ASSERT(xErrorBar->getPropertyValue("PositiveError") >>= nVal);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(nVal, 10.0, 1e-10);
}

void checkCommonTrendline(
        Reference<chart2::XRegressionCurve> xCurve,
        double aExpectedExtrapolateForward, double aExpectedExtrapolateBackward,
        sal_Bool aExpectedForceIntercept, double aExpectedInterceptValue,
        sal_Bool aExpectedShowEquation, sal_Bool aExpectedR2)
{
    Reference<XPropertySet> xProperties( xCurve , uno::UNO_QUERY );
    CPPUNIT_ASSERT(xProperties.is());

    double aExtrapolateForward = 0.0;
    CPPUNIT_ASSERT(xProperties->getPropertyValue("ExtrapolateForward") >>= aExtrapolateForward);
    CPPUNIT_ASSERT_EQUAL(aExpectedExtrapolateForward, aExtrapolateForward);

    double aExtrapolateBackward = 0.0;
    CPPUNIT_ASSERT(xProperties->getPropertyValue("ExtrapolateBackward") >>= aExtrapolateBackward);
    CPPUNIT_ASSERT_EQUAL(aExpectedExtrapolateBackward, aExtrapolateBackward);

    sal_Bool aForceIntercept = false;
    CPPUNIT_ASSERT(xProperties->getPropertyValue("ForceIntercept") >>= aForceIntercept);
    CPPUNIT_ASSERT_EQUAL(aExpectedForceIntercept, aForceIntercept);

    if (aForceIntercept)
    {
        double aInterceptValue = 0.0;
        CPPUNIT_ASSERT(xProperties->getPropertyValue("InterceptValue") >>= aInterceptValue);
        CPPUNIT_ASSERT_EQUAL(aExpectedInterceptValue, aInterceptValue);
    }

    Reference< XPropertySet > xEquationProperties( xCurve->getEquationProperties() );
    CPPUNIT_ASSERT(xEquationProperties.is());

    sal_Bool aShowEquation = false;
    CPPUNIT_ASSERT(xEquationProperties->getPropertyValue("ShowEquation") >>= aShowEquation);
    CPPUNIT_ASSERT_EQUAL(aExpectedShowEquation, aShowEquation);

    sal_Bool aShowCorrelationCoefficient = false;
    CPPUNIT_ASSERT(xEquationProperties->getPropertyValue("ShowCorrelationCoefficient") >>= aShowCorrelationCoefficient);
    CPPUNIT_ASSERT_EQUAL(aExpectedR2, aShowCorrelationCoefficient);
}

void checkNameAndType(Reference<XPropertySet> xProperties, OUString aExpectedName, OUString aExpectedServiceName)
{
    Reference< lang::XServiceName > xServiceName( xProperties, UNO_QUERY );
    CPPUNIT_ASSERT(xServiceName.is());

    OUString aServiceName = xServiceName->getServiceName();
    CPPUNIT_ASSERT_EQUAL(aExpectedServiceName, aServiceName);

    OUString aCurveName;
    CPPUNIT_ASSERT(xProperties->getPropertyValue("CurveName") >>= aCurveName);
    CPPUNIT_ASSERT_EQUAL(aExpectedName, aCurveName);
}

void checkLinearTrendline(
        Reference<chart2::XRegressionCurve> xCurve, OUString aExpectedName,
        double aExpectedExtrapolateForward, double aExpectedExtrapolateBackward,
        sal_Bool aExpectedForceIntercept, double aExpectedInterceptValue,
        sal_Bool aExpectedShowEquation, sal_Bool aExpectedR2)
{
    Reference<XPropertySet> xProperties( xCurve , uno::UNO_QUERY );
    CPPUNIT_ASSERT(xProperties.is());

    checkNameAndType(xProperties, aExpectedName, "com.sun.star.chart2.LinearRegressionCurve");

    checkCommonTrendline(
        xCurve,
        aExpectedExtrapolateForward, aExpectedExtrapolateBackward,
        aExpectedForceIntercept, aExpectedInterceptValue,
        aExpectedShowEquation, aExpectedR2);
}

void checkPolynomialTrendline(
        Reference<chart2::XRegressionCurve> xCurve, OUString aExpectedName,
        sal_Int32 aExpectedDegree,
        double aExpectedExtrapolateForward, double aExpectedExtrapolateBackward,
        sal_Bool aExpectedForceIntercept, double aExpectedInterceptValue,
        sal_Bool aExpectedShowEquation, sal_Bool aExpectedR2)
{
    Reference<XPropertySet> xProperties( xCurve , uno::UNO_QUERY );
    CPPUNIT_ASSERT(xProperties.is());

    checkNameAndType(xProperties, aExpectedName, "com.sun.star.chart2.PolynomialRegressionCurve");

    sal_Int32 aDegree = 2;
    CPPUNIT_ASSERT(xProperties->getPropertyValue("PolynomialDegree") >>= aDegree);
    CPPUNIT_ASSERT_EQUAL(aExpectedDegree, aDegree);

    checkCommonTrendline(
        xCurve,
        aExpectedExtrapolateForward, aExpectedExtrapolateBackward,
        aExpectedForceIntercept, aExpectedInterceptValue,
        aExpectedShowEquation, aExpectedR2);
}

void checkMovingAverageTrendline(
        Reference<chart2::XRegressionCurve> xCurve, OUString aExpectedName, sal_Int32 aExpectedPeriod)
{
    Reference<XPropertySet> xProperties( xCurve , uno::UNO_QUERY );
    CPPUNIT_ASSERT(xProperties.is());

    checkNameAndType(xProperties, aExpectedName, "com.sun.star.chart2.MovingAverageRegressionCurve");

    sal_Int32 aPeriod = 2;
    CPPUNIT_ASSERT(xProperties->getPropertyValue("MovingAveragePeriod") >>= aPeriod);
    CPPUNIT_ASSERT_EQUAL(aExpectedPeriod, aPeriod);
}

void checkTrendlinesInChart(uno::Reference< chart2::XChartDocument > xChartDoc)
{
    CPPUNIT_ASSERT(xChartDoc.is());

    Reference< chart2::XDataSeries > xDataSeries = getDataSeriesFromDoc( xChartDoc, 0 );
    CPPUNIT_ASSERT( xDataSeries.is() );

    Reference< chart2::XRegressionCurveContainer > xRegressionCurveContainer( xDataSeries, UNO_QUERY );
    CPPUNIT_ASSERT( xRegressionCurveContainer.is() );

    Sequence< Reference< chart2::XRegressionCurve > > xRegressionCurveSequence = xRegressionCurveContainer->getRegressionCurves();
    CPPUNIT_ASSERT_EQUAL((sal_Int32) 3, xRegressionCurveSequence.getLength());

    Reference<chart2::XRegressionCurve> xCurve;

    xCurve = xRegressionCurveSequence[0];
    CPPUNIT_ASSERT(xCurve.is());
    checkPolynomialTrendline(xCurve, "col2_poly", 3, 0.1, -0.1, true, -1.0, true, true);

    xCurve = xRegressionCurveSequence[1];
    CPPUNIT_ASSERT(xCurve.is());
    checkLinearTrendline(xCurve, "col2_linear", -0.5, -0.5, false, 0.0, true, false);

    xCurve = xRegressionCurveSequence[2];
    CPPUNIT_ASSERT(xCurve.is());
    checkMovingAverageTrendline(xCurve, "col2_moving_avg", 3);
}

}

// improve the test
void Chart2ExportTest::testErrorBarXLSX()
{
    load("/chart2/qa/extras/data/ods/", "error_bar.ods");
    {
        // make sure the ODS import was successful
        uno::Reference< chart2::XChartDocument > xChartDoc = getChartDocFromSheet( 0, mxComponent );
        CPPUNIT_ASSERT(xChartDoc.is());

        Reference< chart2::XDataSeries > xDataSeries = getDataSeriesFromDoc( xChartDoc, 0 );
        CPPUNIT_ASSERT( xDataSeries.is() );

        Reference< beans::XPropertySet > xPropSet( xDataSeries, UNO_QUERY_THROW );
        CPPUNIT_ASSERT( xPropSet.is() );

        // test that y error bars are there
        Reference< beans::XPropertySet > xErrorBarYProps;
        xPropSet->getPropertyValue("ErrorBarY") >>= xErrorBarYProps;
        testErrorBar(xErrorBarYProps);
    }

    reload("Calc Office Open XML");
    {
        uno::Reference< chart2::XChartDocument > xChartDoc = getChartDocFromSheet( 0, mxComponent );
        CPPUNIT_ASSERT(xChartDoc.is());

        Reference< chart2::XDataSeries > xDataSeries = getDataSeriesFromDoc( xChartDoc, 0 );
        CPPUNIT_ASSERT( xDataSeries.is() );

        Reference< beans::XPropertySet > xPropSet( xDataSeries, UNO_QUERY_THROW );
        CPPUNIT_ASSERT( xPropSet.is() );

        // test that y error bars are there
        Reference< beans::XPropertySet > xErrorBarYProps;
        xPropSet->getPropertyValue("ErrorBarY") >>= xErrorBarYProps;
        testErrorBar(xErrorBarYProps);
    }
}

// This method tests the preservation of properties for trendlines / regression curves
// in an export -> import cycle using different file formats - ODS, XLS and XLSX.
void Chart2ExportTest::testTrendline()
{
    load("/chart2/qa/extras/data/ods/", "trendline.ods");
    checkTrendlinesInChart(getChartDocFromSheet( 0, mxComponent));
    reload("calc8");
    checkTrendlinesInChart(getChartDocFromSheet( 0, mxComponent));

    load("/chart2/qa/extras/data/ods/", "trendline.ods");
    checkTrendlinesInChart(getChartDocFromSheet( 0, mxComponent));
    reload("Calc Office Open XML");
    checkTrendlinesInChart(getChartDocFromSheet( 0, mxComponent));

    load("/chart2/qa/extras/data/ods/", "trendline.ods");
    checkTrendlinesInChart(getChartDocFromSheet( 0, mxComponent));
    reload("MS Excel 97");
    checkTrendlinesInChart(getChartDocFromSheet( 0, mxComponent));

}

void Chart2ExportTest::testStockChart()
{
    /*  For attached file Stock_Chart.docx, in chart1.xml,
     * <c:stockChart>, there are four types of series as
     * Open,Low,High and Close.
     * For Open series, in <c:idx val="0" />
     * an attribute val of index should start from 1 and not from 0.
     * Which was problem area.
     */
    load("/chart2/qa/extras/data/docx/", "testStockChart.docx");

    xmlDocPtr pXmlDoc = parseExport("word/charts/chart", "Office Open XML Text");
    if (!pXmlDoc)
       return;

    assertXPath(pXmlDoc, "/c:chartSpace/c:chart/c:plotArea/c:stockChart/c:ser[1]/c:idx", "val", "1");
    assertXPath(pXmlDoc, "/c:chartSpace/c:chart/c:plotArea/c:stockChart/c:ser[1]/c:order", "val", "1");
    assertXPath(pXmlDoc, "/c:chartSpace/c:chart/c:plotArea/c:stockChart/c:ser[1]/c:tx/c:strRef/c:strCache/c:pt/c:v", "Open");
}

void Chart2ExportTest::testBarChart()
{
    load("/chart2/qa/extras/data/docx/", "testBarChart.docx");
    xmlDocPtr pXmlDoc = parseExport("word/charts/chart", "Office Open XML Text");
    if (!pXmlDoc)
       return;

    assertXPath(pXmlDoc, "/c:chartSpace/c:chart/c:plotArea/c:barChart/c:barDir", "val", "col");
}

void Chart2ExportTest::testCrosses()
{
    load("/chart2/qa/extras/data/docx/", "Bar_horizontal_cone.docx");
    xmlDocPtr pXmlDoc = parseExport("word/charts/chart", "Office Open XML Text");

    assertXPath(pXmlDoc, "/c:chartSpace/c:chart/c:plotArea/c:catAx/c:crosses", "val", "autoZero");
}
void Chart2ExportTest::testChartDataTable()
{
    load("/chart2/qa/extras/data/docx/", "testChartDataTable.docx");

    xmlDocPtr pXmlDoc = parseExport("word/charts/chart", "Office Open XML Text");
    CPPUNIT_ASSERT(pXmlDoc);
    assertXPath(pXmlDoc, "/c:chartSpace/c:chart/c:plotArea/c:dTable/c:showHorzBorder", "val", "1");
    assertXPath(pXmlDoc, "/c:chartSpace/c:chart/c:plotArea/c:dTable/c:showVertBorder", "val", "1");
    assertXPath(pXmlDoc, "/c:chartSpace/c:chart/c:plotArea/c:dTable/c:showOutline", "val", "1");
}

CPPUNIT_TEST_SUITE_REGISTRATION(Chart2ExportTest);

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
