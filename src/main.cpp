#include <boost/program_options.hpp>
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include "machine.h"

using namespace std;

int main(int argc, char **argv){

    boost::program_options::options_description desc("Options");
    desc.add_options()
        ("help,h", "Print help message")
        ("singleStep,s", "Run in singleStep mode")    
        ("predScheme,p", boost::program_options::value<string>(), "branch prediction scheme AT/ANT/BI/SA")
        ("file,f", boost::program_options::value<string>(), "user program to run")
        ("config,c", boost::program_options::value<string>(), "config file used for performance test")
        ("debug,d", "use debug mode")
        ;
 
    boost::program_options::variables_map vm;

    try{
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);
    }
    catch (boost::exception& e) {
        printf("parse command line exception!\n");;
        cout << desc << endl;
        return false;
    }

    if(vm.count("help")){
        cout << desc << endl;
        return 0;
    }

    bool singleStep = false;
    if(vm.count("singleStep"))
        singleStep = true;

    bool Debug = false;
    if(vm.count("debug"))
        Debug = true;

    string fileName;
    if(vm.count("file"))
        fileName = vm["file"].as<string>();
    else{
        printf("please specify a user program!\n");
        cout << desc << endl;
        return 0;
    }

    string configName = "default.cfg";
    if(vm.count("config"))
        configName = vm["config"].as<string>();
    else{
        printf("*****By default config file is default.cfg!*****\n");
        printf(":using -c to specify config file\n\n");
    }

    string scmName;
    if(vm.count("predScheme")){
        scmName = vm["predScheme"].as<string>();
    }
    /*build machine*/
    Machine myMachine;
    myMachine.SetPredScheme(AlwaysNotTaken);

    if(scmName.compare("AT") == 0)
        myMachine.SetPredScheme(AlwaysTaken);
    else if(scmName.compare("ANT") == 0)
        myMachine.SetPredScheme(AlwaysNotTaken);
    else if(scmName.compare("BI") == 0)
        myMachine.SetPredScheme(Bimodal);
    else if(scmName.compare("SA") == 0)
        myMachine.SetPredScheme(SelfAdj);
    else{
        printf("***Please choose a proper branch prediction scheme***\n");
        printf("AT    ------------Always Taken\n");
        printf("ANT   ------------Always Not Taken\n");
        printf("Bi    ------------Bimodal\n");
        printf("SA    ------------Self Adjustment\n");
        printf("By default : Always Not Taken\n");
        printf("******************************************************\n");
    }

    FILE *f = fopen(configName.c_str(), "r");
    if(f == NULL){
        printf("config file does not exists\n");
        printf("DO NOT REMOVE default.cfg\n");
        return 0;
    }
    myMachine.PfmConfig(f);

    myMachine.sgStep = singleStep;
    myMachine.debug = Debug;
    myMachine.ReadUserProg(fileName.c_str());
    myMachine.Run();

    return 0;
}

