// -*- mode: c++; c-basic-offset:8; -*-
//
// Package:    DBDump
// Class:      DBDump
// 
/**\class DBDump DBDump.cc MyPackage/DBDump/src/DBDump.cc

Description: <one line class summary>

Implementation:
<Notes on implementation>
 */
//
// Original Author:  Federico FERRI
//         Created:  Thu Jun 25 15:39:48 CEST 2009
// $Id: DBDump.cc,v 1.18 2011/08/15 06:54:32 ferriff Exp $
//
//


// system include files
#include <memory>
#include <fstream>
#include <string>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ServiceRegistry/interface/Service.h"

#include "CondFormats/EcalObjects/interface/EcalADCToGeVConstant.h"
#include "CondFormats/EcalObjects/interface/EcalChannelStatus.h"
#include "CondFormats/EcalObjects/interface/EcalGainRatios.h"
#include "CondFormats/EcalObjects/interface/EcalIntercalibConstants.h"
#include "CondFormats/EcalObjects/interface/EcalIntercalibConstantsMC.h"
#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatios.h"
#include "CondFormats/EcalObjects/interface/EcalPedestals.h"
#include "CondFormats/EcalObjects/interface/EcalTimeCalibConstants.h"

#include "CondFormats/DataRecord/interface/EcalADCToGeVConstantRcd.h"
#include "CondFormats/DataRecord/interface/EcalChannelStatusRcd.h"
#include "CondFormats/DataRecord/interface/EcalGainRatiosRcd.h"
#include "CondFormats/DataRecord/interface/EcalIntercalibConstantsMCRcd.h"
#include "CondFormats/DataRecord/interface/EcalIntercalibConstantsRcd.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRcd.h"
#include "CondFormats/DataRecord/interface/EcalPedestalsRcd.h"
#include "CondFormats/DataRecord/interface/EcalTimeCalibConstantsRcd.h"

#include "CalibCalorimetry/EcalLaserCorrection/interface/EcalLaserDbService.h"
#include "CalibCalorimetry/EcalLaserCorrection/interface/EcalLaserDbRecord.h"

#include "Geometry/Records/interface/CaloTopologyRecord.h"
//#include "Geometry/Records/interface/IdealGeometryRecord.h"
//#include "Geometry/EcalMapping/interface/EcalMappingRcd.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"

#include "CalibCalorimetry/EcalLaserAnalyzer/interface/MEEBGeom.h"
#include "CalibCalorimetry/EcalLaserAnalyzer/interface/MEEEGeom.h"

#include <Geometry/EcalMapping/interface/EcalElectronicsMapping.h>
#include <Geometry/EcalMapping/interface/EcalMappingRcd.h>

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "usercode/DBDump/interface/HistoManager.h"
#include "usercode/DBDump/interface/Quantile.h"
#include "usercode/DBDump/interface/EcalLaserPlotter.h"

#include <iostream>
#include <fstream>

#include "TProfile.h"
#include "TProfile2D.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TGraph.h"
#include "TGraphAsymmErrors.h"

class DBDump : public edm::EDAnalyzer {
   public:
      explicit DBDump(const edm::ParameterSet&);
      ~DBDump();
      void setDumpFalse();
      void setPlotFalse();

   private:
      virtual void beginJob() ;
      virtual void analyze(const edm::Event&, const edm::EventSetup&);
      //virtual void beginRun(const edm::Event&, const edm::EventSetup&);
      //virtual void endRun(const edm::Event&, const edm::EventSetup&);
      virtual void endJob() ;

      void printSummary();

      // output files
      bool outPlot_;
      bool outDump_;
      std::string outDumpFile_;
      std::string outPlotFile_;
      std::ofstream ofile_;

      // inter-calibration constants
      bool dumpIC_;
      bool plotIC_;
      edm::ESHandle<EcalIntercalibConstants> ic_;
      edm::ESGetToken<EcalIntercalibConstants, EcalIntercalibConstantsRcd> icToken_;
      edm::ESHandle<EcalIntercalibConstantsMC> icMC_;
      edm::ESGetToken<EcalIntercalibConstantsMC, EcalIntercalibConstantsMCRcd> icMCToken_;

      // time-calibration constants
      bool dumpTC_;
      bool plotTC_;
      edm::ESHandle<EcalTimeCalibConstants> tc_;
      edm::ESGetToken<EcalTimeCalibConstants, EcalTimeCalibConstantsRcd> tcToken_;

      // ADCToGeV constant
      bool dumpADCToGeV_;
      edm::ESHandle<EcalADCToGeVConstant> adcToGeV_;
      edm::ESGetToken<EcalADCToGeVConstant, EcalADCToGeVConstantRcd> adcToGeVToken_;

      // laser transparency measurements
      bool dumpTransp_;
      bool plotTransp_;
      edm::ESHandle<EcalLaserAPDPNRatios> apdpn_;
      edm::ESGetToken<EcalLaserAPDPNRatios, EcalLaserAPDPNRatiosRcd> apdpnToken_;

      // laser transparency corrections
      bool dumpTranspCorr_;
      bool plotTranspCorr_;
      edm::ESHandle<EcalLaserDbService> laser_;
      edm::ESGetToken<EcalLaserDbService, EcalLaserDbRecord> laserToken_;

      // channel status map
      bool dumpChStatus_;
      bool plotChStatus_;
      edm::ESHandle<EcalChannelStatus> chStatus_;
      edm::ESGetToken<EcalChannelStatus, EcalChannelStatusRcd> chStatusToken_;

      // pedestals
      bool dumpPedestals_;
      bool plotPedestals_;
      edm::ESHandle<EcalPedestals> ped_;
      edm::ESGetToken<EcalPedestals, EcalPedestalsRcd> pedToken_;

      // gain ratios
      bool dumpGainRatios_;
      bool plotGainRatios_;
      edm::ESHandle<EcalGainRatios> gr_;
      edm::ESGetToken<EcalGainRatios, EcalGainRatiosRcd> grToken_;

      HistoManager histos;

      //edm::ESHandle<CaloGeometry> caloGeometry_;
      //const CaloGeometry * geo_;

      std::vector<DetId> ecalDetIds_;

      EcalLaserPlotter lp_;

      // number of IOVs, first and last
      int niov_;
      time_t iov_first_;
      time_t iov_last_;

      // job start and stop time
      time_t jstart_;
      time_t jstop_;

      time_t il_;

      bool first_;

      FILE * fgeom;
};



DBDump::DBDump(const edm::ParameterSet& ps) :
        niov_(0),
        iov_first_(-1),
        iov_last_(0),
        jstart_(time(NULL)),
        jstop_(0),
        il_(0),
        first_(false) // do not pass if not needed
{
        icToken_       = esConsumes<EcalIntercalibConstants, EcalIntercalibConstantsRcd>();
        icMCToken_     = esConsumes<EcalIntercalibConstantsMC, EcalIntercalibConstantsMCRcd>();
        tcToken_       = esConsumes<EcalTimeCalibConstants, EcalTimeCalibConstantsRcd>();
        adcToGeVToken_ = esConsumes<EcalADCToGeVConstant, EcalADCToGeVConstantRcd>();
        apdpnToken_    = esConsumes<EcalLaserAPDPNRatios, EcalLaserAPDPNRatiosRcd>();
        laserToken_    = esConsumes<EcalLaserDbService, EcalLaserDbRecord>();
        chStatusToken_ = esConsumes<EcalChannelStatus, EcalChannelStatusRcd>();
        pedToken_      = esConsumes<EcalPedestals, EcalPedestalsRcd>();
        grToken_       = esConsumes<EcalGainRatios, EcalGainRatiosRcd>();

	outPlot_        = ps.getParameter<bool>("outPlot");
	outDump_        = ps.getParameter<bool>("outDump");
	outDumpFile_    = ps.getParameter<std::string>("outDumpFile");
	outPlotFile_    = ps.getParameter<std::string>("outPlotFile");
	// inter-calibration constants
	dumpIC_         = ps.getParameter<bool>("dumpIC");
	plotIC_         = ps.getParameter<bool>("plotIC");
	// time-calibration constants
	dumpTC_         = ps.getParameter<bool>("dumpTC");
	plotTC_         = ps.getParameter<bool>("plotTC");
	// ADCToGeV constant
	dumpADCToGeV_   = ps.getParameter<bool>("dumpADCToGeV");
	// laser transparency measurements
	dumpTransp_     = ps.getParameter<bool>("dumpTransp");
	plotTransp_     = ps.getParameter<bool>("plotTransp");
	// channel status map
	dumpChStatus_   = ps.getParameter<bool>("dumpChStatus");
	plotChStatus_   = ps.getParameter<bool>("plotChStatus");
	// pedestals
	dumpPedestals_  = ps.getParameter<bool>("dumpPedestals");
	plotPedestals_  = ps.getParameter<bool>("plotPedestals");
	// gain ratios
	dumpGainRatios_ = ps.getParameter<bool>("dumpGainRatios");
	plotGainRatios_ = ps.getParameter<bool>("plotGainRatios");
	// laser transparency corrections
	dumpTranspCorr_ = ps.getParameter<bool>("dumpTranspCorr");
	plotTranspCorr_ = ps.getParameter<bool>("plotTranspCorr");

	if (outDumpFile_ != "") {
		ofile_.open(outDumpFile_.c_str(), std::ios::out);
		if (!ofile_.is_open()) {
			edm::LogError("DBDump") << "DBDump::DBDump cannot open output file " << outDumpFile_;
			setDumpFalse();
		}
	} else if (outDump_) {
		edm::LogError("DBDump") << "DBDump::DBDump mismatch in your config file, setting outDump to false.";
		setDumpFalse();
	} else {
		setDumpFalse();
	}

	if (outPlot_) {
		histos.addTemplate<TH2D>( "EBh2", new TH2D( "EB", "EB", 360, 0.5, 360.5, 171, -85.5, 85.5 ) );
		histos.addTemplate<TH2D>( "EEh2", new TH2D( "EE", "EE", 100, 0., 100., 100, 0., 100. ) );
		histos.addTemplate<TProfile2D>( "EBprof2", new TProfile2D( "EB", "EB", 360, 0.5, 360.5, 171, -85.5, 85.5 ) );
		histos.addTemplate<TProfile2D>( "EEprof2", new TProfile2D( "EE", "EE", 100, 0., 100., 100, 0., 100. ) );
		histos.addTemplate<TProfile>( "EEprof", new TProfile( "EE", "EE", 55, 0., 55.) );
		histos.addTemplate<TProfile>( "EBprof", new TProfile( "EB", "EB", 171, -85.5, 85.5) );
		histos.addTemplate<TGraphAsymmErrors>("history", new TGraphAsymmErrors());
		histos.addTemplate<TH1D>("distr", new TH1D("distribution", "distribution", 2000, 0., 2.));
		histos.addTemplate<TProfile>("etaProf", new TProfile("etaProf", "etaProf", 250, -2.75, 2.75));
	} else {
		setPlotFalse();
	}

	// initialise ECAL DetId vector
	for (int hi = EBDetId::MIN_HASH; hi <= EBDetId::MAX_HASH; ++hi ) {
		EBDetId ebId = EBDetId::unhashIndex(hi);
		if (ebId != EBDetId()) {
			ecalDetIds_.push_back(ebId);
		}
	}
	for ( int hi = 0; hi < EEDetId::kSizeForDenseIndexing; ++hi ) {
		EEDetId eeId = EEDetId::unhashIndex(hi);
		if (eeId != EEDetId()) {
			ecalDetIds_.push_back(eeId);
		}
	}
	assert(ecalDetIds_.size() == 75848);
}

void DBDump::setDumpFalse()
{
	dumpIC_         = false;
	dumpTC_         = false;
	dumpADCToGeV_   = false;
	dumpTransp_     = false;
	dumpTranspCorr_ = false;
	dumpChStatus_   = false;
	dumpPedestals_  = false;
}

void DBDump::setPlotFalse()
{
	plotIC_         = false;
	plotTC_         = false;
	plotTransp_     = false;
	plotTranspCorr_ = false;
	plotChStatus_   = false;
        plotPedestals_  = false;
}


DBDump::~DBDump()
{
        if (outPlot_) {
                histos.save(outPlotFile_.c_str());
                lp_.save(outPlotFile_.c_str(), "UPDATE");
        }
        if (ofile_.is_open()) {
                ofile_.close();
        }
        jstop_ = time(NULL);
        printSummary();
}

void DBDump::printSummary()
{
	printf("%d IOV(s) analysed in %ld s (%.2f h).\n", niov_, jstop_ - jstart_, (float)(jstop_ - jstart_) / 3600.);
	char buf[128];
	strftime(buf, sizeof(buf), "%F %R:%S", gmtime(&iov_first_));
	printf("First IOV: %ld (%s UTC)\n", iov_first_, buf);
	strftime(buf, sizeof(buf), "%F %R:%S", gmtime(&iov_last_));
	printf(" Last IOV: %ld (%s UTC)\n", iov_last_, buf);
}

// ------------ method called to for each event  ------------
void
DBDump::analyze(const edm::Event& ev, const edm::EventSetup& es)
{
        //es.get<EcalTimeCalibConstantsRcd>().get( tc_ );
        //es.get<CaloGeometryRecord>().get(caloGeometry_);
        //geo_ = caloGeometry_.product();
        //// uncomment below if needed
        //edm::ESHandle< EcalElectronicsMapping > ecalmapping;
        //es.get<EcalMappingRcd>().get(ecalmapping);

        iov_last_ = ev.time().unixTime();
        if (iov_first_ == -1) iov_first_ = iov_last_;

        if (iov_last_ - il_ > 3600 * 24 * 7) {
                il_ = iov_last_;
        }

        bool atLeastOneDump = dumpIC_ || dumpChStatus_ || dumpPedestals_ || dumpTransp_ || dumpTranspCorr_;
        bool atLeastOnePlot = plotIC_ || plotChStatus_ || plotPedestals_ || plotTransp_ || plotTranspCorr_;
        if (atLeastOneDump || atLeastOnePlot) {

                if (dumpIC_ || plotIC_) {
                        ic_   = es.getHandle(icToken_);
                        icMC_ = es.getHandle(icMCToken_);
                }
                if (dumpChStatus_   || plotChStatus_ || dumpTransp_ || plotTransp_)   chStatus_ = es.getHandle(chStatusToken_);
                if (dumpPedestals_  || plotPedestals_)  ped_ = es.getHandle(pedToken_);
                if (dumpGainRatios_ || plotGainRatios_) gr_ = es.getHandle(grToken_);
                if (dumpTransp_     || plotTransp_)     apdpn_ = es.getHandle(apdpnToken_);
                if (dumpTranspCorr_ || plotTranspCorr_) laser_ = es.getHandle(laserToken_);

                // if you need to average and use the average later on...
                if (first_) {
                        fgeom = fopen("detid_geom.dat", "w");
                }
                if (dumpTranspCorr_ || plotTranspCorr_) {
                        lp_.setEcalChannelStatus(*chStatus_.product());
                        lp_.fill((*apdpn_.product()), iov_last_);
                }
                if (first_) {
                        fclose(fgeom);
                        first_ = false;
                }
       
                EcalIntercalibConstantMap::const_iterator itIC;
                EcalIntercalibConstantMap::const_iterator itICMC;
                EcalChannelStatusMap::const_iterator itChStatus;
                EcalPedestalsMap::const_iterator itPed;
                EcalGainRatioMap::const_iterator itGR;
                EcalLaserAPDPNRatios::EcalLaserAPDPNRatiosMap::const_iterator itAPDPN;
                EcalLaserAPDPNRatios::EcalLaserTimeStamp ts;

                char ecalPart[3] = "E*";
                //char eename[6] = "*Z:d*";
                char eename[3] = "*Z";

                char str[128];

                for (size_t iid = 0; iid < ecalDetIds_.size(); ++iid) {
                        DetId id(ecalDetIds_[iid]);

                        //  ix = ieta() for EB, ix() for EE
                        //  iy = iphi() for EB, iy() for EE
                        int ix = -1, iy = -1, iz = -1, r = -1;
                        int isEB = 0;

                        if (id.subdetId() == EcalBarrel) {
                                ecalPart[1] = 'B';
                                ix = EBDetId(id).ieta();
                                iy = EBDetId(id).iphi();
                                iz = 0;
                                isEB = 1;
                        } else if (id.subdetId() == EcalEndcap) {
                                ecalPart[1] = 'E';
                                ix = EEDetId(id).ix();
                                iy = EEDetId(id).iy();
                                iz = EEDetId(id).zside();
                                r = sqrt((ix - 50) * (ix - 50) + (iy - 50) * (iy - 50));
                                if (iz > 0) {
                                        eename[0] = 'p';
                                        //eename[4] = ix < 50 ? '1' : '2';
                                } else {
                                        eename[0] = 'n';
                                        //eename[4] = ix < 50 ? '3' : '4';
                                }

                        }

                        if (dumpIC_ || plotIC_) {
                                itIC = ic_->find(id);
                                itICMC = icMC_->find(id);
                        }
                        if (dumpChStatus_ || plotChStatus_)     itChStatus = chStatus_->find(id);
                        if (dumpPedestals_ || plotPedestals_)   itPed = ped_->find(id);
                        if (dumpGainRatios_ || plotGainRatios_) itGR = gr_->find(id);
                        if (dumpTransp_ || plotTransp_) {
                                size_t iLM = 0;
                                itAPDPN = apdpn_->getLaserMap().find(id);
                                if (id.subdetId()==EcalBarrel) {
                                        EBDetId ebid( id.rawId() );
                                        iLM = MEEBGeom::lmr(ebid.ieta(), ebid.iphi());
                                } else if (id.subdetId()==EcalEndcap) {
                                        EEDetId eeid( id.rawId() );
                                        // SuperCrystal coordinates
                                        MEEEGeom::SuperCrysCoord iX = (eeid.ix()-1)/5 + 1;
                                        MEEEGeom::SuperCrysCoord iY = (eeid.iy()-1)/5 + 1;    
                                        iLM = MEEEGeom::lmr(iX, iY, eeid.zside());    
                                }
                                if ( iLM-1 < apdpn_->getTimeMap().size() ) {
                                        ts = apdpn_->getTimeMap()[iLM];
                                }
                        }
                        float transpCorr = -1234567890.;
                        if (dumpTranspCorr_ || plotTranspCorr_) {
                                transpCorr = laser_->getLaserCorrection(id, ev.time());
                        }

                        if (atLeastOneDump) {
                                ofile_ << ecalPart << "  rawId= " << id.rawId()
                                        << "  ieta/ix= " << ix
                                        << "  iphi/iy= " << iy
                                        << "  0/iz= " << iz;
                        }
                        if (dumpIC_) {
                                if(itIC != ic_->end()) ofile_ << "  ic= " << *itIC;
                                else                   ofile_ << "  ic= " << "NA";
                                if(itICMC != icMC_->end()) ofile_ << "  icMC= " << *itIC;
                                else                       ofile_ << "  icMC= " << "NA";
                        }
                        if (dumpChStatus_) {
                                if(itChStatus != chStatus_->end()) ofile_ << "  chSt= " << (*itChStatus).getStatusCode();
                                else                               ofile_ << "  chSt= " << "NA";
                        }
                        if (dumpPedestals_) {
                                if(itPed != ped_->end()) {
                                        ofile_ << "  ped_g12_g6_g1= " << (*itPed).mean(1) << "_" << (*itPed).mean(2) << "_" << (*itPed).mean(3);
                                        ofile_ << "  ped_rms_g12_g6_g1= " << (*itPed).rms(1) << "_" << (*itPed).rms(2) << "_" << (*itPed).rms(3);
                                } else  ofile_ << "  ped= " << "NA";
                        }
                        if (dumpGainRatios_) {
                                if (itGR != gr_->end()) {
                                        ofile_ << "  gr_12/6_6/1= " << (*itGR).gain12Over6() << "_" << (*itGR).gain6Over1();
                                }
                        }
                        if (dumpTransp_) {
                                if ( itAPDPN != apdpn_->getLaserMap().end() ) {
                                        ofile_ << "  evtTS_t1_t2_t3_p1_p2_p3= " 
                                                << ev.time().value() 
                                                << "_" << ts.t1.value() << "_" << ts.t2.value() 
                                                << "_" << ts.t3.value() 
                                                << "_" << (*itAPDPN).p1 << "_" << (*itAPDPN).p2 
                                                << "_" << (*itAPDPN).p3
                                                ;
                                }
                        }
                        if (dumpTranspCorr_) {
                                ofile_ << "  transpCorr= " 
                                        << transpCorr;
                        }
                        if (atLeastOneDump) ofile_ << "\n";

                        // plotting:
                        if (plotIC_) {
                                if (itIC != ic_->end()) {
                                        if (isEB) {
                                                histos.h<TProfile>("EBprof", "IC")->Fill(ix, *itIC);
                                        } else {
                                                //assert(eename[5] == '\0');
                                                sprintf(str, "%s:IC", eename);
                                                histos.h<TProfile>("EEprof", str)->Fill(r, *itIC);
                                        }
                                }
                                if (itICMC != icMC_->end()) {
                                        if (isEB) {
                                                histos.h<TProfile>("EBprof", "ICMC")->Fill(ix, *itICMC);
                                        } else {
                                                //assert(eename[5] == '\0');
                                                sprintf(str, "%s:ICMC", eename);
                                                histos.h<TProfile>("EEprof", str)->Fill(r, *itIC);
                                        }
                                }
                        }
                        if (plotChStatus_) {
                                if (itChStatus != chStatus_->end()) {
                                        if (isEB) {
                                                histos.h<TH2D>("EBh2", "chStatus")->Fill(iy, ix, (*itChStatus).getStatusCode());
                                        } else {
                                                //assert(eename[5] == '\0');
                                                sprintf(str, "%s:chStatus", eename);
                                                histos.h<TH2D>("EEh2", str)->Fill(ix, iy, (*itChStatus).getStatusCode());
                                        }
                                }
                        }
                        if (plotPedestals_) {
                                if (itPed != ped_->end()) {
                                        if (isEB) {
                                                histos.h<TH2D>("EBh2", "ped_g12" )->Fill(iy, ix, (*itPed).mean(0));
                                                histos.h<TH2D>("EBh2", "ped_g6"  )->Fill(iy, ix, (*itPed).mean(1));
                                                histos.h<TH2D>("EBh2", "ped_g1"  )->Fill(iy, ix, (*itPed).mean(2));
                                                histos.h<TH2D>("EBh2", "ped_rms_g12" )->Fill(iy, ix, (*itPed).rms(0));
                                                histos.h<TH2D>("EBh2", "ped_rms_g6"  )->Fill(iy, ix, (*itPed).rms(1));
                                                histos.h<TH2D>("EBh2", "ped_rms_g1"  )->Fill(iy, ix, (*itPed).rms(2));
                                        } else {
                                                sprintf(str, "%s:ped_g12", eename);
                                                histos.h<TH2D>("EEh2", str)->Fill( ix, iy, (*itPed).mean(0) );
                                                sprintf(str, "%s:ped_g6", eename);
                                                histos.h<TH2D>("EEh2", str)->Fill( ix, iy, (*itPed).mean(1) );
                                                sprintf(str, "%s:ped_g1", eename);
                                                histos.h<TH2D>("EEh2", str)->Fill( ix, iy, (*itPed).mean(2) );
                                                sprintf(str, "%s:ped_rms_g12", eename);
                                                histos.h<TH2D>("EEh2", str)->Fill( ix, iy, (*itPed).rms(0) );
                                                sprintf(str, "%s:ped_rms_g6", eename);
                                                histos.h<TH2D>("EEh2", str)->Fill( ix, iy, (*itPed).rms(1) );
                                                sprintf(str, "%s:ped_rms_g1", eename);
                                                histos.h<TH2D>("EEh2", str)->Fill( ix, iy, (*itPed).rms(2) );
                                        }
                                }
                        }
                        if (plotGainRatios_) {
                                if (itGR != gr_->end()) {
                                        if (isEB) {
                                                histos.h<TH2D>("EBh2", "gr_g12/6")->Fill(iy, ix, (*itGR).gain12Over6());
                                                histos.h<TH2D>("EBh2", "gr_g6/1" )->Fill(iy, ix, (*itGR).gain6Over1());
                                        } else {
                                                sprintf(str, "%s:gr12/6", eename);
                                                histos.h<TH2D>("EEh2", str)->Fill( ix, iy, (*itGR).gain12Over6() );
                                                sprintf(str, "%s:gr6/1", eename);
                                                histos.h<TH2D>("EEh2", str)->Fill( ix, iy, (*itGR).gain6Over1() );
                                        }
                                }
                        }
                }
        }
        if ( dumpADCToGeV_ ) {
                adcToGeV_ = es.getHandle(adcToGeVToken_);
                ofile_ << "ADCToGeV  EB= " << adcToGeV_->getEBValue() << "  EE= " << adcToGeV_->getEEValue() << "\n";
        }
        ++niov_;
}


// ------------ method called once each job just before starting event loop  ------------
	void 
DBDump::beginJob()
{
}

// ------------ method called once each job just after ending the event loop  ------------
void 
DBDump::endJob() {
	lp_.printText();	
}

//define this as a plug-in
DEFINE_FWK_MODULE(DBDump);
