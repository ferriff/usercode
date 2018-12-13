#ifndef __ECAL_LASER_PLOTTER
#define __ECAL_LASER_PLOTTER

#include <ctime>
#include <vector>
#include <set>

#include "CondFormats/EcalObjects/interface/EcalChannelStatus.h"
#include "usercode/DBDump/interface/Quantile.h"
#include "usercode/DBDump/interface/HistoManager.h"

class DetId;
class EcalLaserAPDPNRatios;

#define NSUBDET 3
#define ON 1
#define OFF 0
#define OUTPUTBADFILE OFF  
#define OUTPUTCHANGEFILE OFF
#define OUTPUTBADTIMESFILE OFF
#define MAKEHISTORIES ON

class EcalLaserPlotter {
        public:
                EcalLaserPlotter(const char * geom_filename = "detid_geom.dat");
                ~EcalLaserPlotter() { printSummary(); };

                void fill(const EcalLaserAPDPNRatios &, time_t);
                void printSummary();
                void save(const char * filename = "ecallaserplotter.root", const char * opt = "RECREATE");
                void setEcalChannelStatus(const EcalChannelStatus & chStatus, int onceForAll = 0);
                void setEcalGeometry(const char * geom_filename = "detid_geom.dat");
		void printText();
        private:
		time_t il_;
                char weekly_[128];

                void reset_quantile();
                int etabin(float eta);
                float geom_eta(DetId id);
                void compute_averages(const EcalLaserAPDPNRatios & apdpn, time_t t);
                void fill_histories(time_t t);
		void fill_slope_histories(time_t t);
		void max_min(DetId id, float val);
                void max_min_plots();

                //number of bins for plots of corrections in an eta ring
                const static int netabins_ = 20;

                // all, EE-, EB-, EB+, EE+, 92 LMR, netabins eta ring
                const static int nq_ = 97 + netabins_;
                const static int qetaoffs_ = 97;

		Quantile<int> q_[nq_];  //p2 values
		Quantile<int> qslope1_[nq_];
		Quantile<int> qslope3_[nq_];
		//TH1F th_[nq_];
		 //	Qunatile<int> qeta_[250]; //used for to calculate median for normalization 
		
		int t1, t2, t3;
		
		char qname_[nq_][32];

                HistoManager hm_;

                // number of IOVs, first and last
                int nSlope1_[nq_];
		int nSlope3_[nq_];
		int niov_;
		time_t iov_first_;
                time_t iov_last_;

                std::vector<DetId> ecalDetIds_;
                std::vector<float> max_val_;
                std::vector<float> min_val_;
                std::vector<float> geom_eta_;
                std::vector<uint16_t> ch_status_;
		std::vector< std::vector<int> > bad_channels_;
		std::vector< std::vector<int> > previous_bad_channels_;
		std::vector< std::vector<int> > current_bad_channels_;
                std::set<int> inf_; // +/- DetId according to inf sign
                std::set<int> nan_; // DetId

                char ecalPart[3];
                char eename[3];

                char str[256];

                int set_geom_;
                int set_ch_status_;
};


#endif
