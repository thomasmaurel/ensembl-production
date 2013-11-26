/*
 * Copyright [1999-2013] Wellcome Trust Sanger Institute and the EMBL-European Bioinformatics Institute
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iterator>
#include <vector>
#include <map>
#include <utility>                   // useful for std::pair
#include <algorithm>
#include <boost/regex.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/program_options.hpp>

#include "log4cpp/Appender.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/Layout.hh"
#include "log4cpp/BasicLayout.hh"
#include <log4cpp/PatternLayout.hh>
#include "log4cpp/Priority.hh"
#include "log4cpp/Category.hh"

//#include <boost/filesystem/path.hpp>
//#include <boost/filesystem/fstream.hpp>
//#include <boost/tokenizer.hpp>


// for std::for_each

#include "Constants.h"
#include "gff/GffParser.h"
#include "gff/GffSimpleHandler.h"
#include "gff/BioMartGffHandler.h"
#include "gff/SplicingEventGffGenerator.h"

using namespace std;
using namespace boost;
using namespace as;
using namespace gff;
namespace po = boost::program_options;

void outputUsage(const po::options_description& desc)
{
  std::cout << "altSpliceFinder 0.5.4\n" << endl
  //<< as::version::Version::getApplicationVersion()
  << "Copyright (c) 2008-2011 Gautier Koscielny <koscieln@ebi.ac.uk>" << endl
  << "The European Bioinformatics Institute and" << endl
	<< "Genome Research Limited, and others." << endl
	<< "All rights reserved.\n\n";

    // notice

    // usage
  std::cout << "Usage:" << endl << endl;
  std::cout << "  altSpliceFinder [-i gfffile] [-o resultfile]" << endl << endl;

  std::cout << desc << endl;
  std::cout << "Notes:"<< endl << "  Reads from the standard input stream if no input file is given." << endl
	    << "  Writes to the standard output stream if no output file is given." << endl;
}

int main(int argc, char **argv, char **ppenv)
{
  /**
   * Read program options
   */
  string inputFile;
  string outputFile;

  po::options_description desc("Allowed options");
  desc.add_options()
  ("version,V", "print version string")
  ("help,h", "Produce help message")
  ("verbose,v", "Log computational details on stderr")
  ("inputFile,i", po::value<std::string>(&inputFile), "Provide a path to a Ensembl BioMart gff file that contains transcript structure")
  ("outputFile,o", po::value<std::string>(&outputFile), "Provide a path to a gff file that will contains the splicing events")
  ("limit,l", po::value<int>(), "set a limit on the number of genes (with splicing events) to parse")
  ("constitutives,c", "Compute constitutive exon events only")
	("relax,r", "Compute splicing events with relaxed constraints on flanking features")
  ("statistics,s", "Compute and show statistics on splicing events");

  //("verbose", value<string>()->zero_tokens(), "verbosity level");
  //("listRejectedGenes,lrg", value<string>()->zero_tokens(), "store genes with variants but no events in a file");


  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  // argc < 2
  if (vm.count("help") || vm.count("version") || (argc > 1 && (!strcmp(argv[1], "-?") || !strcmp(argv[1], "--?") || !strcmp(argv[1], "/?") || !strcmp(argv[1], "/h") || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--h") || !strcmp(argv[1], "--help") || !strcmp(argv[1], "/help") || !strcmp(argv[1], "-help") || !strcmp(argv[1], "help")) ))
    {
      outputUsage(desc);
      return 1;
    }

  /*
   * Log for C++ Appender declaration
   */

  log4cpp::Appender* appender; // = Logger.getAppender();

  appender = new log4cpp::OstreamAppender("default", &std::cerr);
  log4cpp::PatternLayout* patternLayout = new log4cpp::PatternLayout();
  patternLayout->setConversionPattern("%d{%H:%M:%S:%l} %c: %m\n"); //"%R %p %c %x: %m\n");
  appender->setLayout(patternLayout);
  //appender->setLayout(new log4cpp::BasicLayout());
  log4cpp::Category& root = log4cpp::Category::getRoot();
  root.addAppender(appender);

  if (vm.count("verbose")) {

    root.setPriority(log4cpp::Priority::INFO);
    root.info("Switch to verbose mode.");

  } else {

    root.setPriority(log4cpp::Priority::ERROR);

  }

  ifstream inStream;
  ofstream outStream;

  bool inFile = false;
  bool outFile = false;

  int limit = (vm.count("limit")) ? vm["limit"].as<int>() : 0;

  if (vm.count("inputFile"))
    {
      inFile = true;
      root.infoStream() << "reading filename " << inputFile << log4cpp::eol;

      // ok, just try to open it and then close it
      inStream.open(inputFile.c_str(), ifstream::in);
      if(inStream.fail())
        {
          inStream.clear(ios::failbit);
          inStream.close();
          root.errorStream() << "Error:\tinput file """ << inputFile.c_str() << """ does not exists. Exiting program." << log4cpp::eol;
          exit(1);
        }

    } else {

      // check cin

      if(std::cin.fail()) {
	root.errorStream() << "Error:\tno data on input stream. Exiting program." << log4cpp::eol;
          exit(1);
      }

    }

  if (vm.count("outputFile"))
    {
      outFile = true;
      outStream.open(outputFile.c_str(), ios::out); //, ios_base::out );
      if(outStream.fail()) {
        outStream.clear(ios::failbit);
        outStream.close();
        cerr << "Error:\t enable to create output file """ << outputFile.c_str() << """." << endl;
        exit(1);
      }
    }

  SplicingEventGffGenerator gffGenerator((outFile) ? outStream : std::cout, "Ensembl", vm.count("constitutives"), vm.count("relax"));

  BioMartGffHandler martHandler(limit);
  martHandler.registerNewGeneEventListener(&gffGenerator);

  GffParser parser((inFile) ? inStream : std::cin, &martHandler);

  parser.parse();

  if (inFile)
    inStream.close();

  if (outFile)
    outStream.close();

  root.info("Parsing done.");

  if (vm.count("statistics")) {

    cerr << "Genes parsed:\t\t\t" << gffGenerator.getGeneCount() << endl;
    cerr << "Genes with multiple transcripts:\t" << gffGenerator.getGenesWithSeveralTranscriptsCount() << endl;
    cerr << "Genes with events:\t\t\t" << gffGenerator.getGenesWithEventsCount() << endl;
    cerr << "Splicing Events:\t\t\t" << gffGenerator.getEventCount() << endl;

    if (gffGenerator.getEventCount() > 0) {

      cerr << "Alternative Initiation  Events:\t" << gffGenerator.getCountAI() << " (" << ((int) ((100*gffGenerator.getCountAI())/gffGenerator.getEventCount())) << "%)" << endl ;
      cerr << "Alternative Termination Events:\t" << gffGenerator.getCountAT() << " (" << ((int) ((100*gffGenerator.getCountAT())/gffGenerator.getEventCount())) << "%)" << endl ;

      cerr << "Alternative First Exon  Events:\t" << gffGenerator.getCountAFE() << " (" << ((int) ((100*gffGenerator.getCountAFE())/gffGenerator.getEventCount())) << "%)" << endl ;
      cerr << "Alternative Last Exon   Events:\t" << gffGenerator.getCountALE() << " (" << ((int) ((100*gffGenerator.getCountALE())/gffGenerator.getEventCount())) << "%)" << endl ;

      cerr << "Exon Isoform            Events:\t" << gffGenerator.getCountEI() << " (" << ((int) ((100*gffGenerator.getCountEI())/gffGenerator.getEventCount())) << "%)" << endl ;
      cerr << "Intron Isoform          Events:\t" << gffGenerator.getCountII() << " (" << ((int) ((100*gffGenerator.getCountII())/gffGenerator.getEventCount())) << "%)" << endl ;
      cerr << "Intron Retention        Events:\t" << gffGenerator.getCountIR() << " (" << ((int) ((100*gffGenerator.getCountIR())/gffGenerator.getEventCount())) << "%)" << endl ;
      cerr << "Cassette Exon           Events:\t" << gffGenerator.getCountCE() << " (" << ((int) ((100*gffGenerator.getCountCE())/gffGenerator.getEventCount())) << "%)" << endl ;
      cerr << "Mutually Exclusive      Events:\t" << gffGenerator.getCountMXE() << " (" << ((int) ((100*gffGenerator.getCountMXE())/gffGenerator.getEventCount())) << "%)" << endl ;
    }
  }

  log4cpp::Category::shutdown();

  return (EXIT_SUCCESS);

}

