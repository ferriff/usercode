#include "CondCore/Utilities/interface/Utilities.h"
#include "CondCore/CondDB/interface/ConnectionPool.h"
#include "CondCore/CondDB/interface/IOVProxy.h"

#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatios.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRcd.h"
#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatiosRef.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRefRcd.h"

#include "Calibration/Tools/interface/DRings.h"

#include "TFile.h"
#include "TH1F.h"

#include <boost/program_options.hpp>
#include <iterator>
#include <iostream>

using namespace std;

float _svalues[2 * DRings::nHalfIEta + 1];
float _rms3x3[2 * DRings::nHalfIEta + 1];
float _rms5x5[2 * DRings::nHalfIEta + 1];
int   _nvalues[2 * DRings::nHalfIEta + 1];
std::vector<float> _rmseta[2 * DRings::nHalfIEta + 1];

float rms(std::vector<float> & v)
{
        float sum = 0.;
        size_t size = v.size();
        for (size_t i = 0; i < size; ++i) {
                sum += v[i];
        }
        float av = sum / size;
        sum = 0.;
        for (size_t i = 0; i < size; ++i) {
                sum += (v[i] - av) * (v[i] - av);
        }
        // relative rms
        return sqrt(sum / (size - 1)) / av;
}

namespace cond {
        class APDPNRatioHgg : public Utilities {
                public:
                        typedef EcalLaserAPDPNRatios A;
                        typedef EcalLaserAPDPNRatios::EcalLaserAPDPNRatiosMap AMap;
                        typedef EcalLaserAPDPNRatios::EcalLaserAPDPNRatiosMap::const_iterator AMapCit;
                        typedef EcalLaserAPDPNRatios::EcalLaserAPDPNpair AP;
                        typedef EcalLaserAPDPNRatios::EcalLaserTimeStamp AT;

                        APDPNRatioHgg();
                        ~APDPNRatioHgg();
                        int execute();
                        void dump_ring_average(FILE * fd, time_t tb, time_t te, TFile * fout);
                        void dump_txt(FILE * fd);
                        void dump_eta_rings(FILE * fd);
                        void fill_ring_average(const A & obja);
                        void reset_ring_average();
                private:
                        std::vector<DetId> ecalDetIds_;
                        DRings dr_;
                        TH1F * h_;
                        bool good_[75848];
                        void read_chstatus();
        };

}

cond::APDPNRatioHgg::APDPNRatioHgg():Utilities("cmscond_list_iov")
{
        addAuthenticationOptions();
        addOption<bool>("verbose","v","verbose");
        addOption<bool>("all","a","list all tags (default mode)");
        addOption<std::string>("connect","c","connect to a specific db");
        addOption<cond::Time_t>("beginTime","b","begin time (first since) (optional)");
        addOption<cond::Time_t>("endTime","e","end time (last till) (optional)");
        addOption<std::string>("tag","t","first tag to be compared");
        addOption<int>("niov","n","number of IOV");
        addOption<int>("prescale","s","prescale factor");
        addOption<std::string>("dump", "d", "file for a txt dump");
        addOption<time_t>("deltaTime", "D", "time in [s] over which compute the average");
        //addOption<bool>("flat", "f", "flat iov, i.e. p1 = p2 = p3");
        addOption<bool>("txt", "x", "dump a txt file as per IC coeff. (NB: the time information of t1, t2, t3 is lost)");
        addOption<bool>("shift", "S", "in the filename, time = (time>>32)");
        addOption<std::string>("ringFile", "r", "ring file");

        ecalDetIds_.resize(EBDetId::MAX_HASH + 1 + EEDetId::kSizeForDenseIndexing);
        int idx = -1;
        for (int hi = EBDetId::MIN_HASH; hi <= EBDetId::MAX_HASH; ++hi ) {
                EBDetId ebId = EBDetId::unhashIndex(hi);
                if (ebId != EBDetId()) {
                        ecalDetIds_[hi] = ebId;
                }
        }
        for ( int hi = 0; hi < EEDetId::kSizeForDenseIndexing; ++hi ) {
                EEDetId eeId = EEDetId::unhashIndex(hi);
                if (eeId != EEDetId()) {
                        idx = EBDetId::MAX_HASH + 1 + hi;
                        ecalDetIds_[idx] = eeId;
                }
        }
        assert(ecalDetIds_.size() == 75848);
        assert(ecalDetIds_.size() == EBDetId::MAX_HASH + 1 + EEDetId::kSizeForDenseIndexing);
        h_ = new TH1F("h", "h", 250, 0.5, 5.5);
}

cond::APDPNRatioHgg::~APDPNRatioHgg(){
        h_->Delete();
}


void cond::APDPNRatioHgg::read_chstatus()
{
        FILE * fd = fopen("chstatus.dat", "r");
        int iid = 0, n = 0, status = 0;
        while ((n = fscanf(fd, "%*d %*d %*d %d %d", &status, &iid)) != EOF) {
                DetId id(iid);
                int hidx = -1;
                if (id.subdetId() == EcalBarrel) {
                        hidx = EBDetId(id).hashedIndex();
                } else {
                        hidx = EBDetId::MAX_HASH + 1 + EEDetId(id).hashedIndex();
                }
                good_[hidx] = status;
        }
}

void cond::APDPNRatioHgg::dump_ring_average(FILE * fd, time_t tb, time_t te, TFile * fout)
{
        fprintf(fd, "%lu %lu", tb, te);
        char name[256];
        for (size_t i = 0; i < 2 * DRings::nHalfIEta + 1; ++i) {
                fprintf(fd, "   %lu %d %f %f %f %f",
                        i, _nvalues[i],
                        _svalues[i] / _nvalues[i],
                        rms(_rmseta[i]),
                        _rms3x3[i] / _nvalues[i],
                        _rms5x5[i] / _nvalues[i]);
                sprintf(name, "h_%lu_%lu", tb, i);
                TH1F * htmp = (TH1F*)h_->Clone(name);
                for (size_t j = 0; j < _rmseta[i].size(); ++j) {
                        htmp->Fill(_rmseta[i][j] / (_svalues[i] / _nvalues[i]));
                }
                htmp->Write();
                htmp->Delete();
        }
        fprintf(fd,    "\n");
}

void cond::APDPNRatioHgg::dump_eta_rings(FILE * fd)
{
        for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
                DetId id(ecalDetIds_[i]);
                if (id.subdetId() == EcalBarrel) {
                        EBDetId eid(id);
                        fprintf(fd, "%d %d %d %d  %u\n", eid.ieta(), eid.iphi(), 0,  
                                dr_.ieta(id),
                                id.rawId());
                } else if (id.subdetId() == EcalEndcap) {
                        EEDetId eid(id);
                        fprintf(fd, "%d %d %d %d  %u\n", eid.ix(), eid.iy(), eid.zside(),
                                dr_.ieta(id),
                                id.rawId());
                } else {
                        fprintf(stderr, "[dump] invalid DetId: %d\n", id.rawId());
                        exit(-1);
                }
        }
}

void cond::APDPNRatioHgg::dump_txt(FILE * fd)
{
        AMapCit ita, itb;
        float p2 = -1.;
        for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
                DetId id(ecalDetIds_[i]);
                p2 = _svalues[i] / _nvalues[i];
                if (id.subdetId() == EcalBarrel) {
                        EBDetId eid(id);
                        fprintf(fd, "%d %d %d %f  %u\n", eid.ieta(), eid.iphi(), 0,  
                                p2,
                                id.rawId());
                } else if (id.subdetId() == EcalEndcap) {
                        EEDetId eid(id);
                        fprintf(fd, "%d %d %d %f  %u\n", eid.ix(), eid.iy(), eid.zside(),
                                p2,
                                id.rawId());
                } else {
                        fprintf(stderr, "[dump] invalid DetId: %d\n", id.rawId());
                        exit(-1);
                }
        }
}

///void cond::APDPNRatioHgg::neighbours_EB(DetId id, std::vector<DetId> & v, int n)
///{
///        EBDetId eid(id);
///}
///
///void cond::APDPNRatioHgg::neighbours_EE(DetId id, std::vector<DetId> & v, int n)
///{
///        if (id.subdetId() == EcalBarrel) {
///                neighbours_EB(id, v, n);
///        } else {
///                neighbours_EE(id, v, n);
///        }
///}
///
///void cond::APDPNRatioHgg::neighbours(DetId id, std::vector<DetId> & v, int n)
///{
///        if (id.subdetId() == EcalBarrel) {
///                neighbours_EB(id, v, n);
///        } else {
///                neighbours_EE(id, v, n);
///        }
///}

void cond::APDPNRatioHgg::fill_ring_average(const A & obja)
{
        AMapCit it, jt;
        for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
                if (good_[i]) continue;
                DetId id(ecalDetIds_[i]);
                float alpha = 0;
                if (id.subdetId() == EcalBarrel) {
                        alpha = 1.52;
                } else {
                        alpha = 1.16;
                }
                it = obja.getLaserMap().find(id);
                float corr = pow(it->p2, -alpha);
                _svalues[dr_.ieta(id) + DRings::nHalfIEta] += corr;
                _nvalues[dr_.ieta(id) + DRings::nHalfIEta] += 1;
                //
                // computing spread in 3x3 and 5x5
                std::vector<float> v3x3, v5x5;
                for (int j = -2; j <= +2; ++j) {
                        for (int k = -2; k <= +2; ++k) {
                                if (!j && !k) continue;
                                DetId jd;
                                if (id.subdetId() == EcalBarrel) {
                                        jd = EBDetId::offsetBy(id, j, k);
                                } else {
                                        jd = EEDetId::offsetBy(id, j, k);
                                }
                                if (jd == DetId(0)) continue;
                                jt = obja.getLaserMap().find(jd);
                                float corr = pow(jt->p2, -alpha);
                                if (abs(j) <= 1 && abs(k) <= 1) {
                                        v3x3.push_back(corr);
                                }
                                v5x5.push_back(corr);
                        }
                }
                _rmseta[dr_.ieta(id) + DRings::nHalfIEta].push_back(corr);
                _rms3x3[dr_.ieta(id) + DRings::nHalfIEta] += rms(v3x3);
                _rms5x5[dr_.ieta(id) + DRings::nHalfIEta] += rms(v5x5);
        }
}

void cond::APDPNRatioHgg::reset_ring_average()
{
        for (size_t i = 0; i < 2 * DRings::nHalfIEta; ++i) {
                _svalues[i] = 0.;
                _nvalues[i] = 0;
                _rms3x3[i] = 0.;
                _rms5x5[i] = 0.;
                _rmseta[i].clear();
        }
}


int cond::APDPNRatioHgg::execute()
{
        std::string connect = getOptionValue<std::string>("connect" );
        cond::persistency::ConnectionPool connPool;
        if( hasOptionValue("authPath") ){
                connPool.setAuthenticationPath( getOptionValue<std::string>( "authPath") ); 
        }
        connPool.configure();
        cond::persistency::Session session = connPool.createSession( connect );

        std::string tag1 = getOptionValue<std::string>("tag");

        session.transaction().start( true );
        const auto & iov = session.readIov(tag1).selectAll();

        cond::Time_t since = std::numeric_limits<cond::Time_t>::min();
        if(hasOptionValue("beginTime")) since = getOptionValue<cond::Time_t>("beginTime");
        cond::Time_t till = std::numeric_limits<cond::Time_t>::max();
        if(hasOptionValue("endTime")) till = getOptionValue<cond::Time_t>("endTime");

        bool shift = hasOptionValue("shift");

        std::string rf = "eerings.dat";
        if (hasOptionValue("ringFile")) rf = getOptionValue<std::string>("ringFile");
        dr_.setEERings(rf.c_str());

        read_chstatus();

        int prescale = 1;
        if (hasOptionValue("prescale")) prescale = getOptionValue<int>("prescale");
        assert(prescale > 0);

        FILE * fdump = NULL;
        TFile * fout = 0;
        char filename[256];
        if (hasOptionValue("dump")) {
                fdump = fopen(getOptionValue<std::string>("dump").c_str(), "w");
                fout = new TFile((getOptionValue<std::string>("dump") + ".root").c_str(), "recreate");
                assert(fdump);
        } else {
                sprintf(filename, "dump_%s__rings_since_%08ld_till_%08ld_prescale%d.dat", tag1.c_str(), (long int)since>>(shift * 32), (long int)till>>(shift * 32), prescale);
                fdump = fopen(filename, "w");
                sprintf(filename, "dump_%s__rings_since_%08ld_till_%08ld_prescale%d.dat.root", tag1.c_str(), (long int)since>>(shift * 32), (long int)till>>(shift * 32), prescale);
                fout = new TFile(filename, "recreate");
                assert(fdump);
        }

        //FILE * fd = fopen("eta_rings.dat", "w");
        //dump_eta_rings(fd);
        //fclose(fd);
        //exit(-1);

        //bool verbose = hasOptionValue("verbose");

        std::cout << "since: " << since << "   till: " << till << "\n";

        size_t niov = 0;
        if (hasOptionValue("niov")) niov = getOptionValue<int>("niov");

        static const size_t nIOVS = iov.size();

        std::cout << "nIOVS: " << nIOVS << "\n";

        size_t cnt = 0, cnt_iov = 0; //, one_dumped = 0;
        A res;
        time_t tb, te;
        for (const auto & i : iov) {
                ++cnt_iov;
                if (i.since < since || i.till > till) continue;
                if (cnt_iov % prescale != 0) continue;
                ++cnt;
                std::cout << cnt_iov << " " << i.since << " -> " << i.till << " " << cnt << "\n";
                std::shared_ptr<A> pa = session.fetchPayload<A>(i.payloadId);
                tb = (time_t)i.since>>32;
                te = (time_t)i.till>>32;
                reset_ring_average();
                printf("--> %lu %lu (%ld/%ld)\n", tb, te, cnt, nIOVS);
                reset_ring_average();
                fill_ring_average(*pa);
                dump_ring_average(fdump, tb, te, fout);
                if (niov > 0 && cnt >= niov) break;
        }
        session.transaction().commit();
        fclose(fdump);
        return 0;
}


int main( int argc, char** argv )
{
        cond::APDPNRatioHgg valida;
        return valida.run(argc,argv);
}
