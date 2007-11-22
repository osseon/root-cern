/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 * @(#)root/roofitcore:$Id$
 * Authors:                                                                  *
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu       *
 *   DK, David Kirkby,    UC Irvine,         dkirkby@uci.edu                 *
 *                                                                           *
 * Copyright (c) 2000-2005, Regents of the University of California          *
 *                          and Stanford University. All rights reserved.    *
 *                                                                           *
 * Redistribution and use in source and binary forms,                        *
 * with or without modification, are permitted according to the terms        *
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             *
 *****************************************************************************/

// -- CLASS DESCRIPTION [AUX] --
// RooIntegrator1D implements an adaptive one-dimensional 
// numerical integration algorithm.


#include "RooFit.h"

#include "TClass.h"
#include "RooIntegrator1D.h"
#include "RooArgSet.h"
#include "RooRealVar.h"
#include "RooNumber.h"
#include "RooIntegratorBinding.h"
#include "RooNumIntConfig.h"
#include "RooNumIntFactory.h"

#include <assert.h>

ClassImp(RooIntegrator1D)
;

// Register this class with RooNumIntConfig
void RooIntegrator1D::registerIntegrator(RooNumIntFactory& fact)
{
  RooCategory sumRule("sumRule","Summation Rule") ;
  sumRule.defineType("Trapezoid",RooIntegrator1D::Trapezoid) ;
  sumRule.defineType("Midpoint",RooIntegrator1D::Midpoint) ;
  sumRule.setLabel("Trapezoid") ;
  RooCategory extrap("extrapolation","Extrapolation procedure") ;
  extrap.defineType("None",0) ;
  extrap.defineType("Wynn-Epsilon",1) ;
  extrap.setLabel("Wynn-Epsilon") ;
  RooRealVar maxSteps("maxSteps","Maximum number of steps",20) ;
  RooRealVar minSteps("minSteps","Minimum number of steps",999) ;
  RooRealVar fixSteps("fixSteps","Fixed number of steps",0) ;

  RooIntegrator1D* proto = new RooIntegrator1D() ;
  fact.storeProtoIntegrator(proto,RooArgSet(sumRule,extrap,maxSteps,minSteps,fixSteps)) ;
  RooNumIntConfig::defaultConfig().method1D().setLabel(proto->IsA()->GetName()) ;
}


RooIntegrator1D::RooIntegrator1D() :
  _h(0), _s(0), _c(0), _d(0), _x(0)
{
}

RooIntegrator1D::RooIntegrator1D(const RooAbsFunc& function, SummationRule rule,
				 Int_t maxSteps, Double_t eps) : 
  RooAbsIntegrator(function), _rule(rule), _maxSteps(maxSteps),  _minStepsZero(999), _fixSteps(0), _epsAbs(eps), _epsRel(eps), _doExtrap(kTRUE)
{
  // Use this form of the constructor to integrate over the function's default range.

  _useIntegrandLimits= kTRUE;
  _valid= initialize();
} 

RooIntegrator1D::RooIntegrator1D(const RooAbsFunc& function, Double_t xmin, Double_t xmax,
				 SummationRule rule, Int_t maxSteps, Double_t eps) : 
  RooAbsIntegrator(function), 
  _rule(rule), 
  _maxSteps(maxSteps), 
  _minStepsZero(999), 
  _fixSteps(0),
  _epsAbs(eps), 
  _epsRel(eps),
  _doExtrap(kTRUE)
{
  // Use this form of the constructor to override the function's default range.

  _useIntegrandLimits= kFALSE;
  _xmin= xmin;
  _xmax= xmax;
  _valid= initialize();
} 

RooIntegrator1D::RooIntegrator1D(const RooAbsFunc& function, const RooNumIntConfig& config) :
  RooAbsIntegrator(function,config.printEvalCounter()), 
  _epsAbs(config.epsAbs()),
  _epsRel(config.epsRel())
{
  // Use this form of the constructor to integrate over the function's default range.

  // Extract parameters from config object
  const RooArgSet& configSet = config.getConfigSection(IsA()->GetName()) ;  
  _rule = (SummationRule) configSet.getCatIndex("sumRule",Trapezoid) ;
  _maxSteps = (Int_t) configSet.getRealValue("maxSteps",20) ;
  _minStepsZero = (Int_t) configSet.getRealValue("minSteps",999) ;
  _fixSteps = (Int_t) configSet.getRealValue("fixSteps",0) ;
  _doExtrap = (Bool_t) configSet.getCatIndex("extrapolation",1) ;

  if (_fixSteps>_maxSteps) {
    cout << "RooIntegrator1D::ctor() ERROR: fixSteps>maxSteps, fixSteps set to maxSteps" << endl ;
    _fixSteps = _maxSteps ;
  }

  _useIntegrandLimits= kTRUE;
  _valid= initialize();
} 


RooIntegrator1D::RooIntegrator1D(const RooAbsFunc& function, Double_t xmin, Double_t xmax,
				const RooNumIntConfig& config) :
  RooAbsIntegrator(function,config.printEvalCounter()), 
  _epsAbs(config.epsAbs()),
  _epsRel(config.epsRel())
{
  // Use this form of the constructor to override the function's default range.

  // Extract parameters from config object
  const RooArgSet& configSet = config.getConfigSection(IsA()->GetName()) ;  
  _rule = (SummationRule) configSet.getCatIndex("sumRule",Trapezoid) ;
  _maxSteps = (Int_t) configSet.getRealValue("maxSteps",20) ;
  _minStepsZero = (Int_t) configSet.getRealValue("minSteps",999) ;
  _fixSteps = (Int_t) configSet.getRealValue("fixSteps",0) ;  
  _doExtrap = (Bool_t) configSet.getCatIndex("extrapolation",1) ;

  _useIntegrandLimits= kFALSE;
  _xmin= xmin;
  _xmax= xmax;
  _valid= initialize();
} 



RooAbsIntegrator* RooIntegrator1D::clone(const RooAbsFunc& function, const RooNumIntConfig& config) const
{
  return new RooIntegrator1D(function,config) ;
}


Bool_t RooIntegrator1D::initialize()
{
  // apply defaults if necessary
  if(_maxSteps <= 0) {
    _maxSteps= (_rule == Trapezoid) ? 20 : 14;
  }

  if(_epsRel <= 0) _epsRel= 1e-6;
  if(_epsAbs <= 0) _epsAbs= 1e-6;

  // check that the integrand is a valid function
  if(!isValid()) {
    cout << "RooIntegrator1D::initialize: cannot integrate invalid function" << endl;
    return kFALSE;
  }

//   // check that the function is one dimensional
//   if(_function->getDimension() != 1) {
//     cout << "RooIntegrator1D::initialize: cannot integrate function of dimension "
// 	 << _function->getDimension() << endl;
//     return kFALSE;
//   }

  // Allocate coordinate buffer size after number of function dimensions
  _x = new Double_t[_function->getDimension()] ;


  // Allocate workspace for numerical integration engine
  _h= new Double_t[_maxSteps + 2];
  _s= new Double_t[_maxSteps + 2];
  _c= new Double_t[_nPoints + 1];
  _d= new Double_t[_nPoints + 1];

  return checkLimits();
}

RooIntegrator1D::~RooIntegrator1D()
{
  // Release integrator workspace
  if(_h) delete[] _h;
  if(_s) delete[] _s;
  if(_c) delete[] _c;
  if(_d) delete[] _d;
  if(_x) delete[] _x;
}

Bool_t RooIntegrator1D::setLimits(Double_t xmin, Double_t xmax) {
  // Change our integration limits. Return kTRUE if the new limits are
  // ok, or otherwise kFALSE. Always returns kFALSE and does nothing
  // if this object was constructed to always use our integrand's limits.

  if(_useIntegrandLimits) {
    cout << "RooIntegrator1D::setLimits: cannot override integrand's limits" << endl;
    return kFALSE;
  }
  _xmin= xmin;
  _xmax= xmax;
  return checkLimits();
}

Bool_t RooIntegrator1D::checkLimits() const {
  // Check that our integration range is finite and otherwise return kFALSE.
  // Update the limits from the integrand if requested.

  if(_useIntegrandLimits) {
    assert(0 != integrand() && integrand()->isValid());
    _xmin= integrand()->getMinLimit(0);
    _xmax= integrand()->getMaxLimit(0);
  }
  _range= _xmax - _xmin;
  if(_range <= 0) {
    cout << "RooIntegrator1D::checkLimits: bad range with min >= max" << endl;
    return kFALSE;
  }
  return (RooNumber::isInfinite(_xmin) || RooNumber::isInfinite(_xmax)) ? kFALSE : kTRUE;
}

Double_t RooIntegrator1D::integral(const Double_t *yvec) 
{
  assert(isValid());

  // Copy yvec to xvec if provided
  if (yvec) {
    UInt_t i ; for (i=0 ; i<_function->getDimension()-1 ; i++) {
      _x[i+1] = yvec[i] ;
    }
  }

  Int_t j;
  _h[1]=1.0;
  Double_t zeroThresh = _epsAbs/_range ;
  for(j= 1; j<=_maxSteps; j++) {
    // refine our estimate using the appropriate summation rule
    _s[j]= (_rule == Trapezoid) ? addTrapezoids(j) : addMidpoints(j);

    if (j >= _minStepsZero) {
      Bool_t allZero(kTRUE) ;
      Int_t jj ; for (jj=0 ; jj<=j ; jj++) {	
	if (_s[j]>=zeroThresh) {
	  allZero=kFALSE ;
	}
      }
      if (allZero) {
	//cout << "Roo1DIntegrator(" << this << "): zero convergence at step " << j << ", value = " << 0 << endl ;
	return 0;
      }
    }
    
    if (_fixSteps>0) {
      
      // Fixed step mode, return result after fixed number of steps
      if (j==_fixSteps) {
	//cout << "returning result at fixed step " << j << endl ;
	return _s[j];
      }

    } else  if(j >= _nPoints) {

      // extrapolate the results of recent refinements and check for a stable result
      if (_doExtrap) {
	extrapolate(j);
      } else {
	_extrapValue = _s[j] ;
	_extrapError = _s[j]-(j>0?_s[j-1]:0) ;
      }

      if(fabs(_extrapError) <= _epsRel*fabs(_extrapValue)) {
	return _extrapValue;
      }
      if(fabs(_extrapError) <= _epsAbs) {
	return _extrapValue ;
      }

    }
    // update the step size for the next refinement of the summation
    _h[j+1]= (_rule == Trapezoid) ? _h[j]/4. : _h[j]/9.;
  }

  cout << "RooIntegrator1D::integral: integral over range (" << _xmin << "," << _xmax << ") did not converge after " 
       << _maxSteps << " steps" << endl;
  for(j= 1; j <= _maxSteps; j++) {
    cout << "   [" << j << "] h = " << _h[j] << " , s = " << _s[j] << endl;
  }
  return 0;
}

Double_t RooIntegrator1D::addMidpoints(Int_t n)
{
  // Calculate the n-th stage of refinement of the Second Euler-Maclaurin
  // summation rule which has the useful property of not evaluating the
  // integrand at either of its endpoints but requires more function
  // evaluations than the trapezoidal rule. This rule can be used with
  // a suitable change of variables to estimate improper integrals.

  Double_t x,tnm,sum,del,ddel;
  Int_t it,j;

  if(n == 1) {
    Double_t xmid= 0.5*(_xmin + _xmax);
    return (_savedResult= _range*integrand(xvec(xmid)));
  }
  else {
    for(it=1, j=1; j < n-1; j++) it*= 3;
    tnm= it;
    del= _range/(3.*tnm);
    ddel= del+del;
    x= _xmin + 0.5*del;
    for(sum= 0, j= 1; j <= it; j++) {
      sum+= integrand(xvec(x));
      x+= ddel;
      sum+= integrand(xvec(x));
      x+= del;
    }      
    return (_savedResult= (_savedResult + _range*sum/tnm)/3.);
  }
}

Double_t RooIntegrator1D::addTrapezoids(Int_t n)
{
  // Calculate the n-th stage of refinement of the extended trapezoidal
  // summation rule. This is the most efficient rule for a well behaved
  // integrand that can be evaluated over its entire range, including the
  // endpoints.

  Double_t x,tnm,sum,del;
  Int_t it,j;

  if (n == 1) {
    // use a single trapezoid to cover the full range
//     cout << "addTrap[" << n << "] count = 2 " << endl ;
    return (_savedResult= 0.5*_range*(integrand(xvec(_xmin)) + integrand(xvec(_xmax))));
  }
  else {
    // break the range down into several trapezoids using 2**(n-2)
    // equally-spaced interior points
    for(it=1, j=1; j < n-1; j++) it <<= 1;
    tnm= it;
    del= _range/tnm;
    x= _xmin + 0.5*del;
//     cout << "addTrap[" << n << "] count = " << it << endl ;
    for(sum=0.0, j=1; j<=it; j++, x+=del) sum += integrand(xvec(x));
    return (_savedResult= 0.5*(_savedResult + _range*sum/tnm));
  }
}


void RooIntegrator1D::extrapolate(Int_t n)
{
  Double_t *xa= &_h[n-_nPoints];
  Double_t *ya= &_s[n-_nPoints];
  Int_t i,m,ns=1;
  Double_t den,dif,dift,ho,hp,w;

  dif=fabs(xa[1]);
  for (i= 1; i <= _nPoints; i++) {
    if ((dift=fabs(xa[i])) < dif) {
      ns=i;
      dif=dift;
    }
    _c[i]=ya[i];
    _d[i]=ya[i];
  }
  _extrapValue= ya[ns--];
  for(m= 1; m < _nPoints; m++) {
    for(i= 1; i <= _nPoints-m; i++) {
      ho=xa[i];
      hp=xa[i+m];
      w=_c[i+1]-_d[i];
      if((den=ho-hp) == 0.0) {
	cout << "RooIntegrator1D::extrapolate: internal error" << endl;
      }
      den=w/den;
      _d[i]=hp*den;
      _c[i]=ho*den;
    }
    _extrapValue += (_extrapError=(2*ns < (_nPoints-m) ? _c[ns+1] : _d[ns--]));
  }
}
