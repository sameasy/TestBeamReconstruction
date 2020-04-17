#include "UserCode/DataProcessing/interface/CLUEAnalysis.h"

//Simplified with respect to the CMSSW version, since
//   1) Only considers the EE section
//   2) Has W_0=2.9 hardcoded
//Returns x and y position of the cluster
void CLUEAnalysis::calculatePositionsAndEnergy(const std::vector<float>& xpos, const std::vector<float>& ypos, const std::vector<float>& weights, const std::vector<int>& clusterid, const std::vector<int>& layerid) {
  assert(!xpos.empty() && !ypos.empty() && !weights.empty() && !clusterid.empty()&& !layerid.empty());
  auto start = std::chrono::high_resolution_clock::now();

  const int nclusters = ( *( std::max_element(clusterid.begin(), clusterid.end()) ) 
			  + 1 /*cluster index starts at zero*/  + 1 /*outliers*/ );
  std::vector<float> total_weight(nclusters, 0.);
  std::vector<float> total_weight_log(nclusters, 0.) ;
  std::vector<float> x(nclusters, 0.);
  std::vector<float> y(nclusters, 0.);
  std::vector<float> layers(nclusters, 0.);

  for(auto i: util::lang::indices(weights))
    {
      unsigned int weight_index = clusterid.at(i) + 1; //outliers will correspond to total_weight[0]
      weight_index = weight_index;
      total_weight.at(weight_index) += weights.at(i);
    }

  en_ = total_weight; //copy

  for (auto i: util::lang::indices(weights))
    {
      unsigned int weight_index = clusterid.at(i) + 1; //outliers will correspond to total_weight[0]
      float Wi = std::max(2.9 + std::log(weights.at(i) / total_weight.at(weight_index)), 0.);
      x.at(weight_index) += xpos.at(i) * Wi;
      y[weight_index] += ypos.at(i) * Wi;
      total_weight_log[weight_index] += Wi;
      if(layers[weight_index] == 0 and weight_index != 0) 
	layers[weight_index] = layerid.at(i); //all hits in a cluster will belong to the same layer
    }

  for(auto i : util::lang::indices(total_weight_log))
    {
      if (total_weight_log.at(i) != 0.) {
	float inv_tot_weight_log = 1.f / total_weight_log.at(i);
	pos_.push_back( std::make_tuple( x.at(i) * inv_tot_weight_log, y.at(i) * inv_tot_weight_log, layers.at(i) ) );
      } 
      else
	pos_.push_back( std::make_tuple(0.f, 0.f, 0.f) );
    }

  auto finish = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = finish - start;
  //std::cout << "--- calculatePositions      " << elapsed.count() *1000 << " ms\n\n";
}


//Simplified with respect to the CMSSW version, since
//   1) Only considers the EE section
//   2) Has W_0=2.9 hardcoded
//Returns x and y position of the cluster
void CLUEAnalysis::calculatePositions(const std::vector<float>& xpos, const std::vector<float>& ypos, const std::vector<float>& weights, const std::vector<int>& clusterid, const std::vector<int>& layerid) {
  auto start = std::chrono::high_resolution_clock::now();

  const int nclusters = *( std::max_element(clusterid.begin(), clusterid.end()) ) + 1; //number of clusters plus outliers
  std::vector<float> total_weight(nclusters, 0.);
  std::vector<float> total_weight_log(nclusters, 0.) ;
  std::vector<float> x(nclusters, 0.);
  std::vector<float> y(nclusters, 0.);
  std::vector<float> layers(nclusters, 0.);

  for(auto i: util::lang::indices(weights))
    {
      unsigned int weight_index = clusterid[i] + 1; //outliers will correspond to total_weight[0]
      total_weight[weight_index] += weights[i];
    }

  for (auto i: util::lang::indices(weights))
    {
      unsigned int weight_index = clusterid[i] + 1; //outliers will correspond to total_weight[0]
      float Wi = std::max(2.9 + std::log(weights[i] / total_weight[weight_index]), 0.);
      x[weight_index] += xpos[i] * Wi;
      y[weight_index] += ypos[i] * Wi;
      total_weight_log[weight_index] += Wi;
      if(layers[weight_index] == 0) 
	layers[weight_index] = layerid[i]; //all hits in a cluster will belong to the same layer
    }

  for(auto i : util::lang::indices(total_weight_log))
    {
      if (total_weight_log[i] != 0.) {
	float inv_tot_weight_log = 1.f / total_weight_log[i];
	pos_.push_back( std::make_tuple( x[i] * inv_tot_weight_log, y[i] * inv_tot_weight_log, layers[i] ) );
      } 
      else
	pos_.push_back( std::make_tuple(0.f, 0.f, 0.f) );
    }

  auto finish = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = finish - start;
  std::cout << "--- calculatePositions      " << elapsed.count() *1000 << " ms\n\n";
}

void CLUEAnalysis::calculateEnergy( const std::vector<float>& weights, const std::vector<int>& clusterid ) {
  auto start = std::chrono::high_resolution_clock::now();

  const int nclusters = *( std::max_element(clusterid.begin(), clusterid.end()) ) + 1; //number of clusters plus outliers
  std::vector<float> total_weight(nclusters, 0.);

  for(auto i: util::lang::indices(weights))
    {
      unsigned int weight_index = clusterid[i] + 1; //outliers will correspond to total_weight[0]
      total_weight[weight_index] += weights[i];
    }

  en_ = total_weight;

  auto finish = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = finish - start;
  std::cout << "--- calculateEnergy      " << elapsed.count() *1000 << " ms\n\n";
}

//Returns quantities of interest of individual clusters
std::vector<dataformats::data> CLUEAnalysis::getIndividualClusterOutput(std::string& outputFileName, bool verbose) {
  bool pos_set = !pos_.empty(), en_set = !en_.empty();

  if(verbose) 
    {
      std::ofstream oFile(outputFileName, std::ios::out);
      if(pos_set && en_set)
	assert(en_.size() == pos_.size());
      oFile << "###Cluster Analysis: individual clusters (posx, posy, posz, energy)\n";
      for(unsigned int i=0; i<pos_.size(); ++i)
	{
	  if(pos_set)
	    oFile << std::get<0>(pos_[i]) << "," << std::get<1>(pos_[i]) << "," << std::get<2>(pos_[i]);
	  if(en_set)
	    oFile << en_[i];
	  oFile << std::endl;
	}
      oFile.close();
    }
  
  std::vector<dataformats::data> d;
  for(auto i : util::lang::indices(pos_))
    {
      float posx=0., posy=0., posz=0., en=0.;
      if(pos_set)
	{
	  posx = std::get<0>(pos_[i]);
	  posy = std::get<1>(pos_[i]);
	  posz = std::get<2>(pos_[i]);
	}
      if(en_set)
	en = en_[i];
      d.push_back( std::make_tuple(posx,posy,posz,en) );
    }
  return d;
}

//Returns the sum of all quantities of interest of individual clusters
float CLUEAnalysis::getTotalClusterOutput(const std::string& outputFileName, bool verbose) {
  assert( !en_.empty() );
  float toten = 0.;
  for(unsigned int i=1; i<en_.size(); ++i) //outliers (i=0) are skipped!
    toten += en_[i];
  if(verbose) 
    {
      std::ofstream oFile(outputFileName, std::ios::out);
      oFile << "###Cluster Analysis: total energy\n";
      oFile << toten << std::endl;
      oFile.close();
    }
  return toten;
}