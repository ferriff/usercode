#include "CondCore/Utilities/interface/Utilities.h"
#include "CondCore/CondDB/interface/ConnectionPool.h"
#include "CondCore/CondDB/interface/IOVProxy.h"

#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatios.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRcd.h"
#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatiosRef.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRefRcd.h"

#include "Calibration/Tools/interface/DRings.h"

#include <boost/program_options.hpp>
#include <iterator>
#include <iostream>
#include <limits>

using namespace std;

float _values[75848];
int   _nvalues[75848];

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
                        void dump_time_average(FILE * fd, time_t tb, time_t te);
                        void dump_txt(FILE * fd);
                        void fill_time_average(const A & obja);
                        void reset_time_average();
                private:
                        std::vector<DetId> ecalDetIds_;
                        DRings dr_;
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
                        _nvalues[hi] = 0;
                        _values[hi] = 0.;
                }
        }
        for ( int hi = 0; hi < EEDetId::kSizeForDenseIndexing; ++hi ) {
                EEDetId eeId = EEDetId::unhashIndex(hi);
                if (eeId != EEDetId()) {
                        idx = EBDetId::MAX_HASH + 1 + hi;
                        ecalDetIds_[idx] = eeId;
                        _nvalues[idx] = 0;
                        _values[idx] = 0.;
                }
        }
        assert(ecalDetIds_.size() == 75848);
        assert(ecalDetIds_.size() == EBDetId::MAX_HASH + 1 + EEDetId::kSizeForDenseIndexing);
}

cond::LaserValidation::~LaserValidation(){
}


void cond::LaserValidation::dump_time_average(FILE * fd, time_t tb, time_t te)
{
        AT ta;
        AMapCit ita, itb;
        fprintf(fd, "T %ld %ld", tb, te);
        for (size_t i = 0; i < 92; ++i) {
                fprintf(fd, " %ld", tb + (te - tb) / 2);
        }
        fprintf(fd, "\n");
        float p2 = -1.;
        for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
                p2 = _values[i] / _nvalues[i];
                fprintf(fd, "P %d %f %f %f\n", ecalDetIds_[i].rawId(), p2, p2, p2);
        }
}

void cond::LaserValidation::dump_txt(FILE * fd)
{
        AMapCit ita, itb;
        float p2 = -1.;
        for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
                DetId id(ecalDetIds_[i]);
                p2 = _values[i] / _nvalues[i];
                if (id.subdetId() == EcalBarrel) {
                        EBDetId eid(id);
                        fprintf(fd, "%d %d %d %f  %u %d\n", eid.ieta(), eid.iphi(), 0,  
                                p2,
                                id.rawId(),
                                dr_.ieta(id)
                                );
                } else if (id.subdetId() == EcalEndcap) {
                        EEDetId eid(id);
                        fprintf(fd, "%d %d %d %f  %u %d\n", eid.ix(), eid.iy(), eid.zside(),
                                p2,
                                id.rawId(),
                                dr_.ieta(id)
                                );
                } else {
                        fprintf(stderr, "[dump] invalid DetId: %d\n", id.rawId());
                        exit(-1);
                }
        }
}

void cond::LaserValidation::fill_time_average(const A & obja)
{
        AMapCit it;
        for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
                it = obja.getLaserMap().find(ecalDetIds_[i]);
                _values[i]  += it->p2;
                _nvalues[i] += 1;
        }
}

void cond::LaserValidation::reset_time_average()
{
        for (size_t i = 0; i < ecalDetIds_.size(); ++i) {
                _values[i] = 0.;
                _nvalues[i] = 0;
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

        time_t t_interval = std::numeric_limits<time_t>::max();
        if (hasOptionValue("deltaTime")) t_interval = getOptionValue<time_t>("deltaTime");
        bool shift = hasOptionValue("shift");

        FILE * fdump = NULL;
        char filename[256];
        if (hasOptionValue("dump")) {
                fdump = fopen(getOptionValue<std::string>("dump").c_str(), "w");
                assert(fdump);
        } else {
                sprintf(filename, "dump_%s__avg_since_%08ld_till_%08ld.dat", tag1.c_str(), (long int)since>>(shift * 32), (long int)till>>(shift * 32));
                fdump = fopen(filename, "w");
                assert(fdump);
        }

        //bool verbose = hasOptionValue("verbose");
        bool txt = hasOptionValue("txt");

        std::string rf = "eerings.dat";
        if (hasOptionValue("ringFile")) rf = getOptionValue<std::string>("ringFile");
        dr_.setEERings(rf.c_str());

        std::cout << "since: " << since << "   till: " << till << "\n";

        size_t niov = 0;
        if (hasOptionValue("niov")) niov = getOptionValue<int>("niov");
        static const size_t nIOVS = iov.size();

        int prescale = 1;
        if (hasOptionValue("prescale")) prescale = getOptionValue<int>("prescale");
        assert(prescale > 0);

        size_t cnt = 0, cnt_iov = 0, one_dumped = 0;
        A res;
        time_t tb, te, tb_first = 0, te_last = 0;
        for (const auto & i : iov) {
                tb_first = i.since>>32;
                te_last  = i.till>>32;
                break;
        }
        for (const auto & i : iov) {
                ++cnt_iov;
                if (i.since < since || i.till > till) continue;
                if (cnt_iov % prescale != 0) continue;
                ++cnt;
                std::cout << cnt_iov << " " << i.since << " -> " << i.till << " " << cnt << "\n";
                std::shared_ptr<A> pa = session.fetchPayload<A>(i.payloadId);
                tb = (time_t)i.since>>32;
                te = (time_t)i.till>>32;
                printf("--> %lu %lu (%ld/%ld)\n", tb, te, cnt, nIOVS);
                if (tb - tb_first > t_interval) {
                        printf("... writing tag with begin: %lu end: %lu (deltaT: %lu) - t_interval: %lu\n", tb_first, te_last, te_last - tb_first, t_interval);
                        if (!txt) dump_time_average(fdump, tb_first, te_last);
                        else      dump_txt(fdump);
                        reset_time_average();
                        tb_first = tb;
                        one_dumped = 1;
                }
                te_last = te;
                fill_time_average(*pa);
                if (niov > 0 && cnt >= niov) break;
        }
        if (!one_dumped) {
                if (!txt) dump_time_average(fdump, tb_first, te_last);
                else      dump_txt(fdump);
        }
        session.transaction().commit();
        fclose(fdump);
        return 0;
}


int main( int argc, char** argv )
{
        cond::LaserValidation valida;
        return valida.run(argc,argv);
}
