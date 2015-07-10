## Tools for dumping CMS database payloads

Package contents:
   * a general plugin for dumping several conditions at the same time/run
     from a given Global Tag, via cmsRun + configuration file.
   * several executables that can be launched with the general syntax of
     `cmscond_list_iov` plus some specific options, in order to dump payloads
     via a direct connection to a DB, given a tag. The options can be seen via
     `<dumper> --help`

Dumper list:
   * `adc2gev_dump.cpp`: dump ADC2GeV calibration constant
   * `alpha_dump.cpp`: dump the record of the alpha parameter of the ECAL response corrections
   * `bs_dump.cpp`: dump beam-spot parameters
   * `chstatus_dump.cpp`: dump ECAL channel status
   * `eseeic_dump.cpp`: dump ECAL ES-EE intercalibration parameters
   * `esgain_dump.cpp`: dump the ES gain
   * `esic_dump.cpp`: dump the ES inter-calibration constants
   * `ic_dump.cpp`: dump the ECAL inter-calibration
   * `lava_db.cpp`: validate a tag of the ECAL monitoring corrections
   * `lava_db_compare.cpp`: compare two tags of ECAL monitoring corrections
   * `lava_db_cond.cpp`: for ECAL monitoring corrections, compare the content
                         of a tag to the content of another tag, by matching the IOV of one tag to the
                         closest IOV of the other tag
   * `lava_db_dumpId.cpp`: dump ECAL monitoring correction histories for a given set of DetId (or all of them)
   * `lava_text.cpp`: convert the content of ECAL monitoring corrections to a txt file
   * `merge_dump.cpp`: merges different txt files of ECAL monitoring
                       corrections with overlapping IOV into one single
                       coherent txt file (N.B. does not need the CMSSW environment to work)
   * `ped_dump.cpp`: dump ECAL pedestals mean and rms for all of the three available gains (12, 6, 1)
   * `runinfo_dump.cpp`: dump start-time and stop-time of CMS runs
   * `timeic_dump.cpp`: dump ECAL time inter-calibration constants
   * `timeoffset_dump.cpp`: dump ECAL time offset constants

Todo:
   * generalize the dumpers, e.g. by deriving them from a common class with common
     options and specifying only the dumper function for each of the classes

```
scram project CMSSW_7_4_6
cd CMSSW_7_4_6//src/
cmsenv
git cms-merge-topic shervin86:iRingInSubdet
git clone git@github.com:ferriff/usercode.git
git remote add -m ecal_calib_tools ferriff git@github.com:ferriff/cmssw.git
git fetch ferriff
git checkout newBranch
git rebase --onto newBranch ebba82923bf40355e8763a0571cdd773e36ba458 ferriff/ecal_calib_tools
scram b -j16
```