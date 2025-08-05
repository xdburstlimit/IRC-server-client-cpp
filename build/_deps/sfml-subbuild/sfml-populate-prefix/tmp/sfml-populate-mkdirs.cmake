# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/mnt/8f40a6b9-a287-4d21-83ba-402bfa4a171b/C++/Network/Project/build/_deps/sfml-src"
  "/mnt/8f40a6b9-a287-4d21-83ba-402bfa4a171b/C++/Network/Project/build/_deps/sfml-build"
  "/mnt/8f40a6b9-a287-4d21-83ba-402bfa4a171b/C++/Network/Project/build/_deps/sfml-subbuild/sfml-populate-prefix"
  "/mnt/8f40a6b9-a287-4d21-83ba-402bfa4a171b/C++/Network/Project/build/_deps/sfml-subbuild/sfml-populate-prefix/tmp"
  "/mnt/8f40a6b9-a287-4d21-83ba-402bfa4a171b/C++/Network/Project/build/_deps/sfml-subbuild/sfml-populate-prefix/src/sfml-populate-stamp"
  "/mnt/8f40a6b9-a287-4d21-83ba-402bfa4a171b/C++/Network/Project/build/_deps/sfml-subbuild/sfml-populate-prefix/src"
  "/mnt/8f40a6b9-a287-4d21-83ba-402bfa4a171b/C++/Network/Project/build/_deps/sfml-subbuild/sfml-populate-prefix/src/sfml-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/mnt/8f40a6b9-a287-4d21-83ba-402bfa4a171b/C++/Network/Project/build/_deps/sfml-subbuild/sfml-populate-prefix/src/sfml-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/mnt/8f40a6b9-a287-4d21-83ba-402bfa4a171b/C++/Network/Project/build/_deps/sfml-subbuild/sfml-populate-prefix/src/sfml-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
