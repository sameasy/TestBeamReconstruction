#include <iostream>
#include <fstream>
#include <string>
#include "TTree.h"
#include "ROOT/RDataFrame.hxx"
#include "ROOT/RDF/InterfaceUtils.hxx"
#include "UserCode/DataProcessing/interface/range.h"
#include "UserCode/DataProcessing/interface/CLUEAnalysis.h"

using mapT = std::map< std::pair<unsigned,unsigned>, float >;

class Selector {
 public:
  Selector(const std::string&, const std::string&, const std::string&, const std::string&, const int&, std::optional<std::string> in_tree_name = std::nullopt, std::optional<std::string> in_tree_name_friend = std::nullopt, std::optional<std::string> out_tree_name = std::nullopt);
  ~Selector();
  void select_relevant_branches();
  void print_relevant_branches(const int&, std::optional<std::string> filename = std::nullopt);

 private:
  int sanity_checks(const std::string&);
  static bool common_selection(const unsigned& layer, const float& energy, const unsigned& chip, const unsigned& channel, const unsigned& module, const float& amplitude, const bool& noise_flag, const mapT& map, const bool& showertype);
  template<typename T> static std::vector<T> clean_arrays(const std::vector<T>&, const std::vector<float>&, const std::vector<unsigned>&, const std::vector<unsigned>&, const std::vector<unsigned>&, const std::vector<unsigned>&, const std::vector<float>&, const std::vector<bool>&, const mapT&, const bool&);
  static std::vector<float> weight_energy(const std::vector<float>&, const std::vector<unsigned>&, const float&, const bool&);
  void load_noise_values();
  static bool reject_noise(const mapT& map, const unsigned& mod, const unsigned& chip, const unsigned& l, const float& amp, const bool& st);
  static std::vector<int> clean_hitK(const int&, const std::vector<int>&);
  
  SHOWERTYPE showertype;
  DATATYPE datatype;
  int beam_energy;
  static const int ncpus_ = 4;
  mapT noise_map_;

  //em showers  
  std::string new_detid_    = "rechit_clean_detid";
  std::string new_x_        = "rechit_clean_x";
  std::string new_y_        = "rechit_clean_y";
  std::string new_z_        = "rechit_clean_z";
  std::string new_layer_    = "rechit_clean_layer";
  std::string new_en_       = "rechit_clean_energy";
  std::string new_en_MeV_   = "rechit_clean_energy_MeV";
  //had showers
  std::string new_ahc_hitK_ = "ahc_clean_hitK";
  //columns to save
  const ROOT::Detail::RDF::ColumnNames_t savedcols_ = {"event", "run", "NRechits", new_detid_, new_x_, new_y_, new_z_, new_layer_, new_en_MeV_, "beamEnergy"};

  struct indata {
    std::string file_path = "/eos/cms/store/group/dpg_hgcal/tb_hgcal/2018/cern_h2_october/offline_analysis/ntuples/v16/ntuple_1000.root";
    std::string tree_name = "rechitntupler/hits";
    std::string tree_name_friend = "trackimpactntupler/impactPoints";
  } indata_;

  struct outdata {
    std::string file_path = "/eos/user/b/bfontana/TestBeamReconstruction/default_output.txt";
    std::string tree_name = "relevant_branches";
  } outdata_;
};
