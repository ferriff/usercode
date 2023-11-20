#include "usercode/DBDump/interface/EcalLaserGreenAnalysis.h"

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

EcalLaserGreenAnalysis::EcalLaserGreenAnalysis(const char * geom_filename) :
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
        hm_.addTemplate<TProfile>("etaProf", new TProfile("etaProf", "etaProf", 250, -2.75, 2.75));

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

void EcalLaserGreenAnalysis::reset_quantile()
{
        for (int i = 0; i < nq_; ) q_[i++].reset();
}

void EcalLaserGreenAnalysis::save(const char * filename, const char * opt)
{
        max_min_plots();
        max_min_values();
        hm_.save(filename, opt);
}

int EcalLaserGreenAnalysis::etabin(float eta)
{
        const float etamin  =  -2.964;
        const float etamax  =  2.964;
        //		if(!(etamin < eta && eta < etamax)){ printf("---> %f\n", eta); }
        assert(etamin < eta && eta < etamax);
        return int((eta - etamin) / (etamax - etamin) * netabins_);
}

float EcalLaserGreenAnalysis::geom_eta(DetId id)
{
        if (id.subdetId() == EcalBarrel) {
                return geom_eta_[EBDetId(id).hashedIndex()];
        } else if (id.subdetId() == EcalEndcap) {
                return geom_eta_[EBDetId::MAX_HASH + 1 + EEDetId(id).denseIndex()];
        } else {
                assert(0);
        }
}

void EcalLaserGreenAnalysis::max_min(DetId id, float val)
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

void EcalLaserGreenAnalysis::max_min_plots()
{
        int ix = -1, iy = -1, iz = -1;
        int isEB = 0;
        const char * subdet[NSUBDET] =  { "nZ_", "" , "pZ_" };
        static TH2D * h2d_max[3]; // FIXME - hope static arrays are zeroed by default
        static TH2D * h2d_min[3]; // FIXME - hope static arrays are zeroed by default
        const char * temhl[] = { "EEh2", "EBh2"};
        for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
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

void EcalLaserGreenAnalysis::max_min_values()
{
        FILE * fout = fopen("test_max_min_values.dat", "w");
        for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
                fprintf(fout, "%d", ecalDetIds_[i].rawId());
                //for (size_t j = 0; j < max_values_[i].size(); ++j) {
                std::set<float>::const_iterator c;
                for (c = max_values_[i].begin(); c != max_values_[i].end(); ++c) {
                        fprintf(fout, " %f", *c);
                }
                fprintf(fout, "  -- ");
                for (c = min_values_[i].begin(); c != min_values_[i].end(); ++c) {
                        fprintf(fout, " %f", *c);
                }
                fprintf(fout, "\n");
        }
        fclose(fout);
}

void EcalLaserGreenAnalysis::setEcalGeometry(const char * geom_filename)
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

void EcalLaserGreenAnalysis::setEcalChannelStatus(const EcalChannelStatus & chStatus, int onceForAll)
{
        if (onceForAll && set_ch_status_) return;
        if (!set_ch_status_) ch_status_.resize(ecalDetIds_.size());
        FILE * ftmp = NULL;
        if (!set_ch_status_) ftmp = fopen("channelStatus.dump", "w");
        for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
                ch_status_[i] = chStatus.find(ecalDetIds_[i])->getStatusCode();
                //if (!set_ch_status_) {
                //        DetId id(ecalDetIds_[i]);
                //        int ix = -1, iy = -1, iz = -1;
                //        if (id.subdetId() == EcalBarrel) {
                //                ix = EBDetId(id).iphi();
                //                iy = EBDetId(id).ieta();
                //                iz = 0;
                //        } else if (id.subdetId() == EcalEndcap) {
                //                ix = EEDetId(id).ix();
                //                iy = EEDetId(id).iy();
                //                iz = EEDetId(id).zside();
                //        }
                //        fprintf(ftmp, "%d %d %d  %d\n", ix, iy, iz, ch_status_[i]);
                //}
        }
        if (!set_ch_status_ && ftmp) fclose(ftmp);
        set_ch_status_ = 1;
}

void EcalLaserGreenAnalysis::printSummary()
{
        printf("%d IOV(s) analysed.\n", niov_);
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

void EcalLaserGreenAnalysis::compute_averages(const EcalLaserAPDPNRatios & apdpn, time_t t)
{
        static TProfile * p = 0;
        char name[64];
        //sprintf(name, "p2_%ld", t);
        sprintf(name, "p2");
        hm_.h<TProfile>("etaProf", name, &p)->Reset();
        for (size_t iid = 0; iid < ecalDetIds_.size(); ++iid) {
                if (set_ch_status_ && ch_status_[iid] != 0) continue;
                DetId id(ecalDetIds_[iid]);
                EcalLaserAPDPNRatios::EcalLaserAPDPNRatiosMap::const_iterator itAPDPN;
                EcalLaserAPDPNRatios::EcalLaserTimeStamp ts;
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
                float eta = 99999;
                p2 = (*itAPDPN).p1; // FIXME

                if (std::isinf(p2) > 0) {
                        p2 =  FLT_MAX;
                } else if (std::isinf(p2) < 0) {
                        p2 = -FLT_MAX;
                }
                if (std::isnan(p2)) {
                        p2 =  FLT_MAX;
                }

                eta = geom_eta(id);

                hm_.h<TProfile>("etaProf", name, &p)->Fill(eta, p2);
                q_[0].fill(p2, iid);
                if (id.subdetId() == EcalBarrel) {
                        if (EBDetId(id).ieta() < 0) q_[2].fill(p2, iid);
                        else                        q_[3].fill(p2, iid);
                } else {
                        if (EEDetId(id).zside() < 0) q_[1].fill(p2, iid);
                        else                         q_[4].fill(p2, iid);
                }
                q_[5 + iLM - 1].fill(p2, iid);
                q_[qetaoffs_ + etabin(eta)].fill(p2, iid);
        }
}

void EcalLaserGreenAnalysis::fill_histories(time_t t)
{
        float fracs[] = { 0.5 * (1 - 0.997), 0.5 * (1 - 0.954), 0.5 * (1 - 0.682), 0 };
        const char * nfrac[] = { "3S", "2S", "1S", "E" };
        char str[128];
        for (int i = 0; i < nq_; ++i) {
                float xm = q_[i].xlow(0.5);
                for (size_t j = 0; j < sizeof(fracs)/sizeof(float); ++j) {
                        sprintf(str, "p2_%s_%s", qname_[i], nfrac[j]);
                        TGraphAsymmErrors * g = hm_.h<TGraphAsymmErrors>("history", str);
                        g->SetPoint(niov_, t, xm);
                        g->SetPointEYlow(niov_, xm - q_[i].xlow(fracs[j]));
                        g->SetPointEYhigh(niov_, q_[i].xhigh(fracs[j]) - xm);
                }
        } 
        ++niov_;
}

void EcalLaserGreenAnalysis::fill(const EcalLaserAPDPNRatios & apdpn, time_t t)
{
        reset_quantile();
        iov_last_ = t;
        static TProfile2D * p2d_weekly_norm[3]; // FIXME - hope static arrays are zeroed by default
        static TProfile2D * p2d_weekly[3]; // FIXME - hope static arrays are zeroed by default
        bzero(p2d_weekly, sizeof(p2d_weekly));
        bzero(p2d_weekly_norm, sizeof(p2d_weekly_norm));
        if (iov_first_ == -1) iov_first_ = iov_last_;
        if (iov_last_ - il_ > 3600 * 24 * 7) {
                il_ = iov_last_;
                sprintf(weekly_, "week_%ld", il_);
                for (int i = 0; i < NSUBDET; ++i) {
                        p2d_weekly[i] = 0;
                        p2d_weekly_norm[i] = 0;
                }
        }
        compute_averages(apdpn, t);
        float p2;
        TProfile * p_eta_norm = 0;
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

                int ix = -1, iy = -1, iz = -1; //, r = -1;
                int isEB = 0;

                if (id.subdetId() == EcalBarrel) {
                        ecalPart[1] = 'B';
                        ix = EBDetId(id).iphi();
                        iy = EBDetId(id).ieta();
                        iz = 0;
                        isEB = 1;
                } else if (id.subdetId() == EcalEndcap) {
                        ecalPart[1] = 'E';
                        ix = EEDetId(id).ix();
                        iy = EEDetId(id).iy();
                        iz = EEDetId(id).zside();
                        //r = sqrt((ix - 50) * (ix - 50) + (iy - 50) * (iy - 50));
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
                p2 = (*itAPDPN).p2; // FIXME

                size_t ns = max_values_[iid].size();
                //if (iid == 11) fprintf(stderr, "%f %f\n", p2, *(max_values_[iid].begin()++));
                if (ns < NMAXVAL) {
                        max_values_[iid].insert(p2);
                } else if (p2 > *(max_values_[iid].begin()++)) {
                        if (ns == NMAXVAL) max_values_[iid].erase(max_values_[iid].begin());
                        max_values_[iid].insert(p2);
                }
                ns = min_values_[iid].size();
                //if (iid == 11) fprintf(stderr, "%f %f\n", p2, *(min_values_[iid].begin()++));
                if (ns < NMAXVAL) {
                        min_values_[iid].insert(p2);
                } else if (p2 < *(min_values_[iid].begin()++)) {
                        if (ns == NMAXVAL) min_values_[iid].erase(min_values_[iid].begin());
                        min_values_[iid].insert(p2);
                }
                //if (iid == 11) getchar();
                //fprintf(stderr, "%lu\n", iid);  getchar();


                const char * subdet[NSUBDET] =  { "nZ_", "" , "pZ_" };

                static TH2D * h2d_infp[3]; // FIXME - hope static arrays are zeroed by default
                static TH2D * h2d_infm[3]; // FIXME - hope static arrays are zeroed by default
                static TH2D * h2d_nan[3]; // FIXME - hope static arrays are zeroed by default
                const char * temhl[] = { "EEh2", "EBh2"};
                if (std::isinf(p2) > 0) {
                        p2 =  FLT_MAX;
                        sprintf(str, "%sinfp", subdet[iz+1]);
                        hm_.h<TH2D>(temhl[isEB], str, &h2d_infp[iz + 1])->Fill(ix, iy);
                        inf_.insert((int)id);
                        continue;
                } else if (std::isinf(p2) < 0) {
                        p2 = -FLT_MAX;
                        sprintf(str, "%sinfm", subdet[iz+1]);
                        hm_.h<TH2D>(temhl[isEB], str, &h2d_infm[iz + 1])->Fill(ix, iy);
                        inf_.insert(-(int)id);
                        continue;
                }
                if (std::isnan(p2)) {
                        p2 =  FLT_MAX;
                        sprintf(str, "%snan", subdet[iz+1]);
                        hm_.h<TH2D>(temhl[isEB], str, &h2d_nan[iz + 1])->Fill(ix, iy);
                        nan_.insert((int)id);
                        continue;
                }

                max_min(id, p2);

                float eta = geom_eta(id);
                static TH1D * h_eta_norm = 0;
                hm_.h<TProfile>("etaProf", name, &p_eta_norm);
                float p2_mean = p_eta_norm->GetBinContent(p_eta_norm->FindBin(eta));
                hm_.h<TH1D>("distr", "eta_normalised_p2", &h_eta_norm)->Fill(p2 / p2_mean);

                //const char* templ[] = { "EEprof2", "EBprof2"};

                //static TProfile2D * p2d_all[3]; // FIXME - hope static arrays are zeroed by default
                //static TProfile2D * p2d_all_norm[3]; // FIXME - hope static arrays are zeroed by default

                ///FIXME/// sprintf(str, "%sp2", subdet[iz+1]);
                ///FIXME/// hm_.h<TProfile2D>(templ[isEB], str, &p2d_all[iz + 1])->Fill(ix, iy, p2);
                ///FIXME/// sprintf(str, "%sp2%s", subdet[iz+1], weekly_);
                ///FIXME/// hm_.h<TProfile2D>(templ[isEB], str, &p2d_weekly[iz + 1])->Fill(ix, iy, p2);

                ///FIXME/// sprintf(str, "%sp2Norm", subdet[iz+1]);
                ///FIXME/// hm_.h<TProfile2D>(templ[isEB], str, &p2d_all_norm[iz + 1])->Fill(ix, iy, p2 / p2_mean); // FIXME
                ///FIXME/// //hm_.h<TProfile2D>(templ[isEB], str, &p2d_all_norm[iz + 1])->Fill(ix, iy, p2);
                ///FIXME/// if(iid==2) hm_.h<TProfile2D>(templ[iz+1], str, &p2d_all_norm[iz + 1])->SetErrorOption("s");
                ///FIXME/// sprintf(str, "%sp2Norm_%s", subdet[iz+1], weekly_);
                ///FIXME/// hm_.h<TProfile2D>(templ[isEB], str, &p2d_weekly_norm[iz + 1])->Fill(ix, iy, p2 / p2_mean); // FIXME
                ///FIXME/// //hm_.h<TProfile2D>(templ[isEB], str, &p2d_weekly_norm[iz + 1])->Fill(ix, iy, p2);
                ///FIXME/// if(iid==2) hm_.h<TProfile2D>(templ[isEB], str, &p2d_weekly_norm[iz + 1])->SetErrorOption("s");

                //if (isEB && (p2 > 1.578 || p2 < 0.634)) printf("-->kPoorCalib EB %d %d %d %d %f\n", id.rawId(), ix, iy, iz, p2);
                //else if ((!isEB) && (p2 > 1.578 || p2 < 0.485)) printf("-->kPoorCalib EE %d %d %d %d %f\n", id.rawId(), ix, iy, iz, p2);
        }
        fill_histories(t);
}
