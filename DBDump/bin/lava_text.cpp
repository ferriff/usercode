#include "CondFormats/EcalObjects/interface/EcalLaserAPDPNRatios.h"
#include "DataFormats/EcalDetId/interface/EBDetId.h"
#include "DataFormats/EcalDetId/interface/EEDetId.h"
#include "DataFormats/Provenance/interface/Timestamp.h"
#include "CalibCalorimetry/EcalLaserAnalyzer/interface/MEEBGeom.h"
#include "CalibCalorimetry/EcalLaserAnalyzer/interface/MEEEGeom.h"

#include "../interface/EcalLaserPlotter.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME 512

void usage(const char * prg)
{
        printf("Usage: %s <options>\n", prg);
        printf("where option can be:\n");
        printf("-b, --break,                break after time or iov limits have been\n");
        printf("                            exceeded (useful for time ordered text files\n");
        printf("                            [default: 1, i.e. break]\n");
        printf("-g, --geom <filename>,      specify the geometry file name\n");
        printf("                            [default: detid_geom.dat]\n");
        printf("                            in the format \"detid eta phi r\"\n");
        printf("-i, --input <filename>,     specify the input file name\n");
        printf("                            [default: input.dat]\n");
        printf("-n, --niov <niov>,          specify the number of IOV from <tmin> to be\n");
        printf("                            used in the validation\n");
        printf("                            [default: -1, all the IOV]\n");
        printf("-o, --output <filename>,    specify the output file name\n");
        printf("                            [default: ecallaserplotter.root]\n");
        printf("-S, --status <filename>,    specify the EcalChannelStatus file name\n");
        printf("                            [default: none]\n");
        printf("                            in the format \"detid status\"\n");
        printf("-t, --tmin <tmin>,          specify the time to start the validation from\n");
        printf("                            in second since the Epoch (1970-01-01), UTC\n");
        printf("                            [default: -1, i.e. no minimum]\n");
        printf("-T, --tmax <tmax>,          specify the time to end the validation to\n");
        printf("                            in second since the Epoch (1970-01-01), UTC\n");
        printf("                            [default: -1, i.e. no maximum]\n");
        printf("-s, --prescale <prescale>,  prescale value for IOV analysis\n");
        printf("                            [default: 1]\n");
        exit(1);
}


int main( int argc, char** argv )
{
        if (argc == 1) usage(argv[0]);
        int c;
        int verbose_flag, quiet_flag;
        char input[MAX_FILENAME], output[MAX_FILENAME], geom[MAX_FILENAME], status[MAX_FILENAME];
        sprintf(input, "input.dat");
        sprintf(output, "output.root");
        sprintf(geom, "detid_geom.dat");
        sprintf(status, " ");

        int tmin = -1, tmax = -1, niov = -1, br = 0, prescale = 1;

        while (1) {

                static struct option long_options[] =
                {
                        {"verbose",  no_argument, &verbose_flag, 1},
                        {"quiet",    no_argument, &quiet_flag, 0},
                        {"input",    required_argument, 0, 'i'},
                        {"output",   required_argument, 0, 'o'},
                        {"geom",     required_argument, 0, 'g'},
                        {"status",   required_argument, 0, 'S'},
                        {"tmin",     required_argument, 0, 't'},
                        {"tmax",     required_argument, 0, 'T'},
                        {"niov",     required_argument, 0, 'n'},
                        {"break",    required_argument, 0, 'b'},
                        {"prescale", required_argument, 0, 's'},
                };

                int option_index = 0;
                c = getopt_long(argc, argv, "vqi:o:g:S:s:t:T:n:b", long_options, &option_index);
                if (c == -1) break;

                switch (c) {
                        case 0:
                                if (long_options[option_index].flag != 0) break;
                                printf("option %s", long_options[option_index].name);
                                if (optarg) printf(" with arg %s", optarg);
                                printf("\n");
                                break;
                        case 'b':
                                br = atoi(optarg);
                                printf("break: %d\n", br);
                                break;
                        case 'g':
                                sprintf(geom, "%s", optarg);
                                printf("geometry file: %s\n", geom);
                                break;
                        case 'i':
                                sprintf(input, "%s", optarg);
                                printf("input file: %s\n", input);
                                break;
                        case 'n':
                                niov = atoi(optarg);
                                printf("number of IOV: %d\n", niov);
                                break;
                        case 'o':
                                sprintf(output, "%s", optarg);
                                printf("output file: %s\n", output);
                                break;
                        case 'S':
                                sprintf(status, "%s", optarg);
                                printf("status file: %s\n", status);
                                break;
                        case 't':
                                tmin = atoi(optarg);
                                printf("tmin: %d\n", tmin);
                                break;
                        case 'T':
                                tmax = atoi(optarg);
                                printf("tmax: %d\n", tmax);
                                break;
                        case 's':
                                prescale = atoi(optarg);
                                printf("prescale: %d\n", tmax);
                                break;
                        default:
                                usage(argv[0]);
                }
        }

        FILE * fd = fopen(input, "r");
        if (fd == NULL) {
                fprintf(stderr, "error opening file `%s': %s.\n", input, strerror(errno));
                exit(2);
        }

        FILE * ftmp = fopen(geom, "r");
        if (ftmp == NULL) {
                fprintf(stderr, "error opening file `%s': %s.\n", geom, strerror(errno));
                exit(3);
        } else {
                fclose(ftmp);
        }

        EcalChannelStatus ch_status_;
        if (strcmp(status, " ") != 0) {
                FILE * fstatus = fopen(status, "r");
                if (fstatus == NULL) {
                        fprintf(stderr, "error opening file `%s': %s.\n", status, strerror(errno));
                        exit(4);
                } else {
                        uint32_t id;
                        uint16_t st;
                        int cnt = 0;
                        while (fscanf(fstatus, "%u %hu", &id, &st) != EOF) {
                                EcalChannelStatusCode code(st);
                                ch_status_[id] = code;
                                ++cnt;
                        }
                        assert(cnt == 75848);
                        fclose(fstatus);
                }
        }

        char * line = NULL;
        size_t len = 0;
        ssize_t read;
        int first = 1;
        EcalLaserPlotter lp(geom);
        if (strcmp(status, " ") != 0) lp.setEcalChannelStatus(ch_status_, 1);
        EcalLaserAPDPNRatios apdpn;
        time_t t[92], t1, t3; //, ot3 = 0;
        int cnt = 0, cnt_iov = 0, skip = 0;
        int id;
        char * beg, * end;
        EcalLaserAPDPNRatios::EcalLaserAPDPNpair p;
        EcalLaserAPDPNRatios::EcalLaserTimeStamp ts;
        size_t cnt_line = 0;
        while( (read = getline(&line, &len, fd)) != EOF ) {
                ++cnt_line;
                if (line[0] == 'T') {
                        if (!first && !skip) lp.fill(apdpn, t1);
                        skip = 0;
                        ++cnt_iov;
                        // new sequence
                        sscanf(line, "T %ld %ld", &t1, &t3);
                        char bufb[128], bufe[128];
                        strftime(bufb, sizeof(bufb), "%F %R:%S", gmtime(&t1));
                        strftime(bufe, sizeof(bufe), "%F %R:%S", gmtime(&t3));
                        if (tmin > 0 && t1 < tmin)      skip = 1;
                        if (tmax > 0 && t1 > tmax)      skip = 1;
                        if (niov > 0 && cnt_iov > niov) skip = 1;
                        if (cnt_iov % prescale != 0)    skip = 1;
                        if (skip) {
                                if (br) break;
                                printf("skipping IOV %5d     begin: %s (%ld)    end: %s (%ld)\n", cnt_iov, bufb, t1, bufe, t3);
                                continue;
                        } else  printf("IOV %5d     begin: %s (%ld)    end: %s (%ld)\n", cnt_iov, bufb, t1, bufe, t3);
                        beg = &line[1];
                        for (int i = -2, j = -2; i < 92; ++i, ++j) {
                                if(i < 0) j = 0;
                                t[j] = strtol(beg, &end, 0);
                                beg = end;
                        }
                        if (!(t1 < t3)) {
                                lp.save(output);
                                fprintf(stderr, "ERROR in line %lu: t1 < t3 failed\n", cnt_line);
                                assert(0);
                        }
                        for (int i = 0; i < 92; ++i) {
                                if(!(t1 <= t[i] && t[i] <= t3)) {
                                        lp.save(output);
                                        fprintf(stderr, "ERROR in line %lu: t1 <= t[i] && t[i] <= t3 failed\n", cnt_line);
                                        assert(0);
                                }
                        }
                        //printf("%d %d %d\n", cnt, EBDetId::MAX_HASH, EEDetId::kSizeForDenseIndexing);
                        if (!first) {
                                //assert(ot3 == t1); // not always true if sequences are taken from DB
                                if (!(cnt == EBDetId::MAX_HASH + 1 + EEDetId::kSizeForDenseIndexing)) {
                                        fprintf(stderr, "found only %d DetId(s) at line %lu\n", cnt, cnt_line);
                                }
                                assert(cnt == EBDetId::MAX_HASH + 1 + EEDetId::kSizeForDenseIndexing);
                        }
                        //ot3 = t3;
                        cnt = 0;
                        first = 0;
                        free(line);
                        line = NULL;
                } else {
                        sscanf(line, "P %d %f %f %f", &id, &p.p1, &p.p2, &p.p3);
                        if (skip) continue;
                        ts.t1 = (edm::Timestamp)t1;
                        DetId did(id);
                        int iLM;
                        if (did.subdetId() == EcalBarrel) {
                                EBDetId ebid(id);
                                iLM = MEEBGeom::lmr(ebid.ieta(), ebid.iphi());
                        } else if (did.subdetId() == EcalEndcap) {
                                EEDetId eeid(id);
                                // SuperCrystal coordinates
                                MEEEGeom::SuperCrysCoord iX = (eeid.ix()-1)/5 + 1;
                                MEEEGeom::SuperCrysCoord iY = (eeid.iy()-1)/5 + 1;    
                                iLM = MEEEGeom::lmr(iX, iY, eeid.zside());    
                        } else {
                                fprintf(stderr, "ERROR on line %lu: DetId %d not belonging to EB nor to EE\n", cnt_line, id);
                                assert(0);
                        }
                        ts.t2 = (edm::Timestamp)t[iLM - 1];
                        ts.t3 = (edm::Timestamp)t3;
                        apdpn.setValue(id, p);
                        apdpn.setTime(iLM - 1, ts);
                        ++cnt;
                        free(line);
                        line = NULL;
                }
        }
        if (!skip) lp.fill(apdpn, t1);
        lp.save(output);
        return 0;
}
