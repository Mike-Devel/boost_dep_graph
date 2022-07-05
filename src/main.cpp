
#include <ui/FileListDisplay.hpp>
#include <ui/control.hpp>
#include <ui/graphwidget.hpp>

#include <core/ModuleInfo.hpp>
#include <core/analysis.hpp>
#include <core/boostdep.hpp>

#include <QListView>
#include <QPushButton>
#include <QTableView>
#include <QTreeView>

#include <QApplication>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QProcessEnvironment>
#include <QSplitter>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

using namespace mdev;
using namespace mdev::bdg;

// This is the list of boost libraries that is ignored in the following process
// TODO: make this changeable at runtime
[[maybe_unused]] const std::vector<std::string> filter_wo_serialization{"serialization"};

[[maybe_unused]] const std::vector<std::string> filter_cpp_20{
	"function",     "assert",    "static_assert", "optional", "variant",
	"mpl",          "smart_ptr", "array",         "tuple",    "iterator",
	"type_traits",  "move",      "atomic",        "bind",     "lambda",
	"chrono",       "date_time", "random",        "thread",   "throw_exception",
	"preprocessor", "detail",    "typeof",        "any",      "type_index",
	"align",        "ratio",     "compatibility", "foreach",  "assign",
	"range",        "system",    "regex",         "variant2", "coroutine",
	"coroutine2",   "filesystem"};

const std::vector<std::string> filter = {};
//= filter_wo_serialization; //= filter_wo_serialization; //= filter_cpp_20; // modules that should be ignored

std::vector<boostdep::FileInfo> cmake_files_only( std::vector<boostdep::FileInfo> files )
{
	files.erase(
		std::remove_if( files.begin(),
						files.end(),
						[]( const boostdep::FileInfo& f ) { return f.category != boostdep::FileCategory::CMake; } ),
		files.end() );

	return std::move( files );
}

std::vector<boostdep::FileInfo> wo_cmake_files( std::vector<boostdep::FileInfo> files )
{
	files.erase(
		std::remove_if( files.begin(),
						files.end(),
						[]( const boostdep::FileInfo& f ) { return f.category == boostdep::FileCategory::CMake; } ),
		files.end() );

	return std::move( files );
}

int main( int argc, char** argv )
{
	QApplication app( argc, argv );

	auto graph_widget = new gui::GraphWidget();

	// This data is referenced from multiple places in the UI, so it has to stay alive as long as the app is running
	std::vector<boostdep::FileInfo> file_infos;
	modules_data                    modules;
	std::filesystem::path           boost_root;

	auto rescan = [&file_infos, &boost_root]() {
		boost_root = determine_boost_root();
		file_infos = boostdep::scan_all_boost_modules(
			boost_root, boostdep::TrackSources::Yes, boostdep::TrackTests::No, boostdep::TrackCMake::Yes );
	};

	auto redo_analysis = [&modules, &graph_widget, &file_infos, &boost_root] {
		{
			auto cmake_dep  = build_module_dependency_map( cmake_files_only( file_infos ) );
			auto module_dep = build_module_dependency_map( wo_cmake_files( file_infos ) );

			cmake_dep.at( "iterator" ).push_back( "hello" );

			for( auto& [m, deps] : module_dep ) {
				auto& cmd = cmake_dep[m];
				if( cmd.empty() ) {
					continue;
				}
				std::sort( deps.begin(), deps.end() );
				std::sort( cmd.begin(), cmd.end() );

				if( !std::equal( deps.begin(), deps.end(), cmd.begin(), cmd.end() ) ) {
					std::cout << "Dependency difference for module " << m << ":" << std::endl;
					std::cout << "FileDeps:  ";
					for( const auto& d : deps ) {
						std::cout << d << " ";
					}
					std::cout << std::endl << "CMakeDeps: ";
					for( const auto& d : cmd ) {
						std::cout << d << " ";
					}
					std::cout << std::endl;
				}
			}
		}

		auto root_lib = get_root_library_name();
		modules       = generate_module_list( wo_cmake_files( file_infos ), boost_root, root_lib, filter );
		// modules       = generate_file_list( file_infos, boost_root, root_lib.value(), filter );
		graph_widget->set_data( &modules );
	};

	auto print_stats  = [&] { print_cmake_stats( modules ); };
	auto print_cycles = [&] {
		std::cout << "\n##############################\n"
					 "Dedected Cycles:\n\n";
		for( auto& c : cycles( modules ) ) {
			for( auto& e : c ) {
				std::cout << e << " ";
			}
			std::cout << '\n';
		}
		std::cout << "\n##############################\n" << std::endl;
	};
	auto rescanfull = [&] {
		rescan();
		redo_analysis();
		print_stats();
		print_cycles();
	};

	rescanfull();

	QMainWindow main_window;

	DisplayFileList     list( &file_infos );
	TreeDisplayFileList treelist( &file_infos );

	QTableView*        tableview = new QTableView();
	QAbstractItemView* treeview  = new QTreeView();
	// QListView*   listview = new QListView();
	tableview->setModel( &list );
	treeview->setModel( &treelist );
	// listview->setModelColumn( 1 );

	QSplitter* layout = new QSplitter();
	layout->addWidget( graph_widget );
	// layout->addWidget( tableview );
	// layout->addWidget( treeview );

	main_window.setCentralWidget( layout );

	QObject::connect( graph_widget, &gui::GraphWidget::reload_requested, rescanfull );
	QObject::connect( graph_widget, &gui::GraphWidget::reset_requested, redo_analysis );
	QObject::connect( graph_widget, &gui::GraphWidget::reprint_stats_requested, print_stats );
	QObject::connect( graph_widget, &gui::GraphWidget::cycle_dedection_requested, print_cycles );
	main_window.show();

	// list.update();
	return app.exec();
}