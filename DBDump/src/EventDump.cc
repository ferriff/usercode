// -*- mode: c++; c-basic-offset:8; -*-
//
// Package:    EventDump
// Class:      EventDump
// 
/**\class EventDump EventDump.cc MyPackage/EventDump/src/EventDump.cc

Description: <one line class summary>

Implementation:
<Notes on implementation>
 */
//
// Original Author:  Federico FERRI
//         Created:  Fri May 11 11:06:00 CEST 2012
// $Id: EventDump.cc,v 1.11 2013/07/10 10:04:32 ferriff Exp $
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
#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatiosRef.h"
#include "CondFormats/EcalObjects/interface/EcalLaserAlphas.h"
#include "CondFormats/EcalObjects/interface/EcalPedestals.h"
#include "CondFormats/EcalObjects/interface/EcalTimeCalibConstants.h"

#include "CondFormats/DataRecord/interface/EcalADCToGeVConstantRcd.h"
#include "CondFormats/DataRecord/interface/EcalChannelStatusRcd.h"
#include "CondFormats/DataRecord/interface/EcalGainRatiosRcd.h"
#include "CondFormats/DataRecord/interface/EcalIntercalibConstantsMCRcd.h"
#include "CondFormats/DataRecord/interface/EcalIntercalibConstantsRcd.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRcd.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRefRcd.h"
#include "CondFormats/DataRecord/interface/EcalLaserAlphasRcd.h"
#include "CondFormats/DataRecord/interface/EcalPedestalsRcd.h"
#include "CondFormats/DataRecord/interface/EcalTimeCalibConstantsRcd.h"

#include "CalibCalorimetry/EcalLaserCorrection/interface/EcalLaserDbService.h"
#include "CalibCalorimetry/EcalLaserCorrection/interface/EcalLaserDbRecord.h"

#include "DataFormats/EcalRecHit/interface/EcalRecHitCollections.h"
#include "DataFormats/EcalDigi/interface/EcalDigiCollections.h"
#include "DataFormats/EgammaReco/interface/SuperCluster.h"
#include "DataFormats/EgammaReco/interface/SuperClusterFwd.h"

#include "Geometry/Records/interface/CaloTopologyRecord.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"

#include "CalibCalorimetry/EcalLaserAnalyzer/interface/MEEBGeom.h"
#include "CalibCalorimetry/EcalLaserAnalyzer/interface/MEEEGeom.h"

#include <Geometry/EcalMapping/interface/EcalElectronicsMapping.h>
#include <Geometry/EcalMapping/interface/EcalMappingRcd.h>

#include "Geometry/CaloTopology/interface/CaloTopology.h"
#include "Geometry/CaloGeometry/interface/CaloCellGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloSubdetectorGeometry.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "usercode/DBDump/interface/HistoManager.h"

#include "TProfile.h"
#include "TProfile2D.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TGraph.h"
#include "TGraphAsymmErrors.h"

class EventDump : public edm::EDAnalyzer {
   public:
      explicit EventDump(const edm::ParameterSet&);
      ~EventDump();
      void setDumpFalse();

   private:
      virtual void beginJob() ;
      virtual void analyze(const edm::Event&, const edm::EventSetup&);
      //virtual void beginRun(const edm::Event&, const edm::EventSetup&);
      //virtual void endRun(const edm::Event&, const edm::EventSetup&);
      virtual void endJob() ;

      void dumpClusters(const edm::Event& ev, const edm::EventSetup& es);
      void dumpClusters(const edm::Event& ev, const edm::EventSetup& es, const reco::SuperClusterCollection * scs, const EcalRecHitCollection * rechits);

      void dumpRecHits(const edm::Event& ev, const edm::EventSetup& es);
      void dumpRecHits(const edm::Event& ev, const edm::EventSetup& es, const EcalRecHitCollection * rechits);

      void dumpUncalibratedRecHits(const edm::Event& ev, const edm::EventSetup& es);
      void dumpUncalibratedRecHits(const edm::Event& ev, const edm::EventSetup& es, const EcalUncalibratedRecHitCollection * urechits);

      void dumpDigis(const edm::Event& ev, const edm::EventSetup& es);
      void dumpDigis(const edm::Event& ev, const edm::EventSetup& es, const EcalDigiCollection * digis);

      void dumpEvent(const edm::Event & ev, const edm::EventSetup & es);

      typedef struct Coord {
              int ix_;
              int iy_;
              int iz_;
      } Coord;
      void coord(DetId id, Coord * c);

      void printSummary();

      // output files
      bool outDump_;
      std::string outDumpFile_;
      std::ofstream ofile_;

      FILE * fd_dump_;

      // geometry
      bool dumpGeometry_;

      // inter-calibration constants
      bool dumpIC_;
      edm::ESHandle<EcalIntercalibConstants> ic_;
      edm::ESHandle<EcalIntercalibConstantsMC> icMC_;

      // time-calibration constants
      bool dumpTC_;
      edm::ESHandle<EcalTimeCalibConstants> tc_;

      // ADCToGeV constant
      bool dumpADCToGeV_;
      edm::ESHandle<EcalADCToGeVConstant> adctogev_;

      // laser transparency measurements
      bool dumpTransp_;
      edm::ESHandle<EcalLaserAPDPNRatios> apdpn_;
      edm::ESHandle<EcalLaserAPDPNRatiosRef> apdpnref_;

      // laser alpha
      bool dumpAlpha_;
      edm::ESHandle<EcalLaserAlphas> alpha_;

      // laser transparency corrections
      bool dumpTranspCorr_;
      edm::ESHandle<EcalLaserDbService> laser_;

      // channel status map
      bool dumpChStatus_;
      edm::ESHandle<EcalChannelStatus> chStatus_;

      // pedestals
      bool dumpPedestals_;
      edm::ESHandle<EcalPedestals> ped_;

      // gain ratios
      bool dumpGainRatios_;
      edm::ESHandle<EcalGainRatios> gr_;

      // event info
      bool dumpEvent_;

      // clusters
      bool dumpClusters_;

      // recHits
      bool dumpRecHits_;

      // uncalibratedRecHits
      bool dumpUncalibratedRecHits_;

      // digis
      bool dumpDigis_;

      // Input tags
      // SuperClusters
      edm::InputTag ieb_scs_;
      edm::InputTag iee_scs_;
      edm::Handle<reco::SuperClusterCollection> heb_scs_;
      edm::Handle<reco::SuperClusterCollection> hee_scs_;
      // EcalRecHits
      edm::InputTag ieb_rechits_;
      edm::InputTag iee_rechits_;
      edm::Handle<EcalRecHitCollection> heb_rechits_;
      edm::Handle<EcalRecHitCollection> hee_rechits_;
      // EcalUncalibratedRecHits
      edm::InputTag ieb_urechits_;
      edm::InputTag iee_urechits_;
      edm::Handle<EcalUncalibratedRecHitCollection> heb_urechits_;
      edm::Handle<EcalUncalibratedRecHitCollection> hee_urechits_;
      // EcalDigis
      edm::InputTag ieb_digis_;
      edm::InputTag iee_digis_;
      edm::Handle<EBDigiCollection> heb_digis_;
      edm::Handle<EEDigiCollection> hee_digis_;

      // conditions: iterators
      EcalIntercalibConstantMap::const_iterator it_ic_;
      EcalChannelStatusMap::const_iterator it_chstatus_;
      EcalPedestalsMap::const_iterator it_ped_;
      EcalGainRatioMap::const_iterator it_gr_;
      EcalLaserAPDPNRatios::EcalLaserAPDPNRatiosMap::const_iterator it_apdpn_;
      EcalLaserAPDPNRatiosRef::const_iterator it_apdpnref_;
      EcalLaserAlphas::const_iterator it_alpha_;
      EcalLaserAPDPNRatios::EcalLaserTimeStamp ts_;

      HistoManager histos;

      //edm::ESHandle<CaloGeometry> caloGeometry_;
      const CaloGeometry * geometry;

      std::vector<DetId> ecalDetIds_;

      // number of IOVs, first and last
      int niov_;
      time_t iov_first_;
      time_t iov_last_;

      // job start and stop time
      time_t jstart_;
      time_t jstop_;

      time_t il_;

      int run_, event_, time_, ls_;

      bool first_;

      FILE * fgeom;
};



EventDump::EventDump(const edm::ParameterSet& ps) :
        niov_(0),
        iov_first_(-1),
        iov_last_(0),
        jstart_(time(NULL)),
        jstop_(0),
        il_(0),
        first_(false) // do not pass if not needed
{
	outDump_ = ps.getParameter<bool>("outDump");
	outDumpFile_ = ps.getParameter<std::string>("outDumpFile");
        // geometry
        dumpGeometry_ = ps.getParameter<bool>("dumpGeometry");
	// inter-calibration constants
	dumpIC_ = ps.getParameter<bool>("dumpIC");
	// time-calibration constants
	dumpTC_ = ps.getParameter<bool>("dumpTC");
	// ADCToGeV constant
	dumpADCToGeV_ = ps.getParameter<bool>("dumpADCToGeV");
	// laser transparency measurements
	dumpTransp_ = ps.getParameter<bool>("dumpTransp");
	// laser alpha
	dumpAlpha_ = ps.getParameter<bool>("dumpAlpha");
	// channel status map
	dumpChStatus_ = ps.getParameter<bool>("dumpChStatus");
	// pedestals
	dumpPedestals_ = ps.getParameter<bool>("dumpPedestals");
	// gain ratios
	dumpGainRatios_ = ps.getParameter<bool>("dumpGainRatios");
	// laser transparency corrections
	dumpTranspCorr_ = ps.getParameter<bool>("dumpTranspCorr");

        // event info
        dumpEvent_ = ps.getParameter<bool>("dumpEvent");

	// digis
	dumpDigis_ = ps.getParameter<bool>("dumpDigis");

	// uncalibratedRecHits
	dumpUncalibratedRecHits_ = ps.getParameter<bool>("dumpUncalibratedRecHits");

	// recHits
	dumpRecHits_ = ps.getParameter<bool>("dumpRecHits");

	// clusters
	dumpClusters_ = ps.getParameter<bool>("dumpClusters");

        ieb_scs_ = ps.getParameter<edm::InputTag>("ecalSuperClustersEB");
        iee_scs_ = ps.getParameter<edm::InputTag>("ecalSuperClustersEE");

        ieb_rechits_ = ps.getParameter<edm::InputTag>("ecalRecHitsEB");
        iee_rechits_ = ps.getParameter<edm::InputTag>("ecalRecHitsEE");

        ieb_urechits_ = ps.getParameter<edm::InputTag>("ecalUncalibratedRecHitsEB");
        iee_urechits_ = ps.getParameter<edm::InputTag>("ecalUncalibratedRecHitsEE");

        ieb_digis_ = ps.getParameter<edm::InputTag>("ecalDigisEB");
        iee_digis_ = ps.getParameter<edm::InputTag>("ecalDigisEE");

	if (outDumpFile_ != "") {
		ofile_.open(outDumpFile_.c_str(), std::ios::out);
		if (!ofile_.is_open()) {
			edm::LogError("EventDump") << "EventDump::EventDump cannot open output file " << outDumpFile_;
			setDumpFalse();
		}
	} else if (outDump_) {
		edm::LogError("EventDump") << "EventDump::EventDump mismatch in your config file, setting outDump to false.";
		setDumpFalse();
	} else {
		setDumpFalse();
	}

        fd_dump_ = fopen(outDumpFile_.c_str(), "w");

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

void EventDump::setDumpFalse()
{
        dumpRecHits_ = false;
        dumpUncalibratedRecHits_ = false;
        dumpDigis_ = false;
        dumpEvent_ = false;

        dumpGeometry_ = false;
	dumpIC_ = false;
	dumpTC_ = false;
	dumpADCToGeV_ = false;
	dumpTransp_ = false;
        dumpAlpha_ = false;
	dumpTranspCorr_ = false;
	dumpChStatus_ = false;
	dumpPedestals_ = false;
}

EventDump::~EventDump()
{
        if (ofile_.is_open()) {
                ofile_.close();
        }
        jstop_ = time(NULL);
        printSummary();
}

void EventDump::printSummary()
{
	//char buf[128];
	//strftime(buf, sizeof(buf), "%F %R:%S", gmtime(&iov_first_));
	//printf("First IOV: %ld (%s UTC)\n", iov_first_, buf);
	//strftime(buf, sizeof(buf), "%F %R:%S", gmtime(&iov_last_));
	//printf(" Last IOV: %ld (%s UTC)\n", iov_last_, buf);
}


void EventDump::coord(DetId id, Coord * c)
{
        if (id.subdetId() == EcalBarrel) {
                EBDetId eid(id);
                c->ix_ = eid.ieta();
                c->iy_ = eid.iphi();
                c->iz_ = 0;
        } else if (id.subdetId() == EcalEndcap) {
                EEDetId eid(id);
                c->ix_ = eid.ix();
                c->iy_ = eid.iy();
                c->iz_ = eid.zside();
        } else {
                fprintf(stderr, "[EventDump::coord] ERROR: invalid DetId %d", id.rawId());
                assert(0);
        }
}


void EventDump::dumpEvent(const edm::Event& ev, const edm::EventSetup& es)
{
        // dump header
        fprintf(fd_dump_, "#evt run event ls time\n");
        fprintf(fd_dump_, "evt %d %d %d %d\n", run_, event_, ls_, time_);
}


void EventDump::dumpDigis(const edm::Event& ev, const edm::EventSetup& es, const EcalDigiCollection * digis)
{
        // dump header
        fprintf(fd_dump_, "#edg run event ls time\trawid ieta/ix iphi/iy 0/zside\tped_g12 ped_g6 ped_g1\tg12_o_g6 g6_o_g1\tsample_0 s1 s2 s4 s5 s6 s7 s8 sample_9\tgain_0 g1 g2 g3 g4 g5 g6 g7 g8 gain_9");
        fprintf(fd_dump_, "\n");
        Coord c;
        for(EcalDigiCollection::const_iterator itdg = digis->begin(); itdg != digis->end(); ++itdg) {

                DetId id(itdg->id());
                coord(id, &c);
                fprintf(fd_dump_, "edg %d %d %d %d\t%d %d %d %d", 
                        run_, event_, ls_, time_, id.rawId(), c.ix_, c.iy_, c.iz_
                        );
                fprintf(fd_dump_, "\t");
                it_ped_ = ped_->find(id);
                fprintf(fd_dump_, "%f %f %f", (*it_ped_).mean(1), (*it_ped_).mean(2), (*it_ped_).mean(3));
                fprintf(fd_dump_, "\t");
                it_gr_ = gr_->find(id);
                fprintf(fd_dump_, "%f %f", (*it_gr_).gain12Over6(), (*it_gr_).gain6Over1());
                fprintf(fd_dump_, "\t");
                for (int is = 0; is < EcalDataFrame::MAXSAMPLES; ++is) {
                        int adc = ((EcalDataFrame)*itdg).sample(is).adc();
                        fprintf(fd_dump_, " %d", adc);
                }
                fprintf(fd_dump_, "\t");
                for (int is = 0; is < EcalDataFrame::MAXSAMPLES; ++is) {
                        int gid = ((EcalDataFrame)*itdg).sample(is).gainId();
                        fprintf(fd_dump_, " %d", gid);
                }
                fprintf(fd_dump_, "\n");
        }
}


void EventDump::dumpUncalibratedRecHits(const edm::Event& ev, const edm::EventSetup& es, const EcalUncalibratedRecHitCollection * urechits)
{
        // dump header
        fprintf(fd_dump_, "#erh run event ls time\trawid ieta/ix iphi/iy 0/zside\tenergy time flag");
        fprintf(fd_dump_, "\tadc2gev ic lc");
        fprintf(fd_dump_, "\n");
        Coord c;
        for(size_t iurh =0; iurh < urechits->size(); ++iurh) {
                EcalUncalibratedRecHitRef urhref(urechits, iurh);
                DetId id(urhref->id());
                coord(id, &c);
                //fprintf(fd_dump_, "erh %d %d %d %d\t%d %d %d %d\t%f %f %d", 
                //        run_, event_, ls_, time_,
                //        id.rawId(), c.ix_, c.iy_, c.iz_,
                //        urhref->energy(), rhref->time(), rhref->recoFlag()
                //        );
                //it_ic_ = ic_->find(id);
                //float lc = laser_->getLaserCorrection(id, ev.time());
                //if (it_ic_ == ic_->end()) {
                //        fprintf(stderr, "problem with IC for detId %d", id.rawId());
                //        assert(0);
                //}
                //fprintf(fd_dump_, "\t%f %f", *it_ic_, lc);
                ////fprintf(fd_dump_, "\t%f  %d %f  %d %f  %d %f", );
                //fprintf(fd_dump_, "\n");
        }
}


void EventDump::dumpClusters(const edm::Event& ev, const edm::EventSetup& es, const reco::SuperClusterCollection * scs, const EcalRecHitCollection * rechits)
{
        fprintf(fd_dump_, "#esc run event ls time\tenergy n_hits energy eta phi raw_energy\n");
        for (reco::SuperClusterCollection::const_iterator cit = scs->begin(); cit != scs->end(); ++cit) {
                fprintf(fd_dump_, "esc %d %d %d %d", run_, event_, ls_, time_);
                const std::vector<std::pair<DetId, float> > & hf = cit->hitsAndFractions();
                float e_raw = 0;
                for (size_t i = 0; i < hf.size(); ++i) {
                        e_raw += rechits->find(hf[i].first)->energy() * hf[i].second;
                }
                fprintf(fd_dump_, " %lu %f %f %f %f %f %f\n", hf.size(), cit->energy(), cit->eta(), cit->phi(), e_raw, cit->rawEnergy(), cit->preshowerEnergy());
        }
}


void EventDump::dumpRecHits(const edm::Event& ev, const edm::EventSetup& es, const EcalRecHitCollection * rechits)
{
        // dump header
        fprintf(fd_dump_, "#erh run event ls time\trawid ieta/ix iphi/iy 0/zside\tenergy time flag");
        fprintf(fd_dump_, "\tic lc");
        fprintf(fd_dump_, "\n");
        Coord c;
        for(size_t irh =0; irh < rechits->size(); ++irh) {
                EcalRecHitRef rhref(rechits, irh);
                DetId id(rhref->id());
                coord(id, &c);
                fprintf(fd_dump_, "erh %d %d %d %d\t%d %d %d %d\t%f %f %d", 
                        run_, event_, ls_, time_,
                        id.rawId(), c.ix_, c.iy_, c.iz_,
                        rhref->energy(), rhref->time(), rhref->recoFlag()
                        );
                it_ic_ = ic_->find(id);
                float lc = laser_->getLaserCorrection(id, ev.time());
                if (it_ic_ == ic_->end()) {
                        fprintf(stderr, "problem with IC for detId %d", id.rawId());
                        assert(0);
                }
                float adctogev = 0;
                if (id.subdetId() == EcalBarrel) {
                        adctogev = adctogev_->getEBValue();
                } else {
                        adctogev = adctogev_->getEEValue();
                }
                fprintf(fd_dump_, "\t%f %f %f", adctogev, *it_ic_, lc);
                //fprintf(fd_dump_, "\t%f  %d %f  %d %f  %d %f", );
                fprintf(fd_dump_, "\n");
        }
}


void EventDump::dumpDigis(const edm::Event& ev, const edm::EventSetup& es)
{
        ev.getByLabel(ieb_digis_, heb_digis_);
        dumpDigis(ev, es, heb_digis_.product());

        ev.getByLabel(iee_digis_, hee_digis_);
        dumpDigis(ev, es, hee_digis_.product());
}


void EventDump::dumpUncalibratedRecHits(const edm::Event& ev, const edm::EventSetup& es)
{
        ev.getByLabel(ieb_urechits_, heb_urechits_);
        dumpUncalibratedRecHits(ev, es, heb_urechits_.product());

        ev.getByLabel(iee_urechits_, hee_urechits_);
        dumpUncalibratedRecHits(ev, es, hee_urechits_.product());
        // Preshower
        //ev.getByLabel( ecalRecHitESCollection_, esRecHits_h_ );
}


void EventDump::dumpRecHits(const edm::Event& ev, const edm::EventSetup& es)
{
        ev.getByLabel(ieb_rechits_, heb_rechits_);
        dumpRecHits(ev, es, heb_rechits_.product());

        ev.getByLabel(iee_rechits_, hee_rechits_);
        dumpRecHits(ev, es, hee_rechits_.product());
        // Preshower
        //ev.getByLabel( ecalRecHitESCollection_, esRecHits_h_ );
}


void EventDump::dumpClusters(const edm::Event& ev, const edm::EventSetup& es)
{
        ev.getByLabel(ieb_scs_, heb_scs_);
        dumpClusters(ev, es, heb_scs_.product(), heb_rechits_.product());

        ev.getByLabel(iee_scs_, hee_scs_);
        dumpClusters(ev, es, hee_scs_.product(), hee_rechits_.product());
        // Preshower
        //ev.getByLabel( ecalRecHitESCollection_, esRecHits_h_ );
}


// ------------ method called to for each event  ------------
void
EventDump::analyze(const edm::Event& ev, const edm::EventSetup& es)
{
        run_   = ev.id().run();
        event_ = ev.id().event();
        time_  = ev.time().unixTime();
        ls_    = ev.id().luminosityBlock();

        //es.get<EcalTimeCalibConstantsRcd>().get( tc_ );
        //es.get<CaloGeometryRecord>().get(caloGeometry_);
        //geo_ = caloGeometry_.product();
        edm::ESHandle< EcalElectronicsMapping > ecalmapping;
        es.get<EcalMappingRcd>().get(ecalmapping);

        edm::ESHandle<CaloGeometry> pGeometry;
        es.get<CaloGeometryRecord>().get(pGeometry);
        geometry = pGeometry.product();

        // get conditions
        es.get<EcalIntercalibConstantsRcd>().get(ic_);
        es.get<EcalIntercalibConstantsRcd>().get(icMC_);
        es.get<EcalChannelStatusRcd>().get(chStatus_);
        es.get<EcalPedestalsRcd>().get(ped_);
        es.get<EcalGainRatiosRcd>().get(gr_);
        es.get<EcalLaserAPDPNRatiosRcd>().get(apdpn_);
        es.get<EcalLaserAPDPNRatiosRefRcd>().get(apdpnref_);
        es.get<EcalLaserAlphasRcd>().get(alpha_);
        es.get<EcalLaserDbRecord>().get(laser_);
        es.get<EcalPedestalsRcd>().get(ped_);
        es.get<EcalGainRatiosRcd>().get(gr_);
        es.get<EcalADCToGeVConstantRcd>().get(adctogev_);

        if (dumpEvent_)   dumpEvent(ev, es);
        if (dumpDigis_)   dumpDigis(ev, es);
        if (dumpUncalibratedRecHits_) dumpUncalibratedRecHits(ev, es);
        if (dumpRecHits_) dumpRecHits(ev, es);
        if (dumpClusters_ && dumpRecHits_) dumpClusters(ev, es);
        else if (dumpClusters_) fprintf(stderr, "Cannot dump clusters without dumping recHits, please change the cfg file.\n");

        ++niov_;
}


// ------------ method called once each job just before starting event loop  ------------
	void 
EventDump::beginJob()
{
}

// ------------ method called once each job just after ending the event loop  ------------
void 
EventDump::endJob() {
}

//define this as a plug-in
DEFINE_FWK_MODULE(EventDump);
