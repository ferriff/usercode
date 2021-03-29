#include "CondCore/Utilities/interface/Utilities.h"
#include "CondCore/CondDB/interface/ConnectionPool.h"
#include "CondCore/CondDB/interface/IOVProxy.h"

#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatios.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRcd.h"
#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatiosRef.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRefRcd.h"

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
                        void dump_txt(char * filename, const A & obja, time_t tb, time_t te, bool flat);
                private:
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
        addOption<int>("niov","n","number of IOV");
        addOption<int>("prescale","s","prescale factor");
        addOption<std::string>("dump", "d", "file for a txt dump");
        addOption<bool>("flat", "f", "flat iov, i.e. p1 = p2 = p3");

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


void cond::LaserValidation::dump_txt(char * filename, const A & obja, time_t tb, time_t te, bool flat)
{
        FILE * fd = fopen(filename, "w");
        if (!fd) {
                char err[256];
                sprintf(err, "[cond::LaserValidation::dump] Impossible to open file `%s' for dumping:", filename);
                perror(err);
                exit(1);
        }
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
                if (!flat) fprintf(fd, "P %d %f %f %f\n", ecalDetIds_[i].rawId(), ita->p1, ita->p2, ita->p3);
                else       fprintf(fd, "P %d %f %f %f\n", ecalDetIds_[i].rawId(), ita->p2, ita->p2, ita->p2);
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

        session.transaction().start( true );
        const auto & iov = session.readIov(tag1).selectAll();

        cond::Time_t since = std::numeric_limits<cond::Time_t>::min();
        if(hasOptionValue("beginTime")) since = getOptionValue<cond::Time_t>("beginTime");
        cond::Time_t till = std::numeric_limits<cond::Time_t>::max();
        if(hasOptionValue("endTime")) till = getOptionValue<cond::Time_t>("endTime");

        FILE * fdump = NULL;
        if (hasOptionValue("dump")) {
                fdump = fopen(getOptionValue<std::string>("dump").c_str(), "w");
                assert(fdump);
        }

        //bool verbose = hasOptionValue("verbose");
        bool flat = hasOptionValue("flat");

        std::cout << "since: " << since << "   till: " << till << "\n";

        int niov = -1;
        if (hasOptionValue("niov")) niov = getOptionValue<int>("niov");

        int prescale = 1;
        if (hasOptionValue("prescale")) prescale = getOptionValue<int>("prescale");
        assert(prescale > 0);

        int cnt = 0, cnt_iov = 0;
        char filename[256];
        A res;

        for (const auto & i : iov) {
                ++cnt_iov;
                if (i.since < since || i.till > till) continue;
                if (cnt_iov % prescale != 0) continue;
                ++cnt;
                std::cout << cnt_iov << " " << i.since << " -> " << i.till << " " << cnt << "\n";
                std::shared_ptr<A> pa = session.fetchPayload<A>(i.payloadId);
                sprintf(filename, "dump_%s__since_%08ld_till_%08ld.dat", tag1.c_str(), (long int)i.since, (long int)i.till);
                fprintf(stderr, "going to dump on `%s'\n", filename);
                dump_txt(filename, *pa, (time_t)i.since>>32, (time_t)i.till>>32, flat);
                if (niov > 0 && cnt >= niov) break;
        }
        session.transaction().commit();
        return 0;
}


int main( int argc, char** argv )
{
        cond::LaserValidation valida;
        return valida.run(argc,argv);
}
