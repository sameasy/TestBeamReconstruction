#include "UserCode/DataProcessing/interface/CLUEAnalysis.h"

CLUEAnalysis::CLUEAnalysis(const SHOWERTYPE& s): showertype(s)
{
  if(showertype == SHOWERTYPE::EM)
    lmax = detectorConstants::nlayers_emshowers;
  else if(showertype == SHOWERTYPE::HAD)
    lmax = detectorConstants::totalnlayers;
  else
    throw std::invalid_argument("Wrong shower type.");

  this->layerdep_vars_.resize(lmax);
  this->clusterdep_vars_.resize(lmax);
}

void CLUEAnalysis::calculateEnergy( const std::vector<float>& weights, const std::vector<int>& clusterid ) {
  const int nclusters = *( std::max_element(clusterid.begin(), clusterid.end()) ) 
    + 1 /*cluster index starts at zero*/ + 1 /*outliers*/;
  std::vector<float> total_weight(nclusters, 0.);

  for(auto i: util::lang::indices(weights))
    {
      unsigned weight_index = clusterid.at(i) + 1; //outliers will correspond to total_weight[0]
      total_weight.at(weight_index) += weights[i];
    }
  en_ = total_weight;
}

//calculate the number of clusterized hits and clusterized energy per layer
void CLUEAnalysis::calculateLayerDepVars(const std::vector<float>& xpos, const std::vector<float>& ypos, const std::vector<float>& weights, const std::vector<int>& clusterid, const std::vector<int>& layerid, const std::vector<float>& rhos, const std::vector<float>& deltas, const std::vector<bool>& seeds, const std::vector<unsigned>& nhitsincluster) {
  assert(!weights.empty() && !clusterid.empty() && !layerid.empty() && !rhos.empty() && !deltas.empty());

  //calculate the number of rechits and clusterized energy per layer
  std::vector<unsigned> hits_per_layer(this->lmax, 0);
  std::vector< std::vector<float> > en_per_layer(this->lmax);
  std::vector< std::vector<float> > rhos_per_layer(this->lmax);
  std::vector< std::vector<float> > deltas_per_layer(this->lmax);
  std::vector< std::vector<bool>  > seeds_per_layer(this->lmax);
  std::vector< std::vector<float> > xpos_per_layer(this->lmax);
  std::vector< std::vector<float> > ypos_per_layer(this->lmax);
  std::vector< std::vector<unsigned> > cluster_size_per_layer(this->lmax); //number of hits in the cluster for each hit and layer

  //unsigned this_cluster_size = 1;
  //int old_i = -1;
  for(auto i: util::lang::indices(weights))
    {
      if(clusterid[i] != -1)  //outliers are not considered
	{
	  int layeridx = layerid[i]-1; //layers start at 1

	  //////////////////////////////////////////////////////////////////////////////////////
	  /*Algorithm for finding the number of hits in the cluster to which each hit is associated*/

	  /*
	  if( i >= old_i + this_cluster_size ) //'==' does not work when clusterid == -1
	    {
	      this_cluster_size = 1;
	      unsigned next_idx = i+1;
	      if(next_idx < weights.size()) //if it overflows the cluster size is 1 (last hit in the vector)
		{
		  while( clusterid[i] == clusterid[next_idx])
		    {
		      this_cluster_size += 1;
		      ++next_idx;
		      if( next_idx >= weights.size()) //exit if it overflows
			break;
		    }
		}
	      for(unsigned j=0; j<this_cluster_size; ++j)
		cluster_size_per_layer.at(layeridx).push_back( this_cluster_size );
	      old_i = i;
	    }
	  */
	  //////////////////////////////////////////////////////////////////////////////////////
	  hits_per_layer.at(layeridx) += 1;
	  en_per_layer.at(layeridx).push_back( weights[i] );
	  rhos_per_layer.at(layeridx).push_back( rhos[i] );
	  deltas_per_layer.at(layeridx).push_back( deltas[i] );
	  seeds_per_layer.at(layeridx).push_back( seeds[i] );
	  xpos_per_layer.at(layeridx).push_back( xpos[i] );
	  ypos_per_layer.at(layeridx).push_back( ypos[i] );
	  cluster_size_per_layer.at(layeridx).push_back( nhitsincluster[i] );
	}
      //Note: We should get an out-of-bounds error for trying to access info at layers > 28. 
      //      It does not happen since all hits not in the CEE are marked as outliers by CLUE (clusterid == -1).
    }
  //fill std::arra1y with fractions
  for(unsigned ilayer=0; ilayer<lmax; ++ilayer) {
    layerdep_vars_.at(ilayer) = std::make_tuple(hits_per_layer.at(ilayer), en_per_layer.at(ilayer), 
						rhos_per_layer.at(ilayer), deltas_per_layer.at(ilayer), seeds_per_layer.at(ilayer),
						xpos_per_layer.at(ilayer), ypos_per_layer.at(ilayer),
						cluster_size_per_layer.at(ilayer));
  }
}

//calculate the number of clusterized hits and clusterized energy per layer and per cluster
void CLUEAnalysis::calculateClusterDepVars(const std::vector<float>& xpos, const std::vector<float>& ypos, const std::vector<float>& weights, const std::vector<int>& clusterid, const std::vector<int>& layerid, const std::vector<float>& impactX, const std::vector<float>& impactY) {
  assert(!weights.empty() && !clusterid.empty() && !layerid.empty());

  std::vector<unsigned> nclusters_per_layer(this->lmax, 0); //number of clusters per layer for resizing the vectors
  std::unordered_map<unsigned, unsigned> clusterIndexMap;

  //calculate number of clusters per layer
  for(auto i: util::lang::indices(weights))
    {
      if(clusterid[i]!=-1 /*outliers are not considered*/ and 
	 clusterIndexMap.find(clusterid[i]) == clusterIndexMap.end()) /*only once per cluster ID*/
	{
	  unsigned layeridx = static_cast<unsigned>(layerid[i]) - 1; //layers start at 1
	  clusterIndexMap.emplace(clusterid[i], nclusters_per_layer[layeridx]);
	  nclusters_per_layer[layeridx] += 1;
	}
    }
  std::vector< std::vector<float> > en_per_cluster(this->lmax);
  std::vector< std::vector<float> > en_per_cluster_ecut(this->lmax); //helper for position measurement
  std::vector< std::vector<float> > en_per_cluster_log_ecut(this->lmax); //helper for position measurement
  std::vector< std::vector<unsigned> > hits_per_cluster(this->lmax);
  std::vector< std::vector<unsigned> > idx_highest_en(this->lmax); //helper for position measurement (distance cut)
  std::vector< std::vector<float> > x_per_cluster(this->lmax);
  std::vector< std::vector<float> > y_per_cluster(this->lmax);
  for(unsigned i=0; i<lmax; ++i)
    {
      en_per_cluster[i].resize( nclusters_per_layer[i], 0.f ); 
      en_per_cluster_ecut[i].resize( nclusters_per_layer[i], 0.f ); 
      en_per_cluster_log_ecut[i].resize( nclusters_per_layer[i], 0.f );
      hits_per_cluster[i].resize( nclusters_per_layer[i], 0 );
      idx_highest_en[i].resize( nclusters_per_layer[i], 9999 ); //there are no clusters with these many hits!
      x_per_cluster[i].resize( nclusters_per_layer[i], 0.f ); 
      y_per_cluster[i].resize( nclusters_per_layer[i], 0.f );
    }

  for(auto i: util::lang::indices(weights)) {
    if(clusterid[i] != -1) { //outliers are not considered
      unsigned layeridx = static_cast<unsigned>(layerid[i]) - 1; //layers start at 1
      unsigned vectoridx = clusterIndexMap[clusterid[i]];
      en_per_cluster.at(layeridx).at(vectoridx) += weights[i];
      hits_per_cluster.at(layeridx).at(vectoridx) += 1;
      
      //avoid repeating loops over the same cluster
      if(idx_highest_en[layeridx][vectoridx] != 9999 )
	continue;

      //find the index of the seed in the same 2D cluster
      for(auto j: util::lang::indices(weights)) 
	{
	  unsigned lidx = static_cast<unsigned>(layerid[j]) - 1;
	  unsigned vidx = clusterIndexMap[clusterid[j]];
	  if(lidx == layeridx and vidx == vectoridx) //hit in the same 2D cluster
	    {
	      if(weights[j] >= weights[i]) //it also works if we are comparing the same hit
		idx_highest_en[layeridx][vectoridx] = j;
	    }
	}
    }
  }

  for(auto i: util::lang::indices(weights)) {
     if(clusterid[i] != -1)  //outliers are not considered
       {
	 unsigned layeridx = static_cast<unsigned>(layerid[i]) - 1; //layers start at 1
	 unsigned vectoridx = clusterIndexMap[clusterid[i]];
	 unsigned maxenid = idx_highest_en[layeridx][vectoridx];
	 if( hit_distance(xpos[i],xpos[maxenid],ypos[i],ypos[maxenid])<1.3 ) //a radius of 13mm is imposed
	   en_per_cluster_ecut.at(layeridx).at(vectoridx) += weights[i];
       }
    }

   for (auto i: util::lang::indices(weights)) {
     if(clusterid[i] != -1)  //outliers are not considered
       {
	 unsigned layeridx = static_cast<unsigned>(layerid[i]) - 1; //layers start at 1
	 unsigned vectoridx = clusterIndexMap[clusterid[i]];
	 unsigned maxenid = idx_highest_en[layeridx][vectoridx];
	 if( hit_distance(xpos[i],xpos[maxenid],ypos[i],ypos[maxenid])<1.3 ) //a radius of 13mm is imposed
	   {
	     float Wi = std::max(W0_ + std::log(weights[i] / en_per_cluster_ecut[layeridx][vectoridx]), 0.f);
	     x_per_cluster.at(layeridx).at(vectoridx) += xpos[i] * Wi;
	     y_per_cluster[layeridx][vectoridx] += ypos[i] * Wi;
	     en_per_cluster_log_ecut.at(layeridx).at(vectoridx) += Wi;
	   }
       }
   }

   for(unsigned s1=0; s1<en_per_cluster_log_ecut.size(); ++s1) {
     for(unsigned s2=0; s2<en_per_cluster_log_ecut[s1].size(); ++s2)
       {
	 if (en_per_cluster_log_ecut.at(s1).at(s2) != 0.)
	   {
	     float inv_log = 1.f / en_per_cluster_log_ecut.at(s1).at(s2);
	     x_per_cluster.at(s1).at(s2) *= inv_log;
	     y_per_cluster.at(s1).at(s2) *= inv_log;
	   }
	 else
	   {
	     x_per_cluster.at(s1).at(s2) = -99.f;
	     y_per_cluster.at(s1).at(s2) = -99.f;
	   }
       }
   }

  //fill std::array with clusterized cluster number of hits and energy
  //both vectors might well be empty, in case there was no cluster in a particular layer
  for(unsigned ilayer=0; ilayer<lmax; ++ilayer)
    {
      //spatial resolution calculation (estimated impact point in layer minus CLUE's position of each cluster)
      assert( x_per_cluster[ilayer].size() == y_per_cluster[ilayer].size() );
      std::vector<float> dx(x_per_cluster[ilayer].size()), dy(x_per_cluster[ilayer].size());
      for(unsigned iclust=0; iclust<x_per_cluster[ilayer].size(); ++iclust) 
	{
	  dx[iclust] = impactX[ilayer] - x_per_cluster[ilayer][iclust];
	  dy[iclust] = impactY[ilayer] - y_per_cluster[ilayer][iclust];
	}
      //store all cluster-related variables
      clusterdep_vars_.at(ilayer) = std::make_tuple( hits_per_cluster[ilayer], en_per_cluster[ilayer], x_per_cluster[ilayer], y_per_cluster[ilayer], dx, dy);
    }
}

//Returns quantities of interest of individual clusters
std::vector<dataformats::data> CLUEAnalysis::getTotalPositionsAndEnergyOutput(std::string& outputFileName, bool verbose) {
  bool pos_set = !pos_.empty(), en_set = !en_.empty();

  if(verbose) 
    {
      std::ofstream oFile(outputFileName, std::ios::out);
      if(pos_set && en_set)
	assert(en_.size() == pos_.size());
      oFile << "###Cluster Analysis: individual clusters (posx, posy, posz, energy)\n";
      for(unsigned i=0; i<pos_.size(); ++i)
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
float CLUEAnalysis::getTotalEnergyOutput(const std::string& outputFileName, bool verbose) {
  assert( !en_.empty() );
  float toten = std::accumulate(en_.begin()+1, en_.end(), 0.f); //outliers (first item) are skipped!
  if(verbose) {
    std::ofstream oFile(outputFileName, std::ios::out);
    oFile << "###Cluster Analysis: total energy\n";
    oFile << toten << std::endl;
    oFile.close();
  }
  return toten;
}

//Returns the number of clusterized hits and clusterized energy per layer
dataformats::layervars CLUEAnalysis::getTotalLayerDepOutput() {
  return this->layerdep_vars_;
}

//Returns the number of clusterized hits and clusterized energy per layer and per cluster
dataformats::clustervars CLUEAnalysis::getTotalClusterDepOutput() {
  return this->clusterdep_vars_;
}

 float CLUEAnalysis::hit_distance(const float& x1, const float& x2, const float& y1, const float& y2)
 {
   return std::sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
 }
