#include "CondCore/Utilities/interface/Utilities.h"
#include "CondCore/CondDB/interface/ConnectionPool.h"
#include "CondCore/CondDB/interface/IOVProxy.h"

#include "CondFormats/Common/interface/TimeConversions.h"
#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatios.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRcd.h"
#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatiosRef.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRefRcd.h"

#include "usercode/DBDump/interface/EcalLaserPlotter.h"

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
        addOption<std::string>("tag","t","list info of the specified tag");
        addOption<std::string>("geom","g","geometry file (default: detid_geom.dat)");
        addOption<std::string>("output","o","output file (default: ecallaserplotter.root)");
        addOption<int>("niov","n","number of IOV");
        addOption<int>("prescale","s","prescale factor");
        addOption<bool>("timebased","T","the IOVs are time-based (default is run-based)");
}

cond::LaserValidation::~LaserValidation(){
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
        std::string output = hasOptionValue("output") ? getOptionValue<std::string>("output") : "ecallaserplotter.root";
        //bool timebased = hasOptionValue("timebased");

        cond::Time_t since = std::numeric_limits<cond::Time_t>::min();
        if( hasOptionValue("beginTime" )) since = getOptionValue<cond::Time_t>("beginTime");
        cond::Time_t till = std::numeric_limits<cond::Time_t>::max();
        if( hasOptionValue("endTime" )) till = getOptionValue<cond::Time_t>("endTime");

        //bool verbose = hasOptionValue("verbose");

        session.transaction().start( true );
        const auto & iov = session.readIov(tag).selectAll();

        //since = std::max((cond::Time_t)2, cond::timeTypeSpecs[iov.timetype()].beginValue); // avoid first IOV
        //till  = std::min(till,  cond::timeTypeSpecs[iov.timetype()].endValue);

        std::cout << "tag " << tag << " , total of " << std::distance(iov.begin(), iov.end()) << "iov(s)\n";
        std::cout << "since: " << since << "   till: " << till << "\n";

        int niov = -1;
        if (hasOptionValue("niov")) niov = getOptionValue<int>("niov");

        int prescale = 1;
        if (hasOptionValue("prescale")) prescale = getOptionValue<int>("prescale");
        assert(prescale > 0);

        typedef unsigned int LuminosityBlockNumber_t;
        typedef unsigned int RunNumber_t;

        int cnt = 0, cnt_iov = 0;
        EcalLaserPlotter lp(geom.c_str());
        for (const auto & i : iov) {
                ++cnt_iov;
                if (i.since < since || i.till > till) continue;
                if (cnt_iov % prescale != 0) continue;
                ++cnt;
                std::cout << cnt_iov << " " << i.since << " -> " << i.till << " " << cnt << "\n";
                std::shared_ptr<A> pa = session.fetchPayload<A>(i.payloadId);
                lp.fill(*pa, (time_t)i.since>>32);
                if (niov > 0 && cnt >= niov) break;
        }
        lp.save(output.c_str());
        session.transaction().commit();
        return 0;
}


int main( int argc, char** argv )
{
        cond::LaserValidation valida;
        return valida.run(argc,argv);
}
