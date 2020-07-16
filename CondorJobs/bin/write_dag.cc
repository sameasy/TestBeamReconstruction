#include <cstdlib>
#include "UserCode/DataProcessing/interface/analyzer.h"

//convenience function which prints all the elements in a vector of strings to std::cout
void print_vector_elements(const std::vector<std::string>& v)
{
  for(auto& elem : v)
    if (elem == v.back())
      std::cout << elem << std::endl;
    else
      std::cout << elem << " / ";
}

//write all individual submission jobs: selection stage
void write_submission_file(const int& id, const std::string& jobpath, const std::string& base, std::string mode,
			   const std::string& datatype, const unsigned int& energy=0)
{
  assert(mode == "selection" or mode == "analysis");
  std::string n = std::to_string(id);
  std::ofstream fw(jobpath, std::ios_base::ate);

  std::string mode2, memory, flavour;
  if(mode == "selection") {
    mode2 = "selector";
    memory = "1.5GB";
    flavour = "\"workday\"";
    fw << "executable = " + base + "selector.sh" << std::endl;
  }
  else {
    mode2 = "analyzer";
    memory = "500MB";
    flavour = "\"longlunch\"";
    fw << "executable = " + base + "analyzer.sh" << std::endl;
  }
  
  fw << "arguments = --ntupleid " + n + " --datatype " + datatype;
  if (datatype.find("sim") != std::string::npos)
      fw << " --energy " + std::to_string(energy);
  fw << std::endl;

  fw << "universe = vanilla" << std::endl;
  fw << "requirements = (OpSysAndVer =?= \"CentOS7\")" << std::endl;
  fw << "output = " + base + "out/" + mode2 + "_" + datatype + "." + n + ".out" << std::endl;
  fw << "error = " + base + "out/" + mode2 + "_" + datatype + "." + n + ".err" << std::endl;
  fw << "log = " + base + "log/" + mode2 + "_" + datatype + "." + n + ".log" << std::endl;

  fw << "RequestMemory = " + memory << std::endl;
  fw << "+JobFlavour = " + flavour << std::endl;
  fw << "queue" << std::endl;
}

//write Direct Acyclic Graph (DAG) submission file: jobs definition
void write_dag_jobs(const std::string& filepath, const std::vector<std::string>& jobnames, const std::vector<std::string>& jobpaths, const std::ios_base::openmode& mode)
{
  assert( jobnames.size() == jobpaths.size() );
  
  std::ofstream f_write(filepath, mode);
  
  for(auto i: util::lang::indices(jobnames))
    {
      std::string in = "JOB " + jobnames[i] + "\t" + jobpaths[i];
      f_write << in << std::endl;
    }
  f_write << std::endl;
}

//write Direct Acyclic Graph (DAG) submission file: jobs parent/child relationships
void write_dag_hierarchy(const std::string& filepath, const std::vector<std::string>& jobdads, const std::vector<std::string>& jobchilds)
{
  assert( jobchilds.size() == jobdads.size() );

  std::ofstream f_write(filepath, std::ios_base::app);
  
  for(auto i: util::lang::indices(jobchilds))
    {
      std::string in2 = "PARENT " + jobdads[i] + " CHILD " + jobchilds[i];
      f_write << in2 << std::endl;
    }
  f_write << std::endl;
}

void write_dag_repetitions(const std::string& filepath, const std::vector<std::string>& jobnames, const int& nrepetitions)
{
  std::ofstream f_write(filepath, std::ios_base::app);
  
  for(auto i: util::lang::indices(jobnames))
    {
      std::string in = "RETRY " + jobnames[i] + " " + std::to_string(nrepetitions);
      f_write << in << std::endl;
    }
  f_write << std::endl;
}

void write_data(const std::string& submission_folder, const std::string& base)
{
  std::ifstream infile(base + "ntuple_ids.txt");
  std::vector<int> a;
  int _a;
  while (infile >> _a)
      a.push_back(_a);
  //std::vector<int> avoid = {453,508,601,607,639};   //some jobs failed, therefore some input files are lacking

  std::vector<int> file_id;
  for(unsigned int i=435; i<=509; ++i)
    {
      if( std::find(a.begin(), a.end(), i) != a.end() /*and std::find(avoid.begin(), avoid.end(), i) == avoid.end()*/ )
	file_id.push_back(i);
    }
  for(unsigned int i=594; i<=676; ++i)
    {
      if( std::find(a.begin(), a.end(), i) != a.end() /*and std::find(avoid.begin(), avoid.end(), i) == avoid.end()*/ )
	file_id.push_back(i);
    }

  constexpr unsigned int nsteps = 2;
  std::vector<std::string> steps = {"selection", "analysis"};
  assert(steps.size() == nsteps);
  
  std::string filepath = base + "clue_data.dag";
  std::vector< std::vector<std::string> > jobnames(nsteps, std::vector<std::string>());
  std::vector< std::vector<std::string> > jobpaths(nsteps, std::vector<std::string>());
  for(auto thisStep: util::lang::indices(steps))
    {
      for(auto i: util::lang::indices(file_id))
	{
	  std::string n = std::to_string(file_id[i]);
	  jobnames[thisStep].push_back(steps[thisStep] + "data_ " + n);
	  jobpaths[thisStep].push_back(base + submission_folder + steps[thisStep] + "/" + steps[thisStep] + n + ".sub");
	}
      if(thisStep == 0)
	write_dag_jobs(filepath, jobnames[thisStep], jobpaths[thisStep], std::ios_base::ate);
      else
	write_dag_jobs(filepath, jobnames[thisStep], jobpaths[thisStep], std::ios_base::app);
    }

  write_dag_hierarchy(filepath, jobnames[0], jobnames[1]);

  //write individual analysis jobs which will be submitted all at once by the DAG written above
  for(auto i: util::lang::indices(file_id))
    {
      write_submission_file(i, jobpaths[0][i], base, steps[0], "data"); //selection
      write_submission_file(i, jobpaths[1][i], base, steps[1], "data"); //analysis
    }
}

void write_simulation(const std::string& submission_folder, const std::string& base, const std::string& datatype, const bool& last_step_only)
{
  constexpr unsigned int ntuples_per_energy = 5; //ntuples indexes with simulation data range from 0 to 4
  std::vector<unsigned int> energies = {{20, 30, 50, 80, 100, 120, 150, 200, 250, 300}};
  const unsigned int nenergies = energies.size();
  const unsigned int njobs = nenergies*ntuples_per_energy;

  std::string filepath;
  std::vector<std::string> steps;
  if(last_step_only) {
    steps = {"analysis"};
    filepath = base + "clue_" + datatype + "_" + steps[0] + "_only.dag";
  }
  else {
    steps = {"selection", "analysis"};
    filepath = base + "clue_" + datatype + ".dag";
  }
  const unsigned int nsteps = steps.size();
  
  std::vector< std::vector<std::string> > jobnames(nsteps, std::vector<std::string>(njobs));
  std::vector< std::vector<std::string> > jobpaths(nsteps, std::vector<std::string>(njobs));
  for(auto thisStep: util::lang::indices(steps)) {
      for(auto i: util::lang::indices(energies)) {
	for(unsigned int j=0; j<ntuples_per_energy; ++j)
	  {
	    const std::string en_str = std::to_string(energies[i]);
	    const std::string idx_str = std::to_string(j);
	    const std::string this_jobname = steps[thisStep] + "_" + datatype + "_beamen" + en_str + "_" + idx_str;
	    jobnames[thisStep].at(j + i*ntuples_per_energy) = this_jobname;
	    jobpaths[thisStep].at(j + i*ntuples_per_energy) = base + submission_folder + steps[thisStep] + "/" + this_jobname + ".sub";
	  }
      }
      if(thisStep == 0)
	write_dag_jobs(filepath, jobnames[thisStep], jobpaths[thisStep], std::ios_base::ate);
      else
	write_dag_jobs(filepath, jobnames[thisStep], jobpaths[thisStep], std::ios_base::app);
    }

  if(!last_step_only)
    {
      write_dag_hierarchy(filepath, jobnames[0], jobnames[1]);
      write_dag_repetitions(filepath, jobnames[1], 2);
    }
  write_dag_repetitions(filepath, jobnames[0], 1);

  //write individual analysis jobs which will be submitted all at once by the DAG written above
  for(auto i: util::lang::indices(energies)) {
    for(unsigned int j=0; j<ntuples_per_energy; ++j)
      {
	write_submission_file(j, jobpaths[0][j + i*ntuples_per_energy], base, steps[0], datatype, energies[i]);
	if(!last_step_only)
	  write_submission_file(j, jobpaths[1][j + i*ntuples_per_energy], base, steps[1], datatype, energies[i]);
      }
  }
}

int main(int argc, char **argv) {
  std::vector<std::string> okargs = {"--datatype", "--last_step_only"};
  std::vector<std::string> datatypes = {"data", "sim_noproton", "sim_proton"};

  //input arguments' checks
  if(argc < 3 or argc > 4) {
    std::cout << "Ypu must specify the following:" << std::endl;
    std::cout << "1) data type: ";
    print_vector_elements(datatypes);
    std::cout << "2) last_step_only (optional)" << std::endl;
    return 1;
  }
  for(unsigned int iarg=0; iarg<static_cast<unsigned int>(argc); ++iarg)
    {
      bool tempbool = std::find(okargs.begin(), okargs.end(), std::string(argv[iarg])) != okargs.end();
      bool tempbool2 = std::string(argv[iarg]).find("--") != std::string::npos;
      std::cout <<std::string(argv[iarg]) << std::endl;
      std::cout << tempbool << ", " << tempbool2 << std::endl;
    }
  for(unsigned int iarg=0; iarg<static_cast<unsigned int>(argc); ++iarg)
    {
      if(std::string(argv[iarg]).find("--") != std::string::npos and
	 std::find(okargs.begin(), okargs.end(), std::string(argv[iarg])) == okargs.end())
	{
	  std::cout << "The arguments currently supported are '--datatype' and '--last_step_only'." << std::endl;
	  return 1;
	}
    }

  std::string datatype;
  bool last_step_only = false;
  for(unsigned int iarg=0; iarg<static_cast<unsigned int>(argc); ++iarg)
    {
      if(std::string(argv[iarg]) == "--datatype") {
	if( std::find(datatypes.begin(), datatypes.end(), std::string(argv[iarg+1])) == datatypes.end() )
	  {
	    std::cout << "The data type has to be one of the following: ";
	    print_vector_elements(datatypes);
	    return 1;
	  }
	else
	  datatype = std::string(argv[iarg+1]);
      }
      else if(std::string(argv[iarg]) == "--last_step_only")
	last_step_only = true;
  }
  
  //define common variables
  std::string cmssw_base = std::getenv("CMSSW_BASE");
  std::string condorjobs = "/src/UserCode/CondorJobs/";
  std::string submission_folder = "submission/";
  std::string condorjobs_base = cmssw_base + condorjobs;

  //write DAG files
  if(datatype == "data")
    write_data(submission_folder, condorjobs_base);
  else
    write_simulation(submission_folder, condorjobs_base, datatype, last_step_only);
}