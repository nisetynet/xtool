#include <args.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unordered_set>

boost::program_options::variables_map parse_program_options(int const argc,
                                                            char **const argv) {
  // Declare a group of options that will be
  // allowed only on command line
  boost::program_options::options_description generic("Allowed options");
  generic.add_options()
      // clang-format off
  ("version,v", "print version string")
  ("help", "produce help message")
  ("config,c",boost::program_options::value<std::string>()->default_value("./config.toml")->notifier([](std::string_view const config_file_path){
std::filesystem::path path(config_file_path);
  if(!std::filesystem::is_regular_file(path)){
    throw invalid_option_error_with_msg(
                  "config", config_file_path,
                  "does not exist or not file");
  }
  })->required(), "config file to use")
  ("inspect", "inspect config entries(musics, playlists)")
  ("test_seed","test rand function and g_mtRand.seed")
  ;

  // clang-format on

  // Hidden options, will be allowed both on command line and
  // in config file, but will not be shown to the user.
  boost::program_options::options_description hidden("Hidden options");
  hidden.add_options();

  boost::program_options::options_description cmdline_options;
  cmdline_options.add(generic).add(hidden);

  // parse argv
  boost::program_options::variables_map vm;
  boost::program_options::store(
      boost::program_options::parse_command_line(argc, argv, cmdline_options),
      vm);

  // check duplication
  [&vm](std::vector<std::string> const &commands) {
    std::vector<std::string> supplied_commands;
    supplied_commands.reserve(commands.size());

    for (auto const &command : commands) {
      if (vm.contains(command)) {
        supplied_commands.push_back(command);
      }
    }
    if (2 <= supplied_commands.size()) {
      throw std::invalid_argument(fmt::format(
          "Got {} commands({}), you can use only one command.",
          supplied_commands.size(), fmt::join(supplied_commands, ", ")));
    }
  }({"help", "version", "inspect", "test_seed"});

  if (vm.count("help")) {
    std::cout << generic << std::endl;
    exit(EXIT_SUCCESS);
  }

  if (vm.count("version")) {
    std::puts("Version 0");
    exit(EXIT_SUCCESS);
  }

  // important to run this after dealt with help, version options
  boost::program_options::notify(vm);

  return vm;
}
