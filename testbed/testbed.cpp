// testbed.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"

#include <iostream>

#include <boost/timer.hpp>

#include <ql/instruments/swaption.hpp>
#include <ql/indexes/ibor/euribor.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/models/calibrationhelper.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/math/optimization/levenbergmarquardt.hpp>
#include <ql/pricingengines/swap/discountingswapengine.hpp>
#include <ql/pricingengines/swaption/jamshidianswaptionengine.hpp>
#include <ql/pricingengines/swaption/treeswaptionengine.hpp>

#include <ql/models/shortrate/calibrationhelpers/swaptionhelper.hpp>
#include <ql/models/shortrate/onefactormodels/hullwhite.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/auto_link.hpp>

#include <calibrator/models/shortrate/onefactormodels/generalhullwhite.hpp>
#include <calibrator/models/parameters/integrableconstant.hpp>
#include <calibrator/models/parameters/intagrablepiecewiseconstant.hpp>


using namespace std;
using namespace HJCALIBRATOR;

Size numRows = 10;
Size numCols = 10;

Integer swapLenghts[] = {
	1,     2,     3,     4,     5,	6,	7,	8,	9,	10 }; //swap ������ ����
Volatility swaptionVols[] = { //swaption vol ���� �ɼǸ���, ���� �ɼǸ��� 
	0.3027, 0.33395, 0.34935, 0.35325, 0.3876, 0.387, 0.388, 0.33615, 0.3314, 0.3308,
	0.3734, 0.3978, 0.3712, 0.3572, 0.3274, 0.33435, 0.3417, 0.3361, 0.32975, 0.3512,
	0.3813, 0.3635, 0.3574, 0.35075, 0.35045, 0.34025, 0.33575, 0.3414, 0.33925, 0.34045,
	0.3761, 0.3604, 0.3528, 0.3463, 0.34065, 0.3355, 0.3311, 0.33575, 0.335, 0.319,
	0.35855, 0.3498, 0.3442, 0.3391, 0.33395, 0.342, 0.3246, 0.3343, 0.33085, 0.31565,
	0.34615, 0.3418, 0.3379, 0.33265, 0.3264, 0.32305, 0.31775, 0.3145, 0.3111, 0.3077,
	0.3284, 0.32575, 0.323, 0.32335, 0.31865, 0.3145, 0.3116, 0.311, 0.3107, 0.30885,
	0.3206, 0.3081, 0.30345, 0.30635, 0.3104, 0.3064, 0.30235, 0.30305, 0.29755, 0.29525,
	0.3058, 0.30025, 0.2977, 0.30025, 0.30325, 0.29965, 0.29625, 0.29415, 0.29205, 0.29,
	0.30675, 0.28665, 0.2985, 0.29665, 0.28285, 0.2957, 0.28735, 0.29035, 0.28955, 0.28815 };

void calibrateModel(
	const boost::shared_ptr<ShortRateModel>& model,
	const std::vector<boost::shared_ptr<CalibrationHelper> >& helpers ) {

	LevenbergMarquardt om;
	model->calibrate( helpers, om,
					  EndCriteria( 400, 100, 1.0e-8, 1.0e-8, 1.0e-8 ) );

	// Output the implied Black volatilities
	for ( Size i = 0; i<numRows; i++ ) {
		Size j = numCols - i - 1; // 1x10, 2x8, 3x7, 4x6, 5x5, ...
		Size k = i * numCols + j;
		Real npv = helpers[i]->modelValue();
		Volatility implied = helpers[i]->impliedVolatility( npv, 1e-4,
															1000, 0.05, 0.50 );
		Volatility diff = implied - swaptionVols[k];
	}
}

int main()
{
	Date todaysDate( 25, April, 2017 ); //���� ��¥
	Calendar calendar = TARGET(); // ��� ���� ó��?
	Date settlementDate( 28, April, 2017 ); //settlementDate
	Settings::instance().evaluationDate() = todaysDate; //evaluationDate
														// flat yield term structure impling 1x10 swap at 2.224%
	boost::shared_ptr<Quote> flatRate( new SimpleQuote( 0.0222400 ) );
	Handle<YieldTermStructure> rhTermStructure(
		boost::shared_ptr<FlatForward>(
			new FlatForward( settlementDate, Handle<Quote>( flatRate ),
							 Actual365Fixed() ) ) ); // YieldTermStructure This abstract class defines the interface of concrete interest rate structures which will be derived from this one

	Frequency fixedLegFrequency = Annual; // fixedLegFrequency
	BusinessDayConvention fixedLegConvention = Unadjusted; //fixedLegConvention
	BusinessDayConvention floatingLegConvention = ModifiedFollowing; //floatingLegConvention
	DayCounter fixedLegDayCounter = Thirty360( Thirty360::European ); //fixedLegDayCounter
	Frequency floatingLegFrequency = Semiannual; //floatingLegFrequency
	VanillaSwap::Type type = VanillaSwap::Payer; //type = VanillaSwap::Payer
	Rate dummyFixedRate = 0.0222400; //FixedRate
	boost::shared_ptr<IborIndex> indexSixMonths( new
												 Euribor6M( rhTermStructure ) );

	Date startDate = calendar.advance( settlementDate, 1, Years,
									   floatingLegConvention );// swap startDate
	Date maturity = calendar.advance( startDate, 10, Years,
									  floatingLegConvention ); // swap maturity
	Schedule fixedSchedule( startDate, maturity, Period( fixedLegFrequency ),
							calendar, fixedLegConvention, fixedLegConvention,
							DateGeneration::Forward, false ); // fixedSchedule
	Schedule floatSchedule( startDate, maturity, Period( floatingLegFrequency ),
							calendar, floatingLegConvention, floatingLegConvention,
							DateGeneration::Forward, false ); // floatSchedule

	boost::shared_ptr<VanillaSwap> swap( new VanillaSwap( //VanillaSwap swap
														  type, 1000.0,
														  fixedSchedule, dummyFixedRate, fixedLegDayCounter,
														  floatSchedule, indexSixMonths, 0.0,
														  indexSixMonths->dayCounter() ) );
	swap->setPricingEngine( boost::shared_ptr<PricingEngine>(
		new DiscountingSwapEngine( rhTermStructure ) ) );
	Rate fixedATMRate = swap->fairRate();
	Rate fixedOTMRate = fixedATMRate * 1.2;
	Rate fixedITMRate = fixedATMRate * 0.8;

	boost::shared_ptr<VanillaSwap> atmSwap( new VanillaSwap(
		type, 1000.0,
		fixedSchedule, fixedATMRate, fixedLegDayCounter,
		floatSchedule, indexSixMonths, 0.0,
		indexSixMonths->dayCounter() ) );
	boost::shared_ptr<VanillaSwap> otmSwap( new VanillaSwap(
		type, 1000.0,
		fixedSchedule, fixedOTMRate, fixedLegDayCounter,
		floatSchedule, indexSixMonths, 0.0,
		indexSixMonths->dayCounter() ) );
	boost::shared_ptr<VanillaSwap> itmSwap( new VanillaSwap(
		type, 1000.0,
		fixedSchedule, fixedITMRate, fixedLegDayCounter,
		floatSchedule, indexSixMonths, 0.0,
		indexSixMonths->dayCounter() ) );

	// defining the swaptions to be used in model calibration
	std::vector<Period> swaptionMaturities; // Period swaptionMaturities
	swaptionMaturities.push_back( Period( 1, Years ) );
	swaptionMaturities.push_back( Period( 2, Years ) );
	swaptionMaturities.push_back( Period( 3, Years ) );
	swaptionMaturities.push_back( Period( 4, Years ) );
	swaptionMaturities.push_back( Period( 5, Years ) );
	swaptionMaturities.push_back( Period( 6, Years ) );
	swaptionMaturities.push_back( Period( 7, Years ) );
	swaptionMaturities.push_back( Period( 8, Years ) );
	swaptionMaturities.push_back( Period( 9, Years ) );
	swaptionMaturities.push_back( Period( 10, Years ) );

	std::vector<boost::shared_ptr<CalibrationHelper> > swaptions; // CalibrationHelper swaptions

																  // List of times that have to be included in the timegrid
	std::list<Time> times;

	Size i;
	for ( i = 0; i<numRows; i++ ) {
		//for ( Size j = 0; j < numRows; j++ ) {
			Size j = numCols - i - 1; // 1x10, 2x8, 3x7, 4x6, 5x5, ...
			Size k = i * numCols + j;
			boost::shared_ptr<Quote> vol( new SimpleQuote( swaptionVols[k] ) );
			swaptions.push_back( boost::shared_ptr<CalibrationHelper>( new
																	   SwaptionHelper( swaptionMaturities[i],
																					   Period( swapLenghts[j], Years ),
																					   Handle<Quote>( vol ),
																					   indexSixMonths,
																					   indexSixMonths->tenor(),
																					   indexSixMonths->dayCounter(),
																					   indexSixMonths->dayCounter(),
																					   rhTermStructure ) ) );
			swaptions.back()->addTimesTo( times ); //Returns a reference to the last element in the vector
		//}
	}

	LevenbergMarquardt om;

	/*
	// HW model (fixed parameter)
	boost::shared_ptr<HullWhite> hwmodel( new HullWhite( rhTermStructure ) );

	for ( i = 0; i < swaptions.size(); i++ )
		swaptions[i]->setPricingEngine( boost::shared_ptr<PricingEngine>(
			new JamshidianSwaptionEngine( hwmodel ) ) );

	hwmodel->calibrate( swaptions, om,
						EndCriteria( 400, 100, 1.0e-8, 1.0e-8, 1.0e-8 ) );

	std::cout << "calibrated to:\n"
		<< "a     = " << hwmodel->params()[0] << ", "
		<< "sigma = " << hwmodel->params()[1] << "\n"
		<< std::endl << std::endl;
    */
	// Generalized HW model

	cout << "====== Calibration with constant sigma ======" << endl;
	boost::shared_ptr<GeneralizedHullWhite> ghwmodel( new GeneralizedHullWhite( rhTermStructure,
																				IntegrableConstantParameter( 0.1, NoConstraint() ),
																				ConstantParameter( 0.01, NoConstraint() ) ) );

	for ( i = 0; i < swaptions.size(); i++ )
		swaptions[i]->setPricingEngine( boost::shared_ptr<PricingEngine>(
			new JamshidianSwaptionEngine( ghwmodel ) ) );

	try
	{
		ghwmodel->calibrate( swaptions, om,
							 EndCriteria( 10000, 100, 1.0e-8, 1.0e-8, 1.0e-8 ) );
	}
	catch ( exception& e )
	{
		cout << e.what() << endl;
	}

	std::cout << "calibrated to:\n"
		<< "a     = " << ghwmodel->params()[0] << ", "
		<< "sigma = " << ghwmodel->params()[1] << "\n"
		<< std::endl << std::endl;
	
	// Output the implied Black volatilities
	for ( Size i = 0; i < numRows; i++ ) {
		//for ( Size j = 0; j < numRows; j++ ) {
			Size j = numCols - i - 1; // 1x10, 2x8, 3x7, 4x6, 5x5, ...
			Size k = i * numCols + j;
			Real npv = swaptions[i]->modelValue();
			Volatility implied = swaptions[i]->impliedVolatility( npv, 1e-4,
																  1000, 0.05, 0.50 );
			Volatility diff = implied - swaptionVols[k];
			cout << "(" << i << "," << j << ") : " << diff << endl;
		//}
	}

	cout << "====== Calibration with piecewise-constant sigma ======" << endl;
	std::vector<Real> nodes = { 1, 3, 5, 10 };
	PiecewiseConstantParameter vol( nodes, BoundaryConstraint(0,1) );
	
	cout << vol.params().size() << endl;
	for ( Size i = 0; i <= nodes.size(); i++ )
	{
		vol.setParam( i, 0.01 );
	}
	
	boost::shared_ptr<GeneralizedHullWhite> ghwmodel2( new GeneralizedHullWhite( rhTermStructure,
																				 IntegrableConstantParameter( 0.1, NoConstraint() ),
																				 vol ) );

	for ( i = 0; i < swaptions.size(); i++ )
		swaptions[i]->setPricingEngine( boost::shared_ptr<PricingEngine>(
			new JamshidianSwaptionEngine( ghwmodel2 ) ) );

	try
	{
		ghwmodel2->calibrate( swaptions, om,
							  EndCriteria( 10000, 100, 1.0e-8, 1.0e-8, 1.0e-8 ) );
	}
	catch ( exception& e )
	{
		cout << e.what() << endl;
	}

	std::cout << "calibrated to:\n"
		<< "a     = " << ghwmodel2->params()[0] << ", "
		<< "sigma = " << ghwmodel2->params()[1] << "\n"
		<< std::endl << std::endl;

	// Output the implied Black volatilities
	for ( Size i = 0; i < numRows; i++ ) {
		//for ( Size j = 0; j < numRows; j++ ) {
			Size j = numCols - i - 1; // 1x10, 2x8, 3x7, 4x6, 5x5, ...
			Size k = i * numCols + j;
			Real npv = swaptions[i]->modelValue();
			Volatility implied = swaptions[i]->impliedVolatility( npv, 1e-4,
																  1000, 0.05, 0.50 );
			Volatility diff = implied - swaptionVols[k];

			cout << "(" << i << "," << j << ") : " << diff << endl;
		//}
	}


	return 0;
}

