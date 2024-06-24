#include <args.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

boost::program_options::variables_map parse_program_options(int const argc,
                                                            char **const argv) {
  // Declare a group of options that will be
  // allowed only on command line
  boost::program_options::options_description generic("Allowed options");
  generic.add_options()
      // clang-format off
  ("version,v", "print version string")
  ("help", "produce help message")
  ("inspect", "inspect config entries(musics, playlists)");
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
