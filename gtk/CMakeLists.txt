include(FindPkgConfig)
pkg_check_modules(GLIB2 IMPORTED_TARGET glib-2.0)
pkg_check_modules(GTK4 IMPORTED_TARGET gtk4)
pkg_check_modules(PANGO IMPORTED_TARGET pango)
pkg_check_modules(PANGOFT2 IMPORTED_TARGET pangoft2)
add_executable(sbash64-budget-gtk main.cpp)
target_link_libraries(
  sbash64-budget-gtk PRIVATE PkgConfig::GTK4 PkgConfig::GLIB2 PkgConfig::PANGO
                             PkgConfig::PANGOFT2)
