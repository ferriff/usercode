#include "usercode/DBDump/interface/CondDBDumper.h"

#include "CondFormats/BeamSpotObjects/interface/BeamSpotObjects.h"
#include "CondFormats/EcalObjects/interface/EcalADCToGeVConstant.h"
#include "CondFormats/EcalObjects/interface/EcalChannelStatus.h"
#include "CondFormats/EcalObjects/interface/EcalClusterLocalContCorrParameters.h"
#include "CondFormats/EcalObjects/interface/EcalGainRatios.h"
#include "CondFormats/EcalObjects/interface/EcalIntercalibConstants.h"
#include "CondFormats/EcalObjects/interface/EcalIntercalibConstantsMC.h"
#include "CondFormats/EcalObjects/interface/EcalLaserAlphas.h"
#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatios.h"
#include "CondFormats/EcalObjects/interface/EcalPedestals.h"
#include "CondFormats/EcalObjects/interface/EcalPFRecHitThresholds.h"
#include "CondFormats/EcalObjects/interface/EcalPulseShapes.h"
#include "CondFormats/EcalObjects/interface/EcalPulseCovariances.h"
#include "CondFormats/EcalObjects/interface/EcalSamplesCorrelation.h"
#include "CondFormats/EcalObjects/interface/EcalSimPulseShape.h"
#include "CondFormats/EcalObjects/interface/EcalTimeCalibConstants.h"
#include "CondFormats/EcalObjects/interface/EcalTimeOffsetConstant.h"
#include "CondFormats/EcalObjects/interface/EcalTPGLinearizationConst.h"
#include "CondFormats/EcalObjects/interface/EcalTPGLutGroup.h"
#include "CondFormats/EcalObjects/interface/EcalTPGLutIdMap.h"
#include "CondFormats/EcalObjects/interface/EcalTPGPedestals.h"
#include "CondFormats/EcalObjects/interface/EcalTPGSlidingWindow.h"
#include "CondFormats/EcalObjects/interface/EcalTPGSpike.h"
#include "CondFormats/EcalObjects/interface/EcalTPGWeightGroup.h"
#include "CondFormats/EcalObjects/interface/EcalTPGWeightIdMap.h"
#include "CondFormats/ESObjects/interface/ESEEIntercalibConstants.h"
#include "CondFormats/ESObjects/interface/ESGain.h"
#include "CondFormats/ESObjects/interface/ESIntercalibConstants.h"
#include "CondFormats/RunInfo/interface/RunInfo.h"


int usage(char * s)
{
        fprintf(stderr, "Usage: %s -O <object> -t <tag> [conddb options]\nFull help available with the -h/--help options.\n", s);
        exit(1);
}


void list_supported(std::vector<std::string> & supported)
{
        fprintf(stderr, "Currently supported objects:\n");
        std::sort(supported.begin(), supported.end());
        for (size_t i = 0; i < supported.size(); ++i) {
                fprintf(stderr, "  %s\n", supported[i].c_str());
        }
}


int main(int argc, char** argv)
{
        if (argc < 2) {
                return usage(argv[0]);
        }

        std::string obj = "none";
        bool help = false;
        std::vector<std::string> supported;
        for (int i = 1; i < argc; ++i) {
                if (strcmp(argv[i], "-O") == 0 || strcmp(argv[i], "--object") == 0) {
                        obj = argv[i + 1];
                } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                        help = true;
                }
        }
        if (obj == "none" && !help) usage(argv[0]);

        supported.push_back("EcalIntercalibConstants");
        if (!help && obj == "EcalIntercalibConstants") {
                cond::CondDBDumper<EcalIntercalibConstants> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalIntercalibConstantsMC");
        if (!help && obj == "EcalIntercalibConstantsMC") {
                cond::CondDBDumper<EcalIntercalibConstantsMC> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalADCToGeVConstant");
        if (!help && obj == "EcalADCToGeVConstant") {
                cond::CondDBDumper<EcalADCToGeVConstant> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalLaserAlphas");
        if (!help && obj == "EcalLaserAlphas") {
                cond::CondDBDumper<EcalLaserAlphas> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalLaserAPDPNRatios");
        if (!help && obj == "EcalLaserAPDPNRatios") {
                cond::CondDBDumper<EcalLaserAPDPNRatios> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalChannelStatus");
        if (!help && obj == "EcalChannelStatus") {
                cond::CondDBDumper<EcalChannelStatus> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalPFRecHitThresholds");
        if (!help && obj == "EcalPFRecHitThresholds") {
                cond::CondDBDumper<EcalPFRecHitThresholds> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalPedestals");
        if (!help && obj == "EcalPedestals") {
                cond::CondDBDumper<EcalPedestals> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalTimeCalibConstants");
        if (!help && obj == "EcalTimeCalibConstants") {
                cond::CondDBDumper<EcalTimeCalibConstants> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalTimeOffsetConstant");
        if (!help && obj == "EcalTimeOffsetConstant") {
                cond::CondDBDumper<EcalTimeOffsetConstant> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalTPGLinearizationConst");
        if (!help && obj == "EcalTPGLinearizationConst") {
                cond::CondDBDumper<EcalTPGLinearizationConst> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalTPGLutGroup");
        if (!help && obj == "EcalTPGLutGroup") {
                cond::CondDBDumper<EcalTPGLutGroup> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalTPGLutIdMap");
        if (!help && obj == "EcalTPGLutIdMap") {
                cond::CondDBDumper<EcalTPGLutIdMap> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalTPGPedestals");
        if (!help && obj == "EcalTPGPedestals") {
                cond::CondDBDumper<EcalTPGPedestals> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalTPGWeightGroup");
        if (!help && obj == "EcalTPGWeightGroup") {
                cond::CondDBDumper<EcalTPGWeightGroup> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalTPGWeightIdMap");
        if (!help && obj == "EcalTPGWeightIdMap") {
                cond::CondDBDumper<EcalTPGWeightIdMap> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalTPGSlidingWindow");
        if (!help && obj == "EcalTPGSlidingWindow") {
                cond::CondDBDumper<EcalTPGSlidingWindow> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalTPGSpike");
        if (!help && obj == "EcalTPGSpike") {
                cond::CondDBDumper<EcalTPGSpike> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalClusterLocalContCorrParameters");
        if (!help && obj == "EcalClusterLocalContCorrParameters") {
                cond::CondDBDumper<EcalClusterLocalContCorrParameters> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalGainRatios");
        if (!help && obj == "EcalGainRatios") {
                cond::CondDBDumper<EcalGainRatios> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalSimPulseShape");
        if (!help && obj == "EcalSimPulseShape") {
                cond::CondDBDumper<EcalSimPulseShape> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalPulseShapes");
        if (!help && obj == "EcalPulseShapes") {
                cond::CondDBDumper<EcalPulseShapes> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalPulseCovariances");
        if (!help && obj == "EcalPulseCovariances") {
                cond::CondDBDumper<EcalPulseCovariances> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("EcalSamplesCorrelation");
        if (!help && obj == "EcalSamplesCorrelation") {
                cond::CondDBDumper<EcalSamplesCorrelation> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("ESEEIntercalibConstants");
        if (!help && obj == "ESEEIntercalibConstants") {
                cond::CondDBDumper<ESEEIntercalibConstants> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("ESGain");
        if (!help && obj == "ESGain") {
                cond::CondDBDumper<ESGain> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("ESIntercalibConstants");
        if (!help && obj == "ESIntercalibConstants") {
                cond::CondDBDumper<ESIntercalibConstants> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("BeamSpotObjects");
        if (!help && obj == "BeamSpotObjects") {
                cond::CondDBDumper<BeamSpotObjects> d(obj);
                d.run(argc, argv);
                return 0;
        }

        supported.push_back("RunInfo");
        if (!help && obj == "RunInfo") {
                cond::CondDBDumper<RunInfo> d(obj);
                d.run(argc, argv);
                return 0;
        }

        if (help) {
                cond::CondDBDumper<bool> d("Help");
                d.run(argc, argv);
                list_supported(supported);
                return 0;
        }

        fprintf(stderr, "Dumper for object `%s' not implemented.\n"
                "Please check the spelling and, if correct, ask the experts for its implementation.\n", obj.c_str());
        list_supported(supported);
        return 2;
}
