project('xtool', 'cpp', default_options :{
    'cpp_std':'c++23',
    'warning_level':'3',
    'werror':'false',
    'b_lto':'true',
   # 'cpp_args':'-DMA_NO_RUNTIME_LINKING',
    'default_library':'static'
}) # https://mesonbuild.com/Builtin-options.html

# add compiler flags
cpp_compiler = meson.get_compiler('cpp')

# gcc specific options
if cpp_compiler.get_id() == 'gcc'
#add_project_arguments('', language : 'cpp')
endif

# clang specific options
if cpp_compiler.get_id() == 'clang'
add_project_arguments('-Wlifetime', language : 'cpp')
endif

# msvc
if cpp_compiler.get_id() == 'msvc'
    winsock_dep = cpp_compiler.find_library('ws2_32', required: true)
endif



# dependencies
# TODO: replace cpp_args -w with supress when https://github.com/mesonbuild/meson/issues/8636 is done
# https://mesonbuild.com/Dependencies.html#boost
boost_dep = dependency('boost',modules : ['program_options'],include_type:'system',default_options:{'warning_level':'0','cpp_args':'-w'}, required: true, static:true)  # without static:true, boost program_options not found


tomlpp_dep = dependency('tomlplusplus',include_type:'system',default_options:{'warning_level':'0','cpp_args':'-w'},static:true)

fmt_dep = dependency('fmt',include_type:'system',default_options:{'warning_level':'0' ,'cpp_args':'-w'})

spdlog_dep = dependency('spdlog',include_type:'system', default_options:{
     'warning_level':'0',
     'cpp_args':'-w'})

# gtest_dep = dependency('gtest')

include_dir = include_directories('include')

xtool_sources=[
    'src/main.cpp',
    'src/args.cpp',
    'src/playlist.cpp',
    'src/music_player.cpp',
    'src/inspection.cpp',
    'src/dolphin_manager.cpp',
    'src/test_seed.cpp',
    'include/dme/DolphinProcess/Linux/LinuxDolphinProcess.cpp',
    'include/dme/DolphinProcess/Windows/WindowsDolphinProcess.cpp',
    'include/dme/DolphinProcess/DolphinAccessor.cpp',
    'include/dme/Common/MemoryCommon.cpp'
]

if cpp_compiler.get_id() == 'msvc'
    executable('xtool',xtool_sources, include_directories:include_dir,dependencies : [boost_dep,fmt_dep,spdlog_dep,tomlpp_dep,winsock_dep])
else
    executable('xtool',xtool_sources, include_directories:include_dir,dependencies : [boost_dep,fmt_dep,spdlog_dep,tomlpp_dep])
endif