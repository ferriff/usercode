#include "CondCore/Utilities/interface/Utilities.h"
#include "CondCore/CondDB/interface/ConnectionPool.h"
#include "CondCore/CondDB/interface/IOVProxy.h"

#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatios.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRcd.h"
#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatiosRef.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRefRcd.h"

#include "CalibCalorimetry/EcalLaserAnalyzer/interface/MEEBGeom.h"
#include "CalibCalorimetry/EcalLaserAnalyzer/interface/MEEEGeom.h"

#include "../interface/EcalLaserDumper.h"

#include <boost/program_options.hpp>
#include <iterator>
#include <iostream>

using namespace std;


namespace cond {
        class LaserValidation : public Utilities {
                public:
                        typedef EcalLaserAPDPNRatios A;
                        typedef EcalLaserAPDPNRatios::EcalLaserAPDPNRatiosMap AMap;
                        typedef EcalLaserAPDPNRatios::EcalLaserAPDPNRatiosMap::const_iterator AMapCit;
                        typedef EcalLaserAPDPNRatios::EcalLaserAPDPNpair AP;
                        typedef EcalLaserAPDPNRatios::EcalLaserTimeStamp AT;

                        LaserValidation();
                        ~LaserValidation();
                        int execute();
                        void dump_txt(FILE * fd, const A & obja, time_t tb, time_t te);
                private:
                        float verify(const A & rcd1);
                        void merge(const A & rcd1, const A & rcd2, const A & rcd3, A & res);
                        void mergePoints(const AP & pa, const AT & ta, const AP & pb_1, const AT & tb_1, const AP & pb_2, const AT & tb_2, AP & res);
                        void mergeTimes(const AT & ta, const AT & tb, AT & res);
                        float interpolate(float a, int ta, float b_1, int tb_1, float b_2, int tb_2);
                        std::vector<DetId> ecalDetIds_;
        };
}

cond::LaserValidation::LaserValidation():Utilities("cmscond_list_iov")
{
        addAuthenticationOptions();
        addOption<bool>("verbose","v","verbose");
        addOption<bool>("all","a","list all tags (default mode)");
        addOption<std::string>("connect","c","connect to a specific db");
        addOption<cond::Time_t>("beginTime","b","begin time (first since) (optional)");
        addOption<cond::Time_t>("endTime","e","end time (last till) (optional)");
        addOption<std::string>("tag","t","first tag to be compared");
        addOption<std::string>("Tag","T","second tag to be compared");
        addOption<std::string>("geom","g","geometry file (default: detid_geom.dat)");
        addOption<std::string>("output","o","output directory (default: ecal_laser_dumper)");
        addOption<int>("niov","n","number of IOV");
        addOption<int>("prescale","s","prescale factor");
        addOption<std::string>("dump", "d", "file for a txt dump");

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
}

cond::LaserValidation::~LaserValidation(){
}


void cond::LaserValidation::mergePoints(const AP & pa, const AT & ta, const AP & pb_1, const AT & tb_1, const AP & pb_2, const AT & tb_2, AP & res)
{
        res.p1 = interpolate(pa.p1, ta.t1.unixTime(), pb_1.p1, tb_1.t1.unixTime(), pb_2.p1, tb_2.t1.unixTime());
        res.p2 = interpolate(pa.p2, ta.t2.unixTime(), pb_1.p2, tb_1.t2.unixTime(), pb_2.p2, tb_2.t2.unixTime());
        res.p3 = interpolate(pa.p3, ta.t3.unixTime(), pb_1.p3, tb_1.t3.unixTime(), pb_2.p3, tb_2.t3.unixTime());
        //fprintf(stderr, " %f %f\n", pa.p2, interpolate(pa.p2, ta.t2.unixTime(), pb_1.p2, tb_1.t2.unixTime(), pb_2.p2, tb_2.t2.unixTime()));
}

float cond::LaserValidation::interpolate(float a, int ta, float b_1, int tb_1, float b_2, int tb_2)
{
        //return b_1 + (b_2 - b_1) / (tb_2 - tb_1) * (ta - tb_1);
        float res =  b_1 + (b_2 - b_1) / (tb_2 - tb_1) * (ta - tb_1);
        //fprintf(stderr, "%f %d   %f %d   %f %d  %f\n", a, ta, b_1, tb_1, b_2, tb_2, res);
        return res;
}

void cond::LaserValidation::mergeTimes(const AT & ta, const AT & tb, AT & res)
{
        // FIXME: add conditions to check pa.t_i with pb.t_i
        res.t1 = ta.t1;
        res.t2 = ta.t2;
        res.t3 = ta.t3;
}

void cond::LaserValidation::merge(const A & obja, const A & objb_1, const A & objb_2, A & res)
{
        AP p;
        AT ta, tb_1, tb_2, ts;
        AMapCit ita, itb_1, itb_2;
        for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
                DetId id(ecalDetIds_[i]);
                ita = obja.getLaserMap().find(id);
                itb_1 = objb_1.getLaserMap().find(id);
                itb_2 = objb_2.getLaserMap().find(id);

                int iLM = 0;
                if (id.subdetId()==EcalBarrel) {
                        EBDetId ebid(id);
                        iLM = MEEBGeom::lmr(ebid.ieta(), ebid.iphi()) - 1;
                } else {
                        EEDetId eeid(id);
                        // SuperCrystal coordinates
                        MEEEGeom::SuperCrysCoord iX = (eeid.ix()-1)/5 + 1;
                        MEEEGeom::SuperCrysCoord iY = (eeid.iy()-1)/5 + 1;
                        iLM = MEEEGeom::lmr(iX, iY, eeid.zside()) - 1;
                }
                ta = obja.getTimeMap()[iLM];  
                tb_1 = objb_1.getTimeMap()[iLM];  
                tb_2 = objb_2.getTimeMap()[iLM];  

                mergePoints(*ita, ta, *itb_1, tb_1, *itb_2, tb_2, p);
                res.setValue(id, p);
        }
        for (size_t i = 0; i < 92; ++i) {
                ta = obja.getTimeMap()[i];
                tb_1 = objb_1.getTimeMap()[i];
                mergeTimes(ta, tb_1, ts);
                res.setTime(i, ts);
        }
}


float cond::LaserValidation::verify(const A & obja)
{
        int ng = 0;
        float low = 0, high = 0;
        AMapCit ita;
        for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
                DetId id(ecalDetIds_[i]);
                if (id.subdetId() == EcalBarrel) {
                        low = .9925;
                        high = 1.0075;
                } else {
                        low = 1.000;
                        high = 1.015;
                }
                ita = obja.getLaserMap().find(id);
                if ((*ita).p2 >= low && (*ita).p2 <= high) ++ng;
        }
        return (float)ng / (float)ecalDetIds_.size();
}


void cond::LaserValidation::dump_txt(FILE * fd, const A & obja, time_t tb, time_t te)
{
        AT ta;
        AMapCit ita, itb;
        fprintf(fd, "T %ld %ld", tb, te);
        for (size_t i = 0; i < 92; ++i) {
                ta = obja.getTimeMap()[i];
                fprintf(fd, " %d", ta.t2.unixTime());
        }
        fprintf(fd, "\n");
        for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
                ita = obja.getLaserMap().find(ecalDetIds_[i]);
                fprintf(fd, "P %d %f %f %f\n", ecalDetIds_[i].rawId(), ita->p1, ita->p2, ita->p3);
        }
}


int cond::LaserValidation::execute()
{
        std::string connect = getOptionValue<std::string>("connect" );
        cond::persistency::ConnectionPool connPool;
        if( hasOptionValue("authPath") ){
                connPool.setAuthenticationPath( getOptionValue<std::string>( "authPath") ); 
        }
        connPool.configure();
        cond::persistency::Session session = connPool.createSession( connect );

        std::string tag1 = getOptionValue<std::string>("tag");
        std::string tag2 = getOptionValue<std::string>("Tag");

        std::string geom = hasOptionValue("geom") ? getOptionValue<std::string>("geom") : "detid_geom.dat";
        std::string odir = hasOptionValue("output") ? getOptionValue<std::string>("output") : "ecal_laser_dumper";


        cond::Time_t since = std::numeric_limits<cond::Time_t>::min();
        if( hasOptionValue("beginTime" )) since = getOptionValue<cond::Time_t>("beginTime");
        cond::Time_t till = std::numeric_limits<cond::Time_t>::max();
        if( hasOptionValue("endTime" )) till = getOptionValue<cond::Time_t>("endTime");

        session.transaction().start( true );
        const auto & iov1 = session.readIov(tag1).selectAll();
        auto iov2_p = session.readIov(tag2);
        const auto & iov2 = iov2_p.selectAll();

        FILE * fdump = NULL;
        if (hasOptionValue("dump")) {
                fdump = fopen(getOptionValue<std::string>("dump").c_str(), "w");
                assert(fdump);
        }

        //bool verbose = hasOptionValue("verbose");

        std::cout << "since: " << since << "   till: " << till << "\n";

        size_t niov = 0;
        if (hasOptionValue("niov")) niov = getOptionValue<int>("niov");

        int prescale = 1;
        if (hasOptionValue("prescale")) prescale = getOptionValue<int>("prescale");
        assert(prescale > 0);

        static const size_t nIOVS = iov1.size();

        std::cout << "nIOVS: " << nIOVS << "\n";

        typedef unsigned int LuminosityBlockNumber_t;
        typedef unsigned int RunNumber_t;

        size_t cnt = 0, cnt_iov = 0;
        EcalLaserDumper ld(odir);
        A res;
        for (const auto & i : iov1) {
                ++cnt_iov;
                if (i.since < since || i.till > till) continue;
                if (cnt_iov % prescale != 0) continue;
                ++cnt;
                std::cout << cnt_iov << " " << i.since << " -> " << i.till << " " << cnt << "\n";
                auto j = iov2_p.getInterval(i.since);
                auto ik = ++(iov2.find(i.since));
                if (ik == iov2.end()) {
                        fprintf(stderr, "Error: reached the end of tag -T, continuing\n");
                        continue;
                }
                auto k = *ik;
                std::shared_ptr<A> pa = session.fetchPayload<A>(i.payloadId);
                std::shared_ptr<A> pb_1 = session.fetchPayload<A>(j.payloadId);
                std::shared_ptr<A> pb_2 = session.fetchPayload<A>(k.payloadId);

                if (j.since > i.since || k.since < i.since) fprintf(stderr, "Error: two consecutive measurements of tag -T before one of tag -t could be found\n");

                merge(*pa, *pb_1, *pb_2, res);
                //if (verify(res) < 0.5) continue;
                ld.dump((time_t)i.since>>32, &(*pa), &res);
                if (niov > 0 && cnt >= niov) break;
                if (fdump) dump_txt(fdump, res, (time_t)i.since>>32, (time_t)i.till>>32);
        }
        session.transaction().commit();
        return 0;
}


int main( int argc, char** argv )
{
        cond::LaserValidation valida;
        return valida.run(argc,argv);
}
