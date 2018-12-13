#include "usercode/DBDump/interface/EcalLaserPlotter.h"

#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatios.h"
#include "CondFormats/EcalObjects/interface/EcalChannelStatus.h"
#include "CalibCalorimetry/EcalLaserCorrection/interface/EcalLaserDbService.h"
#include "CalibCalorimetry/EcalLaserAnalyzer/interface/MEEBGeom.h"
#include "CalibCalorimetry/EcalLaserAnalyzer/interface/MEEEGeom.h"

#include "usercode/DBDump/interface/HistoManager.h"
#include "usercode/DBDump/interface/Quantile.h"

#include "TProfile.h"
#include "TProfile2D.h"
#include "TGraphAsymmErrors.h"

#include <iostream>

EcalLaserPlotter::EcalLaserPlotter(const char * geom_filename) :
        il_(0), niov_(0), iov_first_(-1), iov_last_(0), set_geom_(0), set_ch_status_(0)
{
        sprintf(ecalPart, "E*");
        sprintf(eename, "*Z");
        setEcalGeometry(geom_filename);
        hm_.addTemplate<TH2D>( "EBh2", new TH2D( "EBh2", "EBh2", 360, 0.5, 360.5, 171, -85.5, 85.5 ) );
        hm_.addTemplate<TH2D>( "EEh2", new TH2D( "EEh2", "EEh2", 100, 0., 100., 100, 0., 100. ) );
        hm_.addTemplate<TProfile2D>( "EBprof2", new TProfile2D( "EBp2", "EBp2", 360, 0.5, 360.5, 171, -85.5, 85.5 ) );
        hm_.addTemplate<TProfile2D>( "EEprof2", new TProfile2D( "EEp2", "EEp2", 100, 0., 100., 100, 0., 100. ) );
        hm_.addTemplate<TProfile>( "EEprof", new TProfile( "EEp", "EEp", 55, 0., 55.) );
        hm_.addTemplate<TProfile>( "EBprof", new TProfile( "EBp", "EBp", 171, -85.5, 85.5) );
        hm_.addTemplate<TGraphAsymmErrors>("history", new TGraphAsymmErrors());
        hm_.addTemplate<TH1D>("distr", new TH1D("distribution", "distribution", 2000, 0., 2.));
	hm_.addTemplate<TH2D>( "Slope2", new TH2D( "Slope","Slope",800,.4,1.2,1000,-.05,.05));
	//keep distribution as a function of IOV
	hm_.addTemplate<TH2D>("IOVdistr", new TH2D("IOVdistribution","IOVdistribution", 2000,0.,2., 4000,0,4000));
        hm_.addTemplate<TProfile>("etaProf", new TProfile("etaProf", "etaProf", 250, -2.75, 2.75));
	// hm_.addTemplate<TProfile>("etaProfMedian", new TProfile("etaProfMedian", "etaProfMedian", 250, -2.75, 2.75));
	hm_.addTemplate<TH2D>("channel_LM", new TH2D("channel_LM","channel_LM",92 ,0 ,92 ,4000,0,4000));
	hm_.addTemplate<TH2D>("channel_eta", new TH2D("channel_eta","channel_eta", 250., -2.75, 2.75, 4000., 0., 4000.));
	hm_.addTemplate<TH2D>("slope_eta", new TH2D("slope_eta","slope_eta", 250,-2.75,2.75, 2000, 0,.1));
	for(int i=0;i<nq_;i++) //initialize counters for TGRaph of slopes.
          {
            nSlope1_[i]=0;
            nSlope3_[i]=0;
          }

        // initialise ECAL DetId, max_val, min_val vectors
        ecalDetIds_.resize(EBDetId::MAX_HASH + 1 + EEDetId::kSizeForDenseIndexing);
        max_val_.resize(EBDetId::MAX_HASH + 1 + EEDetId::kSizeForDenseIndexing);
        min_val_.resize(EBDetId::MAX_HASH + 1 + EEDetId::kSizeForDenseIndexing);
        int idx = -1;
        for (int hi = EBDetId::MIN_HASH; hi <= EBDetId::MAX_HASH; ++hi ) {
                EBDetId ebId = EBDetId::unhashIndex(hi);
                if (ebId != EBDetId()) {
                        ecalDetIds_[hi] = ebId;
                        max_val_[hi] = -FLT_MAX;
                        min_val_[hi] = FLT_MAX;
                }
        }
        for ( int hi = 0; hi < EEDetId::kSizeForDenseIndexing; ++hi ) {
                EEDetId eeId = EEDetId::unhashIndex(hi);
                if (eeId != EEDetId()) {
                        idx = EBDetId::MAX_HASH + 1 + hi;
                        ecalDetIds_[idx] = eeId;
                        max_val_[idx] = -FLT_MAX;
                        min_val_[idx] = FLT_MAX;
                }
        }
        assert(ecalDetIds_.size() == 75848);
        assert(ecalDetIds_.size() == EBDetId::MAX_HASH + 1 + EEDetId::kSizeForDenseIndexing);

        // initialise quantile names
	int i = 0;
        sprintf(qname_[i++], "All");
        sprintf(qname_[i++], "EE-");
        sprintf(qname_[i++], "EB-");
        sprintf(qname_[i++], "EB+");
        sprintf(qname_[i++], "EE+");
        for (int j = 1; j <= 92; ++j) {
                sprintf(qname_[i++], "LM%02d", j);
        }
	for(int j = 1; j <= netabins_; ++j){
		sprintf(qname_[i++], "eta%02d", j);
	}
	assert(i == sizeof(qname_)/sizeof(qname_[0]));
}

void EcalLaserPlotter::reset_quantile()
{
        for (int i = 0; i < nq_; i++) 
	  {
	    q_[i].reset();
	    qslope1_[i].reset();
	    qslope3_[i].reset();
	  }
}

void EcalLaserPlotter::save(const char * filename, const char * opt)
{
        static TH2D * bad_channel_map_eta;
	static TH2D * total_channel_map_eta;
	hm_.h<TH2D>("channel_eta","bad_channel_map",&bad_channel_map_eta)->Divide(hm_.h<TH2D>("channel_eta","temp", &total_channel_map_eta));

        max_min_plots();
        hm_.save(filename, opt);
}

int EcalLaserPlotter::etabin(float eta)
{
        const float etamin  =  -2.964;
        const float etamax  =  2.964;
        //		if(!(etamin < eta && eta < etamax)){ printf("---> %f\n", eta); }
        assert(etamin < eta && eta < etamax);
        return int((eta - etamin) / (etamax - etamin) * netabins_);
}

float EcalLaserPlotter::geom_eta(DetId id)
{
        if (id.subdetId() == EcalBarrel) {
                return geom_eta_[EBDetId(id).hashedIndex()];
        } else if (id.subdetId() == EcalEndcap) {
                return geom_eta_[EBDetId::MAX_HASH + 1 + EEDetId(id).denseIndex()];
        } else {
                assert(0);
        }
}

void EcalLaserPlotter::max_min(DetId id, float val)
{
        int idx = -1;
        if (id.subdetId() == EcalBarrel) {
                idx = EBDetId(id).hashedIndex();
        } else if (id.subdetId() == EcalEndcap) {
                idx = EBDetId::MAX_HASH + 1 + EEDetId(id).denseIndex();
        } else {
                assert(0);
        }
        if (max_val_[idx] < val)  max_val_[idx] = val;
        if (min_val_[idx] > val)  min_val_[idx] = val;
}

void EcalLaserPlotter::max_min_plots()
{
        int ix = -1, iy = -1, iz = -1;
        int isEB = 0;
        const char * subdet[NSUBDET] =  { "nZ_", "" , "pZ_" };
	static TH2D * h2d_max[3]; // FIXME - hope static arrays are zeroed by default
        static TH2D * h2d_min[3]; // FIXME - hope static arrays are zeroed by default
        
	const char * temhl[] = { "EEh2", "EBh2"};
	for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
	        if (set_ch_status_ && ch_status_[i] != 0) continue;
		DetId id(ecalDetIds_[i]);
		if (id.subdetId() == EcalBarrel) {
                        ix = EBDetId(id).iphi();
                        iy = EBDetId(id).ieta();
                        iz = 0;
                        isEB = 1;
                } else if (id.subdetId() == EcalEndcap) {
                        ix = EEDetId(id).ix();
                        iy = EEDetId(id).iy();
                        iz = EEDetId(id).zside();
                        isEB = 0;
                }
                sprintf(str, "%smax", subdet[iz+1]);
                hm_.h<TH2D>(temhl[isEB], str, &h2d_max[iz + 1])->Fill(ix, iy, max_val_[i]);
                sprintf(str, "%smin", subdet[iz+1]);
                hm_.h<TH2D>(temhl[isEB], str, &h2d_min[iz + 1])->Fill(ix, iy, min_val_[i]);
        }
}

void EcalLaserPlotter::setEcalGeometry(const char * geom_filename)
{
        if (set_geom_) return;
        FILE * fg = fopen(geom_filename, "r");
        if (fg == NULL) {
                fprintf(stderr, "Cannot open file `%s'.\n", geom_filename);
                fprintf(stderr, "Please specify the correct location of the geometry\n");
                fprintf(stderr, "ascii file (format: DetId eta phi R).\n");
        } else {
                int n, id;
                float eta;
                size_t offset = EBDetId::MAX_HASH - EBDetId::MIN_HASH + 1;
                geom_eta_.resize(offset + EEDetId::kSizeForDenseIndexing);
                while ((n = fscanf(fg, "%d %f %*f %*f", &id, &eta)) == 2) {
                        if (DetId(id).subdetId() == EcalBarrel) {
                                geom_eta_[EBDetId(id).hashedIndex()] = eta;
                        } else {
                                geom_eta_[offset + EEDetId(id).denseIndex()] = eta;
                        }
                }
        }
        fclose(fg);
        set_geom_ = 1;
}

void EcalLaserPlotter::setEcalChannelStatus(const EcalChannelStatus & chStatus, int onceForAll)
{
        if (onceForAll && set_ch_status_) return;
        if (!set_ch_status_) ch_status_.resize(ecalDetIds_.size());
        FILE * ftmp;
        if (!set_ch_status_) 
	  {
	    ftmp = fopen("channelStatus.dump", "w");
	    for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
	      ch_status_[i] = chStatus.find(ecalDetIds_[i])->getStatusCode();
	      if (!set_ch_status_) {
		DetId id(ecalDetIds_[i]);
		int ix = -1, iy = -1, iz = -1;
		if (id.subdetId() == EcalBarrel) {
		  ix = EBDetId(id).iphi();
		  iy = EBDetId(id).ieta();
		  iz = 0;
		} else if (id.subdetId() == EcalEndcap) {
		  ix = EEDetId(id).ix();
		  iy = EEDetId(id).iy();
		  iz = EEDetId(id).zside();
		}
		fprintf(ftmp, "%d %d %d  %d\n", ix, iy, iz, ch_status_[i]);
	      }
	    }
        fclose(ftmp);
        set_ch_status_ = 1;
	  }
}

void EcalLaserPlotter::printSummary()
{
  
        char buf[256];
        strftime(buf, sizeof(buf), "%F %R:%S", gmtime(&iov_first_));
        printf("First IOV: %ld (%s UTC)\n", iov_first_, buf);
        strftime(buf, sizeof(buf), "%F %R:%S", gmtime(&iov_last_));
        printf(" Last IOV: %ld (%s UTC)\n", iov_last_, buf);
        printf("\n");
        printf("List of problematic channels:\n");
        printf("%d nan:", (int)nan_.size());
        for (std::set<int>::const_iterator it = nan_.begin(); it != nan_.end(); ++it) {
                printf(" %d", *it);
        }
        printf("\n");
        printf("%d inf:", (int)inf_.size());
        for (std::set<int>::const_iterator it = inf_.begin(); it != inf_.end(); ++it) {
                printf(" %d", *it);
        }
        printf("\n");
}

void EcalLaserPlotter::compute_averages(const EcalLaserAPDPNRatios & apdpn, time_t t)
{
        int countneg =0;
        int count0 =0;
        int count1 =0;
        static TProfile * p = 0;
	//static TProfile * m = 0;
        char name[64];
        //sprintf(name, "p2_%ld", t);
        sprintf(name, "p2");
	hm_.h<TProfile>("etaProf", name, &p)->Reset(); 
        //hm_.h<TProfile>("etaProfMedian", name, &m)->Reset();
	for (size_t iid = 0; iid < ecalDetIds_.size(); ++iid) {
	        if (set_ch_status_ && ch_status_[iid] != 0) continue;
	        DetId id(ecalDetIds_[iid]);
                EcalLaserAPDPNRatios::EcalLaserAPDPNRatiosMap::const_iterator itAPDPN;
                EcalLaserAPDPNRatios::EcalLaserTimeStamp ts;
		EcalLaserAPDPNRatios::EcalLaserTimeStampMap::const_iterator itLtime;
		size_t iLM = 0;
		
		itAPDPN = apdpn.getLaserMap().find(id);
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
                if ( iLM-1 < apdpn.getTimeMap().size() ) {
		           ts = apdpn.getTimeMap()[iLM];
                }

                float p2  = -1234567890.;
                float p3  = -1234567890.;
		float p1  = -1234567890.;
		float eta = 99999;
                p2 = (*itAPDPN).p2;
		p1 = (*itAPDPN).p1;
		p3 = (*itAPDPN).p3;
	
		t1 = ts.t1.value()>>32;
		t2 = ts.t2.value()>>32;
		t3 = ts.t3.value()>>32;
		
		//if(t3==t2||t2==t1)
		// std::cout << "t1 " << t1 << "  t2 " << t2 << "  t3  " << t3 << std::endl; 
		
		
		
		
                if (isinf(p2) > 0) {
		        p2 =  FLT_MAX;
                } else if (isinf(p2) < 0) {
                        p2 = -FLT_MAX;
                }
                if (isnan(p2)) {
                        p2 =  FLT_MAX;
                }

                eta = geom_eta(id);
		
		if(p2==1.0) //produces number of bad channels per IOV
		  {
		    ++count1;
		  }
		if(p2==0)
		  {
		    ++count0;
		  }
		if(p2<0)
		  {
		    ++countneg;
		  }

		
		hm_.h<TProfile>("etaProf", name, &p)->Fill(eta, p2);
		/*
		  hm_.h<TH2D>("map"
		*/
		float slope3, slope1;
		if(!isinf((p2-p1)/(t2-t1))&&!isnan((p2-p1)/(t2-t1)))
                  slope1 = 3600*(p2-p1)/(t2-t1);
                else
                  slope1=-100.;
                if(!isinf((p3-p2)/(t3-t2))&&!isnan((p3-p2)/(t3-t2)))
                  slope3 = 3600*(p3-p2)/(t3-t2);
                else
                  slope3=-100.;

		q_[0].fill(p2, iid);
		
		if(slope1!=-100.||slope1!=0)
                  qslope1_[0].fill(slope1, iid);
                if(slope3!=-100.||slope3!=0)
                  qslope3_[0].fill(slope3, iid);
		
		
		if (id.subdetId() == EcalBarrel) 
		  {
		    if (EBDetId(id).ieta() < 0) 
		      {
			q_[2].fill(p2, iid);
			if(slope1!=-100.||slope1!=0)
			  qslope1_[2].fill(slope1, iid);
			if(slope3!=-100.||slope3!=0)
			  qslope3_[2].fill(slope3, iid);
		      }
		    else                       
		      {
			q_[3].fill(p2, iid);
			if(slope1!=-100.||slope1!=0)
			  qslope1_[3].fill(slope1, iid);
			if(slope3!=-100.||slope3!=0)
			  qslope3_[3].fill(slope3, iid);

		      }
		  } 
		else
		  {
		    if (EEDetId(id).zside() < 0) 
		      {
			q_[1].fill(p2, iid);
			if(slope1!=-100.||slope1!=0)
			  qslope1_[1].fill(slope1, iid);
			if(slope3!=-100.||slope3!=0)
			  qslope3_[1].fill(slope3, iid);
		      }
		    else
		      {
			q_[4].fill(p2, iid);
			if(slope1!=-100.||slope1!=0)
			  qslope1_[4].fill(slope1, iid);
			if(slope3!=-100.||slope3!=0)
			  qslope3_[4].fill(slope3, iid);
		      } 
		  }
		q_[5 + iLM - 1].fill(p2, iid);
		q_[qetaoffs_ + etabin(eta)].fill(p2, iid);
		if(slope1!=-100.||slope1!=0)
                  qslope1_[5 + iLM - 1].fill(slope1, iid);
                if(slope1!=-100.||slope1!=0)
                  qslope1_[qetaoffs_ + etabin(eta)].fill(slope1, iid);
                if(slope3!=-100.||slope3!=0)
                  qslope3_[5 + iLM - 1].fill(slope3, iid);
                if(slope3!=-100.||slope3!=0)
                  qslope3_[qetaoffs_ + etabin(eta)].fill(slope3, iid);
		//hm_.h<TProfile>("etaMedian",name, &m)->Fill(eta, p2);
	}
	//char buf[256];
	//strftime(buf, sizeof(buf), "%F %R:%S", gmtime(&t));
	//printf(" Last IOV: %ld (%s UTC) \n", t, buf);
	//std::cout << " # p2==1 " << count1 << " # p2==0 " << count0 << " # p2<0 " << countneg << std::endl;
}

void EcalLaserPlotter::fill_histories(time_t t)
{
        float fracs[] = { 0.5 * (1 - 0.997), 0.5 * (1 - 0.954), 0.5 * (1 - 0.682), 0 };
	const char * nfrac[] = { "3S", "2S", "1S", "E" };
        for (int i = 0; i < nq_; ++i) {
	        float xm = q_[i].xlow(0.5);//median of p2
                for (size_t j = 0; j < sizeof(fracs)/sizeof(float); ++j) {
		  
		        sprintf(str, "p2_%s_%s", qname_[i], nfrac[j]);
                        TGraphAsymmErrors * g = hm_.h<TGraphAsymmErrors>("history", str);
                        g->SetPoint(niov_, t, xm);
                        g->SetPointEYlow(niov_, xm - q_[i].xlow(fracs[j]));
                        g->SetPointEYhigh(niov_, q_[i].xhigh(fracs[j]) - xm);
                }
        } 
}

void EcalLaserPlotter::fill_slope_histories(time_t t)
{
  float fracs[] = { 0.5 * (1 - 0.997), 0.5 * (1 - 0.954), 0.5 * (1 - 0.682), 0 };
  const char * nfrac[] = {"3S", "2S", "1S", "E" };
  for (int i = 0; i < nq_; ++i)
    {
      float xm1 = qslope1_[i].xlow(0.5);//median of slope1
      float xm3 = qslope3_[i].xlow(0.5);//median of slope3
      if(xm3==-std::numeric_limits<double>::max())
	xm3=0;
      if(xm1==-std::numeric_limits<double>::max())
	xm1=0;
      for(size_t j = 0; j < sizeof(fracs)/sizeof(float); ++j)
	{
	  if(!isnan(xm3)&&!isinf(xm3))
	    {
	      sprintf(str, "p3_p2_%s_%s", qname_[i],nfrac[j]);
	      TGraphAsymmErrors * g3 = hm_.h<TGraphAsymmErrors>("history", str);
	      g3->SetPoint(nSlope3_[i], t, xm3);
	      g3->SetPointEYlow(nSlope3_[i], xm3 - qslope3_[i].xlow(fracs[j]));
	      g3->SetPointEYhigh(nSlope3_[i], qslope3_[i].xhigh(fracs[j]) - xm3);
	      if(j==3)
		{
		  ++nSlope3_[i];
		}
	    }
	  if(!isnan(xm1)&&!isinf(xm1)&&xm3!=0)
	    {
	      sprintf(str, "p2_p1_%s_%s", qname_[i],nfrac[j]);
	      TGraphAsymmErrors * g1 = hm_.h<TGraphAsymmErrors>("history", str);
	      g1->SetPoint(nSlope1_[i], t, xm1);
	      g1->SetPointEYlow(nSlope1_[i], xm1 - qslope1_[i].xlow(fracs[j]));
	      g1->SetPointEYhigh(nSlope1_[i], qslope1_[i].xhigh(fracs[j]) - xm1);
	      if(j==3)
		++nSlope1_[i];
	    }
	}
    }
}




void EcalLaserPlotter::fill(const EcalLaserAPDPNRatios & apdpn, time_t t)
{
 reset_quantile();
        iov_last_ = t;

	static TProfile2D * p2d_weekly_norm_mean[3]; // FIXME - hope static arrays are zeroed by default
        static TProfile2D * p2d_weekly_norm_median[3]; // FIXME - hope static arrays are zeroed by default
        static TProfile2D * p2d_weekly[3]; // FIXME - hope static arrays are zeroed by default
	

	bzero(p2d_weekly, sizeof(p2d_weekly));
        bzero(p2d_weekly_norm_mean, sizeof(p2d_weekly_norm_mean));
        bzero(p2d_weekly_norm_median, sizeof(p2d_weekly_norm_median));
        if (iov_first_ == -1) iov_first_ = iov_last_;
        if (iov_last_ - il_ > 3600 * 24 * 7) {
	        il_ = iov_last_;
                sprintf(weekly_, "week_%ld", il_);
                for (int i = 0; i < NSUBDET; ++i) {
                        p2d_weekly[i] = 0;
                        p2d_weekly_norm_mean[i] = 0;
			p2d_weekly_norm_median[i] = 0;
		}
        }

	compute_averages(apdpn, t);

	
	float p2, p1, p3;
	

	//TProfile * p_eta_norm_median = 0;
	TProfile * p_eta_norm_mean = 0;
        char name[64];
        //sprintf(name, "p2_%ld", t);
        sprintf(name, "p2");
        for (size_t iid = 0; iid < ecalDetIds_.size(); ++iid) {
	       if (set_ch_status_ && ch_status_[iid] != 0) {
		  max_val_[iid] = 1.;
		  min_val_[iid] = 1.;
		  continue;
		}
                DetId id(ecalDetIds_[iid]);
		EcalLaserAPDPNRatios::EcalLaserTimeStamp ts;
		EcalLaserAPDPNRatios::EcalLaserTimeStampMap::const_iterator itLtime;
                int ix = -1, iy = -1, iz = -1;
		//r is  not used int r = -1;
                int isEB = 0;
		size_t iLM=0;
                if (id.subdetId() == EcalBarrel) { //setting up plots names
		        ecalPart[1] = 'B';
			ix = EBDetId(id).iphi();
                        iy = EBDetId(id).ieta();
			EBDetId ebid( id.rawId() );
                        iLM = MEEBGeom::lmr(ebid.ieta(), ebid.iphi());
			iz = 0;
                        isEB = 1;
                } else if (id.subdetId() == EcalEndcap) {
                        ecalPart[1] = 'E';
                        ix = EEDetId(id).ix();
                        iy = EEDetId(id).iy();
                        iz = EEDetId(id).zside();
			EEDetId eeid( id.rawId() );
                        // SuperCrystal coordinates
                        MEEEGeom::SuperCrysCoord iX = (eeid.ix()-1)/5 + 1;
                        MEEEGeom::SuperCrysCoord iY = (eeid.iy()-1)/5 + 1;    
			iLM = MEEEGeom::lmr(iX, iY, eeid.zside());
			if ( iLM-1 < apdpn.getTimeMap().size() ) {
			  ts = apdpn.getTimeMap()[iLM];
			}
			//			 r = sqrt((ix - 50) * (ix - 50) + (iy - 50) * (iy - 50));
                        //r isn't used currently
			if (iz > 0) {
                                eename[0] = 'p';
                                //eename[4] = ix < 50 ? '1' : '2';
                        } else {
                                eename[0] = 'n';
                                //eename[4] = ix < 50 ? '3' : '4';
                        }

                }
                EcalLaserAPDPNRatios::EcalLaserAPDPNRatiosMap::const_iterator itAPDPN;
                itAPDPN = apdpn.getLaserMap().find(id);
		if ( iLM-1 < apdpn.getTimeMap().size() ) {
		  ts = apdpn.getTimeMap()[iLM];
		}
		t1 = ts.t1.value()>>32;
		t2 = ts.t2.value()>>32;
		t3 = ts.t3.value()>>32;
		p1 = (*itAPDPN).p1;
		p2 = (*itAPDPN).p2;
		p3 = (*itAPDPN).p3;
		
		double slope3, slope1;
		if(!isinf((p2-p1)/(t2-t1))&&!isnan((p2-p1)/(t2-t1)))
                  slope1 = 3600*(p2-p1)/(t2-t1);
                else
                  slope1=-100.;
                if(!isinf((p3-p2)/(t3-t2))&&!isnan((p3-p2)/(t3-t2)))
                  slope3 =3600*(p3-p2)/(t3-t2);
                else
                  slope3=-100.;
		
		
		  /*
		if(ix<171&&ix>165&&iy<-25&&iy>-31&&iz==0)
		  std::cout << "ix : " << ix << " iy : " << iy << " p2 " << p2 << std::endl;
                if((iz==-1)&(
		   (ix==83&&iy==43)||
		   (ix==84&&iy==44)||
		   (ix==33&&iy==58)||
		   (ix==35&&iy==87)||
		   (ix==58&&iy==88)||
		   (ix==38&&iy==5)||
		   (ix==62&&iy==32)||
		   (ix==70&&iy==39)))
		  std::cout << "ix : " << ix << " iy : " << iy << " iz " << iz << " p2 " << p2 << std::endl;
		*/
		   
		
		const char * subdet[NSUBDET] =  { "nZ_", "" , "pZ_" };

                static TH2D * h2d_infp[3]; // FIXME - hope static arrays are zeroed by default
                static TH2D * h2d_infm[3]; // FIXME - hope static arrays are zeroed by default
                static TH2D * h2d_nan[3]; // FIXME - hope static arrays are zeroed by default
				

		const char * temhl[] = { "EEh2", "EBh2"};
		
                if (isinf(p2) > 0) {
                        p2 =  FLT_MAX;
                        sprintf(str, "%sinfp", subdet[iz+1]);
                        hm_.h<TH2D>(temhl[isEB], str, &h2d_infp[iz + 1])->Fill(ix, iy);
                        inf_.insert((int)id);
                        continue;
                } else if (isinf(p2) < 0) {
                        p2 = -FLT_MAX;
                        sprintf(str, "%sinfm", subdet[iz+1]);
                        hm_.h<TH2D>(temhl[isEB], str, &h2d_infm[iz + 1])->Fill(ix, iy);
                        inf_.insert(-(int)id);
                        continue;
                }
                if (isnan(p2)) {
                        p2 =  FLT_MAX;
                        sprintf(str, "%snan", subdet[iz+1]);
                        hm_.h<TH2D>(temhl[isEB], str, &h2d_nan[iz + 1])->Fill(ix, iy);
                        nan_.insert((int)id);
                        continue;
                }

                max_min(id, p2);

                float eta = geom_eta(id);
                static TH1D * h_eta_norm_mean = 0;
		static TH1D * h_eta_norm_median = 0;
		static TH1D * h_eta_mean = 0;
		static TH1D * h_p2 = 0;
		//static TH2D * Ntemp=0;
		//static TH2D * temp=0;
		//static TH2D * Mtemp=0;
                hm_.h<TProfile>("etaProf", "p2", &p_eta_norm_mean);
		float p2_mean = p_eta_norm_mean->GetBinContent(p_eta_norm_mean->FindBin(eta));
                float p2_median = q_[qetaoffs_+etabin(eta)].xlow(.5); //needs smaller bins
		
		


		if(OUTPUTBADTIMESFILE==ON)
{
  if(t1<1330000000 || t2<1330000000 || t3<1330000000) 
     std::cerr << "t1 " << t1 << " t2 " << t2 << " t3 " << t3 << " ix " << ix << " iy " << iy << " iz " << iz << " p2 " << p2 << std::endl;
 }
		if(OUTPUTBADFILE==ON)
  {
    if(p2==1.0||p2==0)
      std::cerr << "t1 " << t1 << " t3 " << t3 << " ix " << ix << " iy " << iy << " iz " << iz << " p2 " << p2 << std::endl;
    // if(p2 <.4||p2>1.4)
    // std::cerr << "t1 " << t1 << " t3 " << t3 << " ix " << ix << " iy " << iy << " iz " << iz << " p2 " << p2 << std::endl;
  }	
				
		/*
		  if(p2/p2_mean==1.0||p2==0)
		  std::cout << "ix : " << ix << " iy : " << iy << " p2 " << p2 << std::endl;
		  if(p2/p2_mean <.4||p2/p2_mean>1.4)
		  std::cout << "ix : " << ix << " iy : " << iy << " p2 " << p2/p2_mean << std::endl;
		*/
		//std::cout <<"mean " << p2_mean << " median " << p2_median << std::endl;
		hm_.h<TH1D>("distr", "eta_normalised_median_p2", &h_eta_norm_median)->Fill((double)(p2 / p2_median));
		hm_.h<TH1D>("distr", "eta_normalised_mean_p2", &h_eta_norm_mean)->Fill((double)(.984*p2 / p2_mean));
		//hm_.h<TH2D>("IOVdistr", "eta_normalized_Mean_distribution",&Ntemp)->Fill((double)(.984*p2/p2_mean),niov_);
		//hm_.h<TH2D>("IOVdistr", "eta_distribution",&temp)->Fill((double)(p2),niov_);
		//hm_.h<TH2D>("IOVdistr", "eta_normalized_Median_distribution",&Mtemp)->Fill((double)(p2/p2_median),niov_);
		hm_.h<TH1D>("distr", "eta_mean_p2", &h_eta_mean)->Fill((double)p2_mean);
		hm_.h<TH1D>("distr", "p2", &h_p2)->Fill(p2);
		
                const char* templ[] = { "EEprof2", "EBprof2"};
		
                static TProfile2D * p2d_all[3]; // FIXME - hope static arrays are zeroed by default
                static TProfile2D * p2d_all_norm_mean[3]; // FIXME - hope static arrays are zeroed by default
		static TProfile2D * p2d_all_norm_median[3]; // FIXME - hope static arrays are zeroed by default

                sprintf(str, "%sp2", subdet[iz+1]);
                hm_.h<TProfile2D>(templ[isEB], str, &p2d_all[iz + 1])->Fill(ix, iy, p2);
                sprintf(str, "%sp2%s", subdet[iz+1], weekly_);
                hm_.h<TProfile2D>(templ[isEB], str, &p2d_weekly[iz + 1])->Fill(ix, iy, p2);
		

                sprintf(str, "%sp2Norm_Median", subdet[iz+1]);
                hm_.h<TProfile2D>(templ[isEB], str, &p2d_all_norm_median[iz + 1])->Fill(ix, iy, p2 / p2_median);
                if(iid==2) hm_.h<TProfile2D>(templ[iz+1], str, &p2d_all_norm_median[iz + 1])->SetErrorOption("s");
                sprintf(str, "%sp2Norm_Median_%s", subdet[iz+1], weekly_);
                hm_.h<TProfile2D>(templ[isEB], str, &p2d_weekly_norm_median[iz + 1])->Fill(ix, iy, p2 / p2_median);
                if(iid==2) hm_.h<TProfile2D>(templ[isEB], str, &p2d_weekly_norm_median[iz + 1])->SetErrorOption("s");

		 sprintf(str, "%sp2Norm_Mean", subdet[iz+1]);
                hm_.h<TProfile2D>(templ[isEB], str, &p2d_all_norm_mean[iz + 1])->Fill(ix, iy, p2 / p2_mean);
                if(iid==2) hm_.h<TProfile2D>(templ[iz+1], str, &p2d_all_norm_mean[iz + 1])->SetErrorOption("s");
                sprintf(str, "%sp2Norm_Mean_%s", subdet[iz+1], weekly_);
                hm_.h<TProfile2D>(templ[isEB], str, &p2d_weekly_norm_mean[iz + 1])->Fill(ix, iy, p2 / p2_mean);
                if(iid==2) hm_.h<TProfile2D>(templ[isEB], str, &p2d_weekly_norm_mean[iz + 1])->SetErrorOption("s");
		
		
		static TProfile2D * h2d_slope_p3[3];
		static TProfile2D * h2d_slope_p1[3];
		//static TProfile * slope_p1;
		//static TProfile * slope_p3;
		static TProfile2D * h2d_slope_n3[3];
		static TProfile2D * h2d_slope_n1[3];
		//static TProfile * slope_n1;
		//static TProfile * slope_n3;
		static TH2D* p2_p1_pcut;
		static TH2D* p2_p1_ncut;
		static TH2D* p3_p2_pcut;
		static TH2D* p3_p2_ncut;
		static TProfile* p2_p1_cut_x;
		static TProfile* p3_p2_cut_x;
		static TProfile* p2_p1_pcut_x;
		static TProfile* p2_p1_ncut_x;
		static TProfile* p3_p2_pcut_x;
		static TProfile* p3_p2_ncut_x;
		static TH2D* p2d_slope_p3[3];
		static TH2D* p2d_slope_p1[3];
		//static TH2D* p2d_slope_p3_corr[3];
		//static TH2D* p2d_slope_p1_corr[3];
		
		
		sprintf(str,"%s_p2_p1_slope",temhl[isEB]);
		if(slope1!=-100&&slope1!=0)
		  hm_.h<TH2D>("Slope2",str,&p2d_slope_p1[iz+1])->Fill(p2,slope1);
		sprintf(str,"%s_p3_p2_slope",temhl[isEB]);
		if(slope3!=-100&&slope3!=0)
		  hm_.h<TH2D>("Slope2",str,&p2d_slope_p3[iz+1])->Fill(p2,slope3);

		/*
		sprintf(str,"%s_p2_p1_slope_corrected",temhl[isEB]);
		if(slope1!=-100&&slope1!=0)
		  hm_.h<TH2D>("Slope2",str,&p2d_slope_p1_corr[iz+1])->Fill(p2/p2_mean,slope1);
		sprintf(str,"%s_p3_p2_slope_corrected",temhl[isEB]);
		if(slope3!=-100&&slope3!=0)
		  hm_.h<TH2D>("Slope2",str,&p2d_slope_p3_corr[iz+1])->Fill(p2/p2_mean,slope3);
		*/
		
		hm_.h<TProfile>("etaProf","p2_p1_cut_x",&p2_p1_cut_x)->Fill(eta,slope1);
		hm_.h<TProfile>("etaProf","p3_p2_cut_x",&p3_p2_cut_x)->Fill(eta,slope3);
		
		if(slope1>0.0)
		  {
		    sprintf(str,"%sp2_p1_pslope",subdet[iz+1]);
		    hm_.h<TProfile2D>(templ[isEB],str,&h2d_slope_p1[iz+1])->Fill(ix,iy,slope1);
		    //hm_.h<TProfile>("etaProf","p2_p1_pslope",&slope_p1)->Fill(eta,slope1);
		    hm_.h<TH2D>("slope_eta","p2_p1_pcut",&p2_p1_pcut)->Fill(eta,slope1);
		    hm_.h<TProfile>("etaProf","p2_p1_pcut_x",&p2_p1_pcut_x)->Fill(eta,slope1);
		  }
		if(slope1<0.0&&slope1!=-100)
		  {
		    sprintf(str,"%sp2_p1_nslope",subdet[iz+1]);
		    hm_.h<TProfile2D>(templ[isEB],str,&h2d_slope_n1[iz+1])->Fill(ix,iy,-slope1);
		    //hm_.h<TProfile>("etaProf","p2_p1_nslope",&slope_n1)->Fill(eta,slope1);
		    hm_.h<TH2D>("slope_eta","p2_p1_ncut",&p2_p1_ncut)->Fill(eta,-slope1);
		    hm_.h<TProfile>("etaProf","p2_p1_ncut_x",&p2_p1_ncut_x)->Fill(eta,-slope1);
		  }
		if(slope3>0.0)
		  {
		    sprintf(str,"%sp3_p2_pslope",subdet[iz+1]);
		    hm_.h<TProfile2D>(templ[isEB],str,&h2d_slope_p3[iz+1])->Fill(ix,iy,slope3);
		    //hm_.h<TProfile>("etaProf","p3_p2_pslope",&slope_p3)->Fill(eta,slope3);
		    hm_.h<TH2D>("slope_eta","p3_p2_pcut",&p3_p2_pcut)->Fill(eta,slope3);
		    hm_.h<TProfile>("etaProf","p3_p2_pcut_x",&p3_p2_pcut_x)->Fill(eta,slope3);
		  }
		if(slope3<0.0&&slope3!=-100)
		  {
		    sprintf(str,"%sp3_p2_nslope",subdet[iz+1]);
		    hm_.h<TProfile2D>(templ[isEB],str,&h2d_slope_n3[iz+1])->Fill(ix,iy,-slope3);
		    //hm_.h<TProfile>("etaProf","p3_p2_nslope",&slope_n3)->Fill(eta,slope3);
		    hm_.h<TH2D>("slope_eta","p3_p2_ncut",&p3_p2_ncut)->Fill(eta,-slope3);
		    hm_.h<TProfile>("etaProf","p3_p2_ncut_x",&p3_p2_ncut_x)->Fill(eta,-slope3);
		  }
		
		char str2[64];
		char str1[64];
		static TProfile2D * bad_channel_map[3][8000];
		static TH2D * bad_channel_map_eta;
		static TH2D * total_channel_map_eta;
		static TH2D* bad_channel_map_LM;
		static TH2D * bad_channel_summary_map[3];
		static TH2D * bad_timing_summary_map[3][3];
		//static TH2D* p2_map[3][8000];
		static TH2D* propagation_map[3][2];

		if(niov_>=1000)
		  sprintf(str, "%sBadChannel_%i", subdet[iz+1], niov_);  //(if else)s to make sure the plots are displayed in the correct order 
		else if(niov_>=100)
		  sprintf(str, "%sBadChannel_0%i", subdet[iz+1], niov_);
		else if(niov_>=10)
                  sprintf(str, "%sBadChannel_00%i", subdet[iz+1], niov_);
		else
		  sprintf(str, "%sBadChannel_000%i", subdet[iz+1], niov_);
		
		sprintf(str1, "%sSingleIOVp2_%i", subdet[iz+1], niov_);
		
	
		sprintf(str2, "%spropagation_map", subdet[iz+1]);
		if(niov_!=0)
		  {
		    if(hm_.h<TH2D>(temhl[isEB],"p2_map_last_IOV",&propagation_map[iz+1][1])->GetBinContent(ix,iy)==p2)
		      hm_.h<TH2D>(temhl[isEB], str2,&propagation_map[iz+1][0])->Fill(ix,iy);
		  }
		hm_.h<TH2D>(temhl[isEB],"p2_map_last_IOV",&propagation_map[iz+1][1])->SetBinContent(ix,iy,1);
		
		char sumName[256]; 
		if(t1<1330000000)
		  {
		    sprintf(sumName,"%s%s_Bad_Time_Summary",subdet[iz+1],"t1");
		    hm_.h<TH2D>(temhl[isEB],sumName, &bad_timing_summary_map[iz+1][0])->Fill(ix,iy,1);
		  }
		if(t2<1330000000)
		  {
		    sprintf(sumName,"%s%s_Bad_Time_Summary",subdet[iz+1],"t2");
		    hm_.h<TH2D>(temhl[isEB],sumName, &bad_timing_summary_map[iz+1][1])->Fill(ix,iy,1);
		  }
		if(t3<1330000000) 
		  {
		    sprintf(sumName,"%s%s_Bad_Time_Summary",subdet[iz+1],"t3");
		    hm_.h<TH2D>(temhl[isEB],sumName, &bad_timing_summary_map[iz+1][2])->Fill(ix,iy,1);
		  }
		sprintf(sumName,"%sBadChannel_Summary",subdet[iz+1]);
		//hm_.h<TH2D>(temhl[isEB],str1,&p2_map[iz+1][niov_])->Fill(ix,iy,p2);
		
		hm_.h<TH2D>("channel_eta","temp", &total_channel_map_eta)->Fill(eta,niov_,1);
		if(p2==1.0) //bad channel
		  {
		    hm_.h<TH2D>(temhl[isEB],sumName, &bad_channel_summary_map[iz+1])->Fill(ix,iy,1);
		    //hm_.h<TProfile2D>(templ[isEB], str, &bad_channel_map[iz + 1][niov_])->Fill(ix, iy, -p2);
		    hm_.h<TH2D>("channel_eta","bad_channel_map", &bad_channel_map_eta)->Fill(eta,niov_,p2);
		    hm_.h<TH2D>("channel_LM","bad_channel_map_LM",&bad_channel_map_LM)->Fill(iLM,niov_,p2);
		  }
		else
		  {
 		    //hm_.h<TProfile2D>(templ[isEB], str, &bad_channel_map[iz + 1][niov_])->Fill(ix, iy, 1.0);
		  }
//for changed file output
if(OUTPUTCHANGEFILE==ON)
  if(niov_>0)
  {
    int tiy,tix;
    if(isEB==1)
      {
	tix=ix-1;
	tiy=85+iy;
      }
    else
      {
	tix=ix; 
	tiy=iy;
      }
    if(p2==1.0) //if bad
      {
	if((hm_.h<TProfile2D>(templ[isEB], str, &bad_channel_map[iz + 1][niov_-1])->GetBinContent(tix+1, tiy+1))!=-1.0) //but was good last IOV
	    //std::cout << "t1 " << t1 << " ix " << ix << " iy " << iy << " iz " << iz << " status_change_to " << -1 << std::endl; //print
	  std::cout << "t1 " << t1 << " t3 " << t3 << " ix " << ix << " iy " << iy << " iz " << iz << " status_change_from " << hm_.h<TProfile2D>(templ[isEB], str, &bad_channel_map[iz + 1][niov_-1])->GetBinContent(tix+1, tiy+1) << " to " << -1 << std::endl; //print
      }
    else //if good 
      if((hm_.h<TProfile2D>(templ[isEB], str, &bad_channel_map[iz + 1][niov_-1])->GetBinContent(tix+1, tiy+1))==-1.0) //but was bad last IOV
	//std::cout << "t1 " << t1 << " ix " << ix << " iy " << iy << " iz " << iz << " status_change_to " << 1 << std::endl; //print
	  std::cout << "t1 " << t1 << " t3 " << t3 << " ix " << ix << " iy " << iy << " iz " << iz << " status_change_from "<< hm_.h<TProfile2D>(templ[isEB], str, &bad_channel_map[iz + 1][niov_-1])->GetBinContent(tix+1, tiy+1) << " to "  << 1 << std::endl; //print
  }
                 
		  /********************************/
		

        }
	        if(MAKEHISTORIES)
		  {
		    fill_histories(t);
		    fill_slope_histories(t);
		  }
		++niov_;
}


void EcalLaserPlotter::printText()
{
  
  for(int i=0;i<((int) bad_channels_.size());++i) 
    std::cerr << bad_channels_[i][0] << " " << bad_channels_[i][1] << " " << bad_channels_[i][2] << " " << bad_channels_[i][3] << std::endl;
}
