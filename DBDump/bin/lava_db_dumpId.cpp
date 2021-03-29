#include "CondCore/Utilities/interface/Utilities.h"
#include "CondCore/CondDB/interface/ConnectionPool.h"
#include "CondCore/CondDB/interface/IOVProxy.h"

#include "CondFormats/Common/interface/TimeConversions.h"

#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatios.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRcd.h"
#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatiosRef.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRefRcd.h"

#include <signal.h>

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
                        void save() { printf("saving file %p\n", (void*)fd); if (fd) fclose(fd); }
                private:
                        FILE * fd;
        };

}

cond::LaserValidation::LaserValidation():Utilities("cmscond_list_iov")
{
        addAuthenticationOptions();
        addOption<bool>("verbose","v","verbose");
        addOption<bool>("all","a","list all tags (default mode)");
        addOption<cond::Time_t>("beginTime","b","begin time (first since) (optional)");
        addOption<cond::Time_t>("endTime","e","end time (last till) (optional)");
        addOption<std::string>("tag","t","list info of the specified tag");
        addOption<std::string>("geom","g","geometry file (default: detid_geom.dat)");
        addOption<std::string>("output","o","output file (default: ecal_laser_dumped_ids.dat)");
        addOption<std::string>("id","i","DetId (raw Id) to be dumped, quoted and blank separated list of raw DetIds, or one of the following keywords: all, EB, EB-, EB+, EE, EE-, EE+");
        addOption<int>("niov","n","number of IOV");
        addOption<int>("prescale","s","prescale factor");
        addOption<std::string>("connect","c","connect to a specific db");
}

cond::LaserValidation::~LaserValidation()
{
        save();
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
        std::string output = hasOptionValue("output") ? getOptionValue<std::string>("output") : "ecal_laser_dumped_ids.dat";

        if      (output == "stdout") fd = stdout;
        else if (output == "stderr") fd = stderr;
        else {
                fd = fopen(output.c_str(), "w");
        }
        assert(fd != NULL);

        fprintf(fd, "# %s @ %s\n", tag.c_str(), getConnectValue().c_str());

        cond::Time_t since = std::numeric_limits<cond::Time_t>::min();
        if( hasOptionValue("beginTime" )) since = getOptionValue<cond::Time_t>("beginTime");
        cond::Time_t till = std::numeric_limits<cond::Time_t>::max();
        if( hasOptionValue("endTime" )) till = getOptionValue<cond::Time_t>("endTime");

        session.transaction().start( true );
        const auto & iov = session.readIov(tag).selectAll();

        std::cout << "tag " << tag << " , total of " << std::distance(iov.begin(), iov.end()) << "iov(s)\n";
        std::cout << "since: " << since << "   till: " << till << "\n";

        int niov = -1;
        if (hasOptionValue("niov")) niov = getOptionValue<int>("niov");

        std::vector<int> id;
        if (!hasOptionValue("id")) {
                fprintf(stderr, "Fatal error: <id> option is mandatory, abort.\n");
                assert(0);
        } else {
                std::string ids = getOptionValue<std::string>("id");
                if (ids == "all" || ids.find("EB") != std::string::npos || ids.find("EE") != std::string::npos) {
                        for (int hi = EBDetId::MIN_HASH; hi <= EBDetId::MAX_HASH; ++hi ) {
                                EBDetId ebId = EBDetId::unhashIndex(hi);
                                int ieta = ebId.ieta();
                                if (ebId != EBDetId()) {
                                        if (ieta > 0 && (ids == "all" || ids == "EB" || ids == "EB+")) id.push_back(ebId);
                                        if (ieta < 0 && (ids == "all" || ids == "EB" || ids == "EB-")) id.push_back(ebId);
                                }
                        }
                        for ( int hi = 0; hi < EEDetId::kSizeForDenseIndexing; ++hi ) {
                                EEDetId eeId = EEDetId::unhashIndex(hi);
                                int zside = eeId.zside();
                                if (eeId != EEDetId()) {
                                        if (zside > 0 && (ids == "all" || ids == "EE" || ids == "EE+")) id.push_back(eeId);
                                        if (zside < 0 && (ids == "all" || ids == "EE" || ids == "EE-")) id.push_back(eeId);
                                }
                        }
                } else {
                        const char * beg = ids.c_str();
                        const char * lim = beg + ids.size();
                        char * end;
                        int num = 1;
                        while (beg < lim && (num = strtol(beg, &end, 0)) != 0) {
                                id.push_back(num);
                                beg = end;
                        }
                }
                printf("Going to dump data for the following DetId('s):");
                for (size_t i = 0; i < id.size(); ++i) {
                        printf(" %d", id[i]);
                }
                printf("\n");
        }

        int prescale = 1;
        if (hasOptionValue("prescale")) prescale = getOptionValue<int>("prescale");
        assert(prescale > 0);

        typedef unsigned int LuminosityBlockNumber_t;
        typedef unsigned int RunNumber_t;

        fprintf(fd, "#time     ");
        for (size_t i = 0; i < id.size(); ++i) {
                fprintf(fd, " %d", id[i]);
        }
        fprintf(fd, "\n");

        int cnt = 0, cnt_iov = 0;
        for (const auto & i : iov) {
                ++cnt_iov;
                if (i.since < since || i.till > till) continue;
                if (cnt_iov % prescale != 0) continue;
                ++cnt;
                std::cout << cnt_iov << " " << i.since << " -> " << i.till << " " << cnt << "\n";
                std::shared_ptr<A> pa = session.fetchPayload<A>(i.payloadId);
                EcalLaserAPDPNRatios::EcalLaserAPDPNRatiosMap::const_iterator itAPDPN;
                fprintf(fd, "%ld", (time_t)i.since>>32);
                for (size_t i = 0; i < id.size(); ++i) {
                        itAPDPN = (*pa).getLaserMap().find(id[i]);
                        assert(itAPDPN != (*pa).getLaserMap().end());
                        fprintf(fd, " %f", (*itAPDPN).p2);
                }
                fprintf(fd, "\n");
                if (niov > 0 && cnt >= niov) break;
        }
        session.transaction().commit();
        return 0;
}

cond::LaserValidation * valida;

void save_file(int)
{
        valida->save();
        exit(1);
}

int main( int argc, char** argv )
{
        signal(SIGINT, save_file);
        valida = new cond::LaserValidation();
        valida->run(argc,argv);
        return 0;
}
