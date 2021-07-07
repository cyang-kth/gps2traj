// Author: Can Yang
// Email : cyang@kth.se

#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <ctime>
#include <sys/stat.h>
#include <getopt.h>
#include <chrono>

// Data types

struct Point {
  double x;
  double y;
  double timestamp;
};

double string2timestamp(const std::string &intermediate,
  int tag){
  if (tag == 0){
    // double value as timestamp
    return std::stod(intermediate);
  } else if (tag==1) {
    // 2020-01-01T00:00:27
    std::tm tm = {};
    strptime(intermediate.c_str(), "%Y-%m-%dT%H:%M:%S", &tm);
    // std::cout<<"Debug "<< intermediate << "" << std::mktime(&tm) <<"\n";
    return std::mktime(&tm);
  }
  return 0;
};

bool point_comp(Point &p1,Point &p2) {
  return (p1.timestamp<p2.timestamp);
};

struct Trajectory {
  std::string id;
  std::vector<Point> geom;
};

typedef std::unordered_map<std::string,int> TrajIDMap;

typedef std::vector<Trajectory> DataStore;

// struct OutputConfig {
//   bool write_timestamp=false;
//   bool write_index=true;
// };

struct InputConfig {
  std::string id_name;
  std::string x_name;
  std::string y_name;
  std::string timestamp_name;
  int id_idx;
  int x_idx;
  int y_idx;
  int timestamp_idx;
  char delim;
  bool header;
  int time_format;
};

struct OutputConfig {
  bool write_ts=false;
  bool write_tend=false;
  bool write_timestamp=false;
};



void parse_ofields(OutputConfig &config, const std::string &str){
  char delim = ',';
  std::unordered_set<std::string> fields;
  std::stringstream ss(str);
  std::string intermediate;
  while (getline(ss, intermediate, delim)) {
    fields.insert(intermediate);
  }
  if (fields.find("ts") != fields.end()) {
    config.write_ts = true;
  }
  if (fields.find("tend") != fields.end()) {
    config.write_tend = true;
  }
  if (fields.find("timestamp") != fields.end()) {
    config.write_timestamp = true;
  }
};

// Functions to manipulate trajectories

// void append_gps_trajectory(Trajectory &traj, Point &p){
//   traj.geom.push_back(p);
// };

void append_point(DataStore &ds, TrajIDMap &id_map, std::string &traj_id,
                  Point &p){
  // Get the current index
  auto search = id_map.find(traj_id);
  // Search for id
  if (search != id_map.end()) {
    // A trajectory exists already
    int idx =  search->second;
    ds[idx].geom.push_back(p);
    // ds[idx].geom.push_back(p);
  } else {
    // A new node is found, how to ensure that the ds is sorted
    int idx = id_map.size();
    id_map.insert({traj_id, idx});
    ds.push_back(Trajectory());
    ds[idx].id = traj_id;
    ds[idx].geom.push_back(p);
  };
};

void read_header_config(InputConfig &config){
  config.id_idx = std::atoi(config.id_name.c_str());
  config.x_idx = std::atoi(config.x_name.c_str());
  config.y_idx = std::atoi(config.y_name.c_str());
  config.timestamp_idx = std::atoi(config.timestamp_name.c_str());
  std::cout<<"    Id index "<< config.id_idx<<"\n";
  std::cout<<"    X index "<< config.x_idx<<"\n";
  std::cout<<"    Y index "<< config.y_idx<<"\n";
  std::cout<<"    Timestamp index "<< config.timestamp_idx<<"\n";
};

void read_header_config(const std::string &row, InputConfig &config){
  int i = 0;
  std::string intermediate;
  int id_idx = -1;
  int x_idx = -1;
  int y_idx = -1;
  int timestamp_idx = -1;
  std::stringstream check1(row);
  while (std::getline(check1, intermediate, config.delim)) {
    if (intermediate == config.id_name) {
      id_idx = i;
    }
    if (intermediate == config.x_name) {
      x_idx = i;
    }
    if (intermediate == config.y_name) {
      y_idx = i;
    }
    if (intermediate == config.timestamp_name) {
      timestamp_idx = i;
    }
    ++i;
  }
  if (id_idx < 0 || x_idx < 0 || y_idx < 0 || timestamp_idx<0) {
    if (id_idx < 0) {
      std::cout<<"    Id column "<< config.id_name << "not found\n";
    }
    if (x_idx < 0) {
      std::cout<<"    X column "<< config.x_name << "not found\n";
    }
    if (y_idx < 0) {
      std::cout<<"    Y column "<< config.y_name << "not found\n";
    }
    if (timestamp_idx < 0) {
      std::cout<<"    Timestamp column "<< config.timestamp_name
               << "not found\n";
    }
    std::exit(EXIT_FAILURE);
  }
  config.id_idx = id_idx;
  config.x_idx = x_idx;
  config.y_idx = y_idx;
  config.timestamp_idx = timestamp_idx;
  std::cout<<"    Id index "<< id_idx<<"\n";
  std::cout<<"    X index "<< x_idx<<"\n";
  std::cout<<"    Y index "<< y_idx<<"\n";
  std::cout<<"    Timestamp index "<< timestamp_idx<<"\n";
};

void read_row_to_point(long long row_index, std::string &row,
                       InputConfig &config, std::string &traj_id, Point &p){
  // Parse fields from the input line
  std::stringstream ss(row);
  std::string intermediate;
  double x = 0, y = 0;
  double timestamp = 0;
  int index = 0;
  bool id_parsed = false;
  bool x_parsed = false;
  bool y_parsed = false;
  bool timestamp_parsed = false;
  // std::cout<<"Row "<< row << "\n";
  while (std::getline(ss, intermediate, config.delim)) {
    if (index == config.id_idx) {
      traj_id = intermediate;
      id_parsed = true;
    }
    if (index == config.x_idx) {
      p.x = std::stod(intermediate);
      x_parsed = true;
    }
    if (index == config.y_idx) {
      p.y = std::stod(intermediate);
      y_parsed = true;
    }
    if (index == config.timestamp_idx) {
      // std::cout<<"Timestamp "<< intermediate << "\n";
      p.timestamp = string2timestamp(intermediate,config.time_format);
      timestamp_parsed = true;
    }
    ++index;
  }
  if (!(id_parsed && x_parsed && y_parsed && timestamp_parsed)) {
    std::cout<<"     Error in parsing row " << row_index << " "<< row << "\n";
    std::exit(EXIT_FAILURE);
  }
};

void read_traj_data(std::ifstream &ifs, InputConfig &config,
                    DataStore &ds, TrajIDMap &id_map){
  std::cout<<"    Read gps data\n";
  long long num_traj;
  long long num_point;
  std::string row;
  // skip header
  if (config.header){
    std::getline(ifs, row);
    read_header_config(row, config);
  } else {
    read_header_config(config);
  }
  long long progress = 0;
  // Ensure that the data is sorted in ascending order by time
  while (std::getline(ifs, row)) {
    if (progress%1000000==0) {
      std::cout<<"    Lines read " << progress << "\n";
    }
    Point point;
    std::string traj_id;
    read_row_to_point(progress, row, config, traj_id, point);
    // std::cout << "Point timestamp "<< point.timestamp << "\n";
    append_point(ds, id_map, traj_id, point);
    ++progress;
  }
  std::cout<<"    Read gps data done with lines count "<<progress<<"\n";
};

void sort_trajectory(Trajectory &traj){
  std::sort(traj.geom.begin(), traj.geom.end(),point_comp);
};

void sort_data_store(DataStore &ds){
  // Sort all trajectories according to their time information
  for(auto iter=ds.begin(); iter!=ds.end(); ++iter) {
    sort_trajectory(*iter);
  }
};

void write_part_trip(std::ofstream &ofs,OutputConfig &config,
                     int traj_idx, Trajectory &traj,
                     int start_idx, int end_idx){
  ofs<<traj_idx<<";"<<traj.id<<";";
  ofs<<"LineString(";
  for (int i = start_idx; i<=end_idx; ++i) {
    ofs<<traj.geom[i].x<<" "<<traj.geom[i].y<<(i==end_idx ? "" : ",");
  }
  ofs<<")";
  if (config.write_ts){
    ofs<<";"<<traj.geom[start_idx].timestamp;
  }
  if (config.write_tend){
    ofs<<";"<<traj.geom[end_idx].timestamp;
  }
  if (config.write_timestamp){
    ofs<<";";
    for (int j = start_idx; j<=end_idx; ++j) {
      ofs<<traj.geom[j].timestamp<<(j==end_idx ? "" : ",");
    }
  }
  ofs<<"\n";
};

void write_traj_data(std::ofstream &ofs, OutputConfig &config,
                     DataStore &ds, TrajIDMap &id_map,
                     double time_gap, double dist_gap,
                     long long& num_traj, long long& num_point){
  long long total_id_count = ds.size();
  std::cout<< "    Total distinct id to write " << total_id_count << "\n";
  long long step = total_id_count/10;
  if (step<1) step = 1;
  ofs<<"index;id;geom";
  if (config.write_ts){
    ofs<<";ts";
  }
  if (config.write_tend){
    ofs<<";tend";
  }
  if (config.write_timestamp){
    ofs<<";timestamps";
  }
  ofs<<"\n";
  long long progress = 0;
  for(auto iter=ds.begin(); iter!=ds.end(); ++iter) {
    // write_traj_to_stream(ofs, *iter, time_gap, dist_gap);
    // Write traj to stream
    if (progress%step==0){
      std::cout<<"    Progress "<< progress << " / " << total_id_count << "\n";
    }
    Trajectory &traj = *iter;
    // Iterate current and next point
    int N = traj.geom.size();
    int start_idx = 0;
    int end_idx = 0;
    for (int i=0; i<N-1; ++i) {
      end_idx = i+1;
      double time_diff = traj.geom[i+1].timestamp-traj.geom[i].timestamp;
      double distance = std::sqrt(std::pow(traj.geom[i+1].x-traj.geom[i].x,2)+
                                  std::pow(traj.geom[i+1].y-traj.geom[i].y,2));
      if(time_diff>time_gap || distance>dist_gap) {
        if (end_idx-1>start_idx){
          num_traj+=1;
          num_point+=end_idx-start_idx;
          write_part_trip(ofs, config, num_traj, traj, start_idx, end_idx-1);
        }
        start_idx = end_idx;
      }
    }
    if (end_idx>start_idx){
      num_traj+=1;
      num_point+=end_idx-start_idx+1;
      write_part_trip(ofs, config, num_traj, traj, start_idx, end_idx);
    }
    ++progress;
  }
};

bool check_file_exist(const std::string &filename){
  const char *filename_c_str = filename.c_str();
  struct stat buf;
  if (stat(filename_c_str, &buf) != -1) {
    return true;
  }
  return false;
};

void print_help(){
  std::cout<<"Usage:\n";
  std::cout<<"-i/--input: input gps file\n";
  std::cout<<"-o/--output: output trajectory file\n";
  std::cout<<"-d/--delim: delimiter character (, by default)\n";
  std::cout<<"--id: id column name or index (id by default)\n";
  std::cout<<"-x/--x: x column name or index (x by default)\n";
  std::cout<<"-y/--y: y column name or index (y by default)\n";
  std::cout<<"-t/--time: time column name or index (timestamp by default)\n";
  std::cout<<"-f/--tf: time format(0 for int, 1 for 2020-01-01T00:00:27)\n";
  std::cout<<"--time_gap: time gap to split long trajectory \n";
  std::cout<<"--dist_gap: dist gap to split long trajectory \n";
  std::cout<<"--no_header: if specified, gps file contains no header\n";
  std::cout<<"--ofields: output fields (ts,tend,timestamp) separated by ,\n";
  std::cout<<"-h/--help: print help information\n";
};

int main(int argc, char**argv){
  std::cout<<"gps2traj\n";
  if (argc==1){
    print_help();
    return 0;
  }
  std::string input_file;
  std::string output_file;
  std::string id_name = "id";
  std::string x_name = "x";
  std::string y_name = "y";
  std::string timestamp_name = "timestamp";
  std::string output_fields = "";
  bool header = true;
  char delim = ',';
  int opt;
  double dist_gap=1e9;
  double time_gap=1e9;
  int time_format = 0;
  // The last element of the array has to be filled with zeros.
  static struct option long_options[] =
  {
    {"input",  required_argument,0,'i' },
    {"output",   required_argument,0,'o' },
    {"delim",   required_argument,0,'d' },
    {"id",   required_argument,0, 0},
    {"x",   required_argument,0,'x' },
    {"y",   required_argument,0,'y' },
    {"time",   required_argument,0,'t' },
    {"tf",   required_argument,0, 'f'},
    {"time_gap",   required_argument,0, 0},
    {"ofields",   required_argument,0, 0},
    {"dist_gap",   required_argument,0, 0},
    {"no_header",   no_argument, 0, 0},
    {"help",   no_argument,0,'h' },
    {0,         0,                 0,  0 }
  };
  int long_index =0;
  while ((opt = getopt_long(argc, argv,"i:o:d:0:t:x:y:f:h",
                            long_options, &long_index )) != -1)
  {
    switch (opt)
    {
    case 'i':
      input_file = std::string(optarg);
      break;
    case 'o':
      output_file = std::string(optarg);
      break;
    case 'd':
      delim = *optarg;
      break;
    case 'x':
      x_name = std::string(optarg);
      break;
    case 'y':
      y_name = std::string(optarg);
      break;
    case 't':
      timestamp_name = std::string(optarg);
      break;
    case 'f':
      time_format = std::stoi(optarg);
      break;
    case 'h':
      print_help();
      std::exit(EXIT_SUCCESS);
    case 0:
      if (strcmp(long_options[long_index].name,"id")==0){
        id_name = std::string(optarg);
      }
      // printf("option %s", long_options[long_index].name);
      if (strcmp(long_options[long_index].name,"time_gap")==0){
        time_gap = std::atof(optarg);
      }
      if (strcmp(long_options[long_index].name,"dist_gap")==0){
        dist_gap = std::atof(optarg);
      }
      if (strcmp(long_options[long_index].name,"no_header")==0){
        header = false;
      }
      if (strcmp(long_options[long_index].name,"ofields")==0){
        output_fields = std::string(optarg);
      }
      break;
    default:
      print_help();
      exit(EXIT_FAILURE);
    }
  }
  if (!check_file_exist(input_file))
  {
    std::cout<<"  Error: Input file not found: "<< input_file <<"\n";
    std::exit(EXIT_FAILURE);
  }
  std::cout<<"---- Configurations ----\n";
  std::cout<<"    Input  file: "<<input_file<<"\n";
  std::cout<<"    Output file: "<<output_file<<"\n";
  std::cout<<"    id column name: "<<id_name<<"\n";
  std::cout<<"    x column name: "<<x_name<<"\n";
  std::cout<<"    y column name: "<<y_name<<"\n";
  std::cout<<"    time column name: "<<timestamp_name<<"\n";
  std::cout<<"    column delimter: "<< delim <<"\n";
  std::cout<<"    time format: "<< time_format <<"\n";
  std::cout<<"    header: "<< (header?"true":"false") <<"\n";
  std::cout<<"    ofields: "<< output_fields <<"\n";
  std::cout<<"    time gap: "<< time_gap <<"\n";
  std::cout<<"    dist gap: "<< dist_gap <<"\n";
  auto t1 = std::chrono::high_resolution_clock::now();
  long long num_traj = 0;
  long long num_point = 0;
  DataStore ds;
  TrajIDMap id_map;
  InputConfig input_config{
    id_name,x_name,y_name,timestamp_name,-1,-1,-1,-1,delim, header, time_format
  };
  OutputConfig output_config;
  parse_ofields(output_config, output_fields);
  std::cout<<"---- Reading GPS data ----\n";
  std::ifstream ifs(input_file);
  read_traj_data(ifs, input_config, ds, id_map);
  auto t2 = std::chrono::high_resolution_clock::now();
  auto input_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    t2 - t1 ).count();
  std::cout<<"Reading input takes " << input_duration << " ms\n";
  std::cout<<"---- Sorting points in trajectory ----\n";
  sort_data_store(ds);
  auto t3 = std::chrono::high_resolution_clock::now();
  auto sort_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    t3 - t2 ).count();
  std::cout<<"Sorting points takes " << sort_duration << " ms\n";
  std::cout<<"---- Writing trajectory data ----\n";
  std::ofstream ofs(output_file);
  ofs.precision(12);
  write_traj_data(ofs, output_config, ds, id_map, time_gap, dist_gap,
    num_traj, num_point);
  auto t4 = std::chrono::high_resolution_clock::now();
  auto write_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    t4 - t3 ).count();
  std::cout<<"Write output takes " << write_duration << " ms\n";
  std::cout<<"---- gps2traj statistcs ----\n";
  std::cout<<"    Distinct ids "<< id_map.size() <<"\n";
  std::cout<<"    Number of trips "<< num_traj <<"\n";
  std::cout<<"    Number of points "<< num_point <<"\n";
  auto t5 = std::chrono::high_resolution_clock::now();
  auto whole_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    t5 - t1 ).count();
  std::cout<<"gps2traj finish in " << whole_duration <<" ms \n";
};
