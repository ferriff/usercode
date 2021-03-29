#include "CondCore/Utilities/interface/Utilities.h"
#include "CondCore/CondDB/interface/ConnectionPool.h"
#include "CondCore/CondDB/interface/IOVProxy.h"

#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatios.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRcd.h"
#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatiosRef.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRefRcd.h"

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

                        typedef struct Coord {
                                int ix_;
                                int iy_;
                                int iz_;
                        } Coord;

                        void coord(DetId id, Coord * c);
                        int execute();
                        void dump_txt(FILE * fd, const A & obja, time_t tb, time_t te);
                        void dump_etaphi(FILE * fd, const A & obja, time_t tb, time_t te);

                private:
                        float verify(const A & rcd1);
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


void cond::LaserValidation::coord(DetId id, Coord * c)
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
                fprintf(stderr, "[coord] ERROR: invalid DetId %d", id.rawId());
                assert(0);
        }
}



void cond::LaserValidation::dump_etaphi(FILE * fd, const A & obja, time_t tb, time_t te)
{
        AT ta;
        AMapCit ita, itb;
        fprintf(fd, "# T %ld %ld", tb, te);
        for (size_t i = 0; i < 92; ++i) {
                ta = obja.getTimeMap()[i];
                fprintf(fd, " %d", ta.t2.unixTime());
        }
        fprintf(fd, "\n");
        for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
                Coord c;
                coord(ecalDetIds_[i], &c);
                ita = obja.getLaserMap().find(ecalDetIds_[i]);
                fprintf(fd, "P %d %f %f %f  %d %d %d\n", ecalDetIds_[i].rawId(), ita->p1, ita->p2, ita->p3, c.ix_, c.iy_, c.iz_);
        }
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

        std::string tag = getOptionValue<std::string>("tag");

        std::string geom = hasOptionValue("geom") ? getOptionValue<std::string>("geom") : "detid_geom.dat";
        std::string odir = hasOptionValue("output") ? getOptionValue<std::string>("output") : "ecal_laser_dumper";

        cond::Time_t since = std::numeric_limits<cond::Time_t>::min();
        if( hasOptionValue("beginTime" )) since = getOptionValue<cond::Time_t>("beginTime");
        cond::Time_t till = std::numeric_limits<cond::Time_t>::max();
        if( hasOptionValue("endTime" )) till = getOptionValue<cond::Time_t>("endTime");

        FILE * fdump = NULL;
        //if (hasOptionValue("dump")) {
        fdump = fopen(getOptionValue<std::string>("dump").c_str(), "w");
        assert(fdump);
        //}

        //bool verbose = hasOptionValue("verbose");

        session.transaction().start( true );
        const auto & iov = session.readIov(tag).selectAll();

        std::cout << "since: " << since << "   till: " << till << "\n";

        size_t niov = 0;
        if (hasOptionValue("niov")) niov = getOptionValue<int>("niov");

        int prescale = 1;
        if (hasOptionValue("prescale")) prescale = getOptionValue<int>("prescale");
        assert(prescale > 0);

        static const size_t nIOVS = iov.size();

        std::cout << "nIOVS: " << nIOVS << "\n";

        typedef unsigned int LuminosityBlockNumber_t;
        typedef unsigned int RunNumber_t;

        size_t cnt = 0;
        for (const auto & i : iov) {
                //if (cnt == 0 || cnt < 2) continue;
                ++cnt;
                if (cnt % prescale != 1) continue;
                if (i.since < since || i.till > till) continue;
                if (niov > 0 && cnt > niov) break;
                std::cout << cnt << " " << i.since << " -> " << i.till << "\n";
                std::shared_ptr<A> pa = session.fetchPayload<A>(i.payloadId);
                //if (fdump) dump_txt(fdump, *pa, (time_t)i.since>>32, (time_t)i.till>>32);
                if (fdump) dump_etaphi(fdump, *pa, (time_t)i.since>>32, (time_t)i.till>>32);
        }
        session.transaction().commit();
        return 0;
}


int main( int argc, char** argv )
{
        cond::LaserValidation valida;
        return valida.run(argc,argv);
}
