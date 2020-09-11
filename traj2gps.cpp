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
#include <sys/stat.h>
#include <getopt.h>
#include <chrono>
#include <ctype.h>

// Data types

struct Point {
  double x;
  double y;
};

struct Trajectory {
  std::string id;
  std::vector<Point> points;
};

struct InputConfig {
  std::string id_name="id";
  int id_idx;
  std::string geom_name="geom";
  int geom_idx;
  char delim = ';';
  bool header = true;
  std::string input_file;
  std::string output_file;
};

void read_header_config(InputConfig &config){
  config.id_idx = std::atoi(config.id_name.c_str());
  config.geom_idx = std::atoi(config.geom_name.c_str());
  std::cout<<"    Id index "<< config.id_idx<<"\n";
  std::cout<<"    Geom index "<< config.geom_idx<<"\n";
};

void read_header_config(const std::string &row, InputConfig &config){
  int i = 0;
  std::string intermediate;
  int id_idx = -1;
  int geom_idx = -1;
  std::stringstream check1(row);
  while (std::getline(check1, intermediate, config.delim)) {
    if (intermediate == config.id_name) {
      id_idx = i;
    }
    if (intermediate == config.geom_name) {
      geom_idx = i;
    }
    ++i;
  }
  if (id_idx < 0 || geom_idx < 0) {
    if (id_idx < 0) {
      std::cout<<"    Id column "<< config.id_name << "not found\n";
    }
    if (geom_idx < 0) {
      std::cout<<"    geom column "<< config.geom_name << "not found\n";
    }
    std::exit(EXIT_FAILURE);
  }
  config.id_idx = id_idx;
  config.geom_idx = geom_idx;
  std::cout<<"    Id index "<< id_idx<<"\n";
  std::cout<<"    Geom index "<< geom_idx<<"\n";
};

bool iequals(const std::string& a, const std::string& b)
{
  unsigned int sz = a.size();
  if (b.size() != sz)
    return false;
  for (unsigned int i = 0; i < sz; ++i)
    if (tolower(a[i]) != tolower(b[i]))
      return false;
  return true;
};

std::vector<Point> wkt2traj(const std::string &str){
  std::vector<Point> pts;
  std::stringstream stringStream(str);
  std::string line;
  std::vector<std::string> tokens;
  while(std::getline(stringStream, line))
  {
    std::size_t prev = 0, pos;
    while ((pos = line.find_first_of("() ,", prev)) != std::string::npos)
    {
      if (pos > prev)
        tokens.push_back(line.substr(prev, pos-prev));
      prev = pos+1;
    }
    if (prev < line.length())
      tokens.push_back(line.substr(prev, std::string::npos));
  }
  // Iterate to parse data
  auto iter = tokens.begin();
  if (iter != tokens.end() &&
      iequals(*iter, "LINESTRING"))
  {
    ++iter;
  }
  else
  {
    std::cout<<"Error, geom field should start with LINESTRING "<<str<<"\n";
    std::exit(EXIT_FAILURE);
  }
  while (iter!=tokens.end()) {
    double x = std::stod(*iter);
    ++iter;
    if (iter!=tokens.end()) {
      double y = std::stod(*iter);
      pts.push_back(Point{x,y});
      ++iter;
    } else {
      std::cout<<"Error, uneven number of pts "<<str<<"\n";
      std::exit(EXIT_FAILURE);
    }
  }
  return pts;
};

void read_row_to_trajectory(long long row_index, std::string &row,
                            InputConfig &config, Trajectory &traj){
  // Parse fields from the input line
  std::stringstream ss(row);
  std::string intermediate;
  int index = 0;
  bool id_parsed = false;
  bool geom_parsed = false;
  // std::cout<<"Row "<< row << "\n";
  while (std::getline(ss, intermediate, config.delim)) {
    if (index == config.id_idx) {
      traj.id = intermediate;
      id_parsed = true;
    }
    if (index == config.geom_idx) {
      traj.points = wkt2traj(intermediate);
      geom_parsed = true;
    }
    ++index;
    if (id_parsed && geom_parsed) break;
  }
  if (!(id_parsed && geom_parsed)) {
    std::cout<<"     Error in parsing row " << row_index << " "<< row << "\n";
    std::exit(EXIT_FAILURE);
  }
};

void write_trajectory(std::ofstream &ofs, Trajectory &traj){
  for (int i = 0; i<traj.points.size(); ++i) {
    ofs<<traj.id<<";"<<i<<";"<<traj.points[i].x<<";"<<traj.points[i].y<<"\n";
  }
};

void traj2gps(InputConfig &config){
  std::cout<<"Read gps data\n";
  std::ifstream ifs(config.input_file);
  long long num_traj;
  long long num_point;
  std::string row;
  // skip header
  if (config.header) {
    std::getline(ifs, row);
    read_header_config(row, config);
  } else {
    read_header_config(config);
  }
  std::ofstream ofs(config.output_file);
  ofs << "id;point_idx;x;y\n";
  long long progress = 0;
  // Ensure that the data is sorted in ascending order by time
  while (std::getline(ifs, row)) {
    ++progress;
    if (progress%1000000==0) {
      std::cout<<"  Lines read " << progress << "\n";
    }
    Trajectory traj;
    read_row_to_trajectory(progress, row, config, traj);
    // std::cout << "Point timestamp "<< point.timestamp << "\n";
    write_trajectory(ofs,traj);
  }
  std::cout<<"Read gps data done with lines count "<<progress<<"\n";
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
  std::cout<<"-d/--delim: delimiter character (; by default)\n";
  std::cout<<"--id: id column name or index (id by default)\n";
  std::cout<<"-g/--geom: geom column name or index (geom by default)\n";
  std::cout<<"--no_header: if specified, traj file contains no header\n";
  std::cout<<"-h/--help: print help information\n";
};

void parse_arguments(int argc, char **argv, InputConfig &config){
  int opt;
  //int N;
  //double min_sup_value = 1;
  // The last element of the array has to be filled with zeros.
  static struct option long_options[] =
  {
    {"input",      required_argument,0,'i' },
    {"output", required_argument,0,  'o' },
    {"delim", required_argument,0,  'd' },
    {"id", required_argument,0,  'a' },
    {"geom", required_argument,0,  'g' },
    {"no_header", no_argument,0,  'n' },
    {"help",   no_argument,0,'h' },
    {0,         0,                 0,  0 }
  };

  // An option followed by a colon only means that it needs
  // an argument. It doesn't mean that the option is enforced.
  // You should write your own code to enforce the existence of
  // options/arguments
  int long_index =0;
  while ((opt = getopt_long(argc, argv,"i:o:d:a:g:nh",
                            long_options, &long_index )) != -1)
  {
    switch (opt)
    {
    case 'i':
      config.input_file = std::string(optarg);
      break;
    case 'o':
      config.output_file = std::string(optarg);
      break;
    case 'g':
      config.geom_name = std::string(optarg);
      break;
    case 'a':
      config.id_name = std::string(optarg);
      break;
    case 'd':
      config.delim = *optarg;
      break;
    case 'n':
      config.header = false;
      break;
    case 'h':
      std::cout<<"Help information:"<<std::endl;
      print_help();
      exit(EXIT_SUCCESS);
    default:
      print_help();
      std::cout<<"Unrecognized options"<<std::endl;
      exit(EXIT_FAILURE);
    }
  }
  std::cout<<"Parameters read in finished"<<std::endl;
  // strtok requires input to be a char *, so we need to copy it
  if (!check_file_exist(config.input_file)) {
    std::cout<<"Error: input file not found: " <<config.input_file<<std::endl;
    // print_help();
    exit(EXIT_FAILURE);
  }
  std::cout<<"\n---Configurations---\n"<<std::endl;
  std::cout<<"Input file: "<<config.input_file<<std::endl;
  std::cout<<"Output file: "<< config.output_file <<std::endl;
  std::cout<<"Field delim: "<< config.delim <<std::endl;
  std::cout<<"Id name/index: "<< config.id_name <<std::endl;
  std::cout<<"Geom name/index: "<< config.geom_name <<std::endl;
  std::cout<<"Header: "<< (config.header ? "true" : "false") <<std::endl;
};

int main(int argc, char**argv){
  std::cout<<"traj2gps\n";
  if (argc==1) {
    print_help();
    return 0;
  }
  InputConfig config;
  parse_arguments(argc,argv,config);
  auto t1 = std::chrono::high_resolution_clock::now();
  traj2gps(config);
  auto t2 = std::chrono::high_resolution_clock::now();
  auto whole_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    t2 - t1 ).count();
  std::cout<<"traj2gps finish in " << whole_duration <<" ms \n";
};
