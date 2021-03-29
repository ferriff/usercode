#include "CondCore/CondDB/interface/ConnectionPool.h"
#include "CondCore/CondDB/interface/IOVProxy.h"
#include "CondCore/Utilities/interface/Utilities.h"
#include "CondFormats/Common/interface/TimeConversions.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRcd.h"
#include "CondFormats/DataRecord/interface/EcalLaserAPDPNRatiosRefRcd.h"
#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatios.h"
#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatiosRef.h"
#include "DataFormats/DetId/interface/DetId.h"

#include "usercode/DBDump/interface/EcalLaserPlotter.h"
#include "usercode/DBDump/interface/EcalLaserDumper.h"

#include <TFile.h>
#include <TTree.h>

#include <stdio.h>
#include <time.h>

#include <boost/program_options.hpp>

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string>
#include <vector>

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
                        void initRoot();
                        int initRootToAppend();
                        void closeRoot();
                        void dbToRoot(const EcalLaserAPDPNRatios & corr);

                private: 
                        TTree *tree;
                        TFile *tfile;
                        unsigned int time[93];
                        float nrv[75848];
                        int ix[75848];
                        int iy[75848];
                        int iz[75848];
                        int detId[75848];
                        static std::string timeToString(time_t t);
                        std::vector<DetId> ecalDetIds_;
                        bool _verbose;
        };

}


std::string cond::LaserValidation::timeToString(time_t t)
{
        char buf[256];
        struct tm lt;
        localtime_r(&t, &lt);
        strftime(buf, sizeof(buf), "%F %R:%S", &lt);
        buf[sizeof(buf)-1] = 0;
        return string(buf);
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
        addOption<std::string>("output","o","output file (default: ecallaserplotter.root)");
        addOption<int>("niov","n","number of IOV to write");
        addOption<int>("skipiov","S","number of IOV to skip");
        addOption<int>("appendiov","N","number of IOV to append");
        addOption<int>("updateTree","U","update existing tree");
        addOption<std::string>("connect","c","connect to a specific db");
        addOption<int>("prescale","s","prescale factor");

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


cond::LaserValidation::~LaserValidation()
{
}


void cond::LaserValidation::initRoot()
{
        std::stringstream title;
        std::stringstream fname;
        title << "Dump of Laser data from file";
        fname << "/tmp/DumpLaserDB";
        fname << ".";
        fname << "r";
        fname << "o";
        fname << "o";
        fname << "t";
        std::cout << "Building tree " << title.str() << " on file " << fname.str()
                << std::endl;

        tfile = new TFile(fname.str().c_str(), "RECREATE", title.str().c_str());
        tree = new TTree("ntu", title.str().c_str());
        tree->Branch("time",          time,          "time[93]/i");
        tree->Branch("detId",         detId,         "detId[75848]/I");
        tree->Branch("ix",            ix,            "ix[75848]/I");
        tree->Branch("iy",            iy,            "iy[75848]/I");
        tree->Branch("iz",            iz,            "iz[75848]/I");
        TBranch * b = tree->Branch("nrv",           nrv,           "nrv[75848]/F");
        b->SetTitle("Normalized response to laser light");
        std::cout << "Tree created" << std::endl;
}


int cond::LaserValidation::initRootToAppend()
{
        int result=0;
        std::stringstream title;
        std::stringstream fname;
        title << "Dump of Laser data from file";
        fname << "DumpLaserDB";
        fname << ".root";
        std::cout << "Building tree " << title.str() << " on file " << fname.str()
                << std::endl;

        tfile = new TFile(fname.str().c_str(), "UPDATE", title.str().c_str());
        // tree = new TTree("LDB", title.str().c_str());

        tree =(TTree*)gDirectory->Get("LDB");

        TBranch        *b_time;   //!
        TBranch        *b_detId;   //!
        TBranch        *b_ix;   //!
        TBranch        *b_iy;   //!
        TBranch        *b_iz;   //!
        TBranch        *b_nrv;   //!

        tree->SetBranchAddress("time", time, &b_time);
        tree->SetBranchAddress("detId", detId, &b_detId);
        tree->SetBranchAddress("ix", ix, &b_ix);
        tree->SetBranchAddress("iy", iy, &b_iy);
        tree->SetBranchAddress("iz", iz, &b_iz);
        tree->SetBranchAddress("nrv", nrv, &b_nrv);

        std::cout << "Tree re-loaded" << std::endl;

        result = (int) tree->GetEntriesFast();
        std::cout << ">> number of entries in tree: " << result<< std::endl;
        return result; 
}


void cond::LaserValidation::closeRoot()
{
        tree->Write();
        tfile->Close();
}


int cond::LaserValidation::execute()
{
        _verbose = hasOptionValue("verbose");

        // connects to the DB 
        std::string connect = getOptionValue<std::string>("connect" );
        cond::persistency::ConnectionPool connPool;
        if( hasOptionValue("authPath") ){
                connPool.setAuthenticationPath( getOptionValue<std::string>( "authPath") );
        }
        connPool.configure();
        cond::persistency::Session condDbSession = connPool.createSession( connect );
        condDbSession.transaction().start( true ); 

        // loads the tag with its IOV list
        std::string tag = getOptionValue<std::string>("tag");
        auto iov_p = condDbSession.readIov(tag);
        const auto & iov = condDbSession.readIov(tag).selectAll();

        // loads the geometry file 
        std::string geom = hasOptionValue("geom") ? getOptionValue<std::string>("geom") : "detid_geom.dat";
        std::string output = hasOptionValue("output") ? getOptionValue<std::string>("output") : "ecallaserplotter.root";


        size_t nIOVTree = 0;
        bool updateTree = false;
        int upd = 0;
        if (hasOptionValue("updateTree")) upd = getOptionValue<int>("updateTree");
        if(upd == 1) updateTree = true;
        /* start the root Tree */
        if(!updateTree) {
                initRoot();
        } else {
                nIOVTree = initRootToAppend();
                if(_verbose) std::cout << "the tree already contains "<<nIOVTree<<" entries"<<std::endl; 
        }

        cond::Time_t since = std::numeric_limits<cond::Time_t>::min();
        if( hasOptionValue("beginTime" )) since = getOptionValue<cond::Time_t>("beginTime");
        cond::Time_t till = std::numeric_limits<cond::Time_t>::max();
        if( hasOptionValue("endTime" )) till = getOptionValue<cond::Time_t>("endTime");

        since = std::max((cond::Time_t)2, since ); 
        till  = std::min(till,  iov_p.getLast().since);

        if(_verbose) std::cout << "tag " << tag << std::endl ;
        if(_verbose) std::cout << "since/till  " << since << "/"<< till<< std::endl ;

        size_t niov = 0;
        if (hasOptionValue("niov")) niov = getOptionValue<int>("niov");

        size_t skipiov = 0;
        if (hasOptionValue("skipiov")) skipiov = getOptionValue<int>("skipiov");

        int NiovAppend = -1;
        if (hasOptionValue("appendiov")) NiovAppend = getOptionValue<int>("appendiov");

        int cntAppend=0;

        int prescale = 1;
        if (hasOptionValue("prescale")) prescale = getOptionValue<int>("prescale");
        assert(prescale > 0);

        // iterate over the IOVs 
        if(_verbose) std::cout << "iterate over the IOVs "  << std::endl ;

        //for (cond::persistency::IOVProxy::Iterator ita = iov.begin(); ita != iov.end() ; ++ita ) {
        size_t cnt = 0, cnt_iov = 0;
        for (const auto & i : iov) {
                ++cnt_iov;
                if (i.since < since || i.till > till) continue;
                if (cnt_iov % prescale != 0) continue;
                ++cnt;
                std::cout << cnt_iov << " " << i.since << " -> " << i.till << " " << cnt << "\n";

                if((updateTree && ( (niov>0 && cnt_iov <nIOVTree ) || (NiovAppend>0 && cnt_iov<= nIOVTree) ))||(skipiov>0 && cnt_iov<=skipiov)) {
                        std::cout <<"IOV not written in rootuple (already present or to be skipped )"<< "\n";
                } else {
                        std::shared_ptr<EcalLaserAPDPNRatios> my_laser 
                                = condDbSession.fetchPayload<EcalLaserAPDPNRatios>( i.payloadId );
                        time[0] = (unsigned int)(i.since>>32);
                        dbToRoot(*my_laser);
                        ++cntAppend;
                }
                if (niov > 0 && cnt >= niov) break;
                if (NiovAppend > 0 && cntAppend >= NiovAppend) break;
        }

        if(_verbose) std::cout << "number of IOVs appended ="<<cntAppend<<std::endl; 
        if(_verbose) std::cout << "number of IOVs read ="<<cnt<<std::endl; 

        condDbSession.transaction().commit();
        closeRoot();
        return 0;
}


void cond::LaserValidation::dbToRoot(const EcalLaserAPDPNRatios & corr)
{
        const EcalLaserAPDPNRatios::EcalLaserAPDPNRatiosMap& p = corr.getLaserMap();
        const EcalLaserAPDPNRatios::EcalLaserTimeStampMap& t = corr.getTimeMap();

        if(t.size() != 92) throw cms::Exception("LasCor") << "Unexpected number time parameter triplets\n";

        unsigned t1 = t[0].t1.unixTime();
        unsigned t3 = t[0].t3.unixTime();
        if(_verbose) cout << "Processing IOV " << t1 << " - " << t3
                << "(" << timeToString(t1) << " - " << timeToString(t3) << "\n";

        for(unsigned i = 0; i < t.size(); ++i) {
                time[i + 1]=t[i + 1].t2.unixTime();
        }

        int icrys=0;
        for(int ieta = -EBDetId::MAX_IETA; ieta <= EBDetId::MAX_IETA; ++ieta) {
                if(ieta==0) continue;
                for(int iphi = EBDetId::MIN_IPHI; iphi <= EBDetId::MAX_IPHI; ++iphi) {
                        if (EBDetId::validDetId(ieta,iphi)) {
                                EBDetId detid(ieta,iphi);
                                EcalLaserAPDPNRatios::EcalLaserAPDPNpair corr =  p.barrel(detid.hashedIndex());
                                nrv[icrys]   = corr.p2;
                                detId[icrys] = detid.hashedIndex();
                                ix[icrys]   = detid.ieta();
                                iy[icrys]   = detid.iphi();
                                iz[icrys]     = 0;
                                icrys++;
                        }
                }
        }

        for(int iZ = 1; iZ >= -1; --iZ){
                for(int iX = EEDetId::IX_MIN; iX <= EEDetId::IX_MAX; ++iX) {
                        for(int iY = EEDetId::IY_MIN; iY <= EEDetId::IY_MAX; ++iY) {
                                if (EEDetId::validDetId(iX,iY,iZ)) {
                                        EEDetId detid(iX,iY,iZ);
                                        EcalLaserAPDPNRatios::EcalLaserAPDPNpair corr =  p.endcap(detid.hashedIndex());
                                        nrv[icrys]   = corr.p2;
                                        detId[icrys] = detid.hashedIndex();
                                        ix[icrys]     = detid.ix();
                                        iy[icrys]     = detid.iy();
                                        iz[icrys]     = detid.zside();
                                        icrys++;
                                }
                        }
                }
        }
        tree->Fill();
}


int main( int argc, char** argv )
{
        cond::LaserValidation valida;
        return valida.run(argc,argv);
}
