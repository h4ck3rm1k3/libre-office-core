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
#ifndef INCLUDED_CHART2_SOURCE_INC_LINEARREGRESSIONCURVECALCULATOR_HXX
#define INCLUDED_CHART2_SOURCE_INC_LINEARREGRESSIONCURVECALCULATOR_HXX

#include "RegressionCurveCalculator.hxx"
#include "PolynomialRegressionCurveCalculator.hxx"

namespace chart
{

class LinearRegressionCurveCalculator : public PolynomialRegressionCurveCalculator
{
public:
    LinearRegressionCurveCalculator();
    virtual ~LinearRegressionCurveCalculator();

private:
    virtual void SAL_CALL setRegressionProperties(
        sal_Int32 aDegree,
        sal_Bool  aForceIntercept,
        double    aInterceptValue,
        sal_Int32 aPeriod)
        throw (css::uno::RuntimeException);

    virtual css::uno::Sequence<css::geometry::RealPoint2D> SAL_CALL getCurveValues(
        double min,
        double max,
        sal_Int32 nPointCount,
        const css::uno::Reference<css::chart2::XScaling>& xScalingX,
        const css::uno::Reference<css::chart2::XScaling>& xScalingY,
        sal_Bool bMaySkipPointsInCalculation )
        throw (css::lang::IllegalArgumentException,
               css::uno::RuntimeException);
};

} //  namespace chart

// INCLUDED_CHART2_SOURCE_INC_LINEARREGRESSIONCURVECALCULATOR_HXX
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
