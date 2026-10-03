cmake_minimum_required(VERSION 3.16)

project(nnis-qt
        VERSION 1.0
        LANGUAGES CXX
        DESCRIPTION "Portage Qt/C++17 du projet MFC nis (classification d'images par graphes de minuties)")

# ---------------------------------------------------------------------------
# Regles generales
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Mode de compilation" FORCE)
endif()

option(NIS_BUILD_UI    "Construire l'interface graphique Qt Widgets" ON)
option(NIS_BUILD_TESTS "Construire les tests unitaires du noyau"     ON)
option(NIS_SANITIZE    "Active les sanitizers Address et Undefined"  OFF)

add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)

if(NIS_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=address,undefined)
endif()

# ---------------------------------------------------------------------------
# Noyau de calcul : volontairement independant de Qt (testable sans ecran)
# ---------------------------------------------------------------------------
file(GLOB NIS_CORE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp")

add_library(nis_core STATIC ${NIS_CORE_SOURCES})
target_include_directories(nis_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

find_package(Threads QUIET)
if(TARGET Threads::Threads)
  target_link_libraries(nis_core PUBLIC Threads::Threads)
endif()

# ---------------------------------------------------------------------------
# Interface Qt (Qt6 ou Qt5, le premier trouve gagne).  Le code source est
# ecrit pour compiler avec les deux.
# ---------------------------------------------------------------------------
if(NIS_BUILD_UI)
  find_package(Qt6 COMPONENTS Widgets QUIET)
  set(NIS_QT_VERSION_MAJOR 6)
  if(NOT Qt6_FOUND)
    find_package(Qt5 5.12 COMPONENTS Widgets QUIET)
    set(NIS_QT_VERSION_MAJOR 5)
  endif()

  if(Qt${NIS_QT_VERSION_MAJOR}_FOUND)
    message(STATUS "Portage nis : Qt${NIS_QT_VERSION_MAJOR} trouve, construction de l'interface")
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    file(GLOB NIS_UI_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/qt/*.cpp")
    if(NOT NIS_UI_SOURCES)
      message(FATAL_ERROR
        "Portage nis : Qt${NIS_QT_VERSION_MAJOR} trouve mais aucun fichier dans src/qt/. "
        "Le portage de l'interface est incomplet : reconstruisez apres avoir ajoute les sources.")
    endif()
    if(WIN32)
      add_executable(nis WIN32 ${NIS_UI_SOURCES})
    else()
      add_executable(nis ${NIS_UI_SOURCES})
    endif()
    target_compile_definitions(nis PRIVATE NIS_QT_VERSION_MAJOR=${NIS_QT_VERSION_MAJOR})
    target_include_directories(nis PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/qt")
    target_link_libraries(nis PRIVATE nis_core Qt${NIS_QT_VERSION_MAJOR}::Widgets)
  else()
    message(WARNING
      "NIS_BUILD_UI est active mais aucune version de Qt (Qt6 ou Qt5 >= 5.12) n'a ete trouvee. "
      "Le noyau et les tests sont quand meme construits. "
      "Installation Linux : sudo apt-get install -y qtbase5-dev qtbase5-dev-tools")
  endif()
endif()

# ---------------------------------------------------------------------------
# Tests du noyau (framework maison minmal, aucune dependance externe)
# ---------------------------------------------------------------------------
if(NIS_BUILD_TESTS)
  enable_testing()
  file(GLOB NIS_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
  add_executable(nis_tests ${NIS_TEST_SOURCES})
  target_include_directories(nis_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  target_link_libraries(nis_tests PRIVATE nis_core)
  add_test(NAME noyau COMMAND nis_tests)
endif()
