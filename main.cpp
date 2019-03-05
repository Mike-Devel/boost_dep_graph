
#include <ui/graphwidget.hpp>

#include <core/ModuleInfo.hpp>
#include <core/analysis.hpp>

#include <QApplication>
#include <QFileDialog>
#include <QInputDialog>
#include <QMainWindow>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QString>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <vector>

using namespace mdev;
using namespace mdev::bdg;

void print_stats( const modules_data& modules )
{
	std::vector<const ModuleInfo*> modules_sorted_by_dep_count = get_modules_sorted_by_dep_count( modules );

	int count = 0;
	std::cout <<"Total Rev Dep cnt" << "/" << "without cmake" << "/" << "blocked" << "\t" << "name" << "\n";
	for( auto m : modules_sorted_by_dep_count ) {
		if( !m->has_cmake ) {
			count++;
			auto ncdpcnt = std::count_if(
				m->all_rev_deps.begin(), m->all_rev_deps.end(), []( const auto& p ) { return !p->has_cmake; } );
			auto blocked_cnt = block_count( *m );
			std::cout << m->all_rev_deps.size() << "/" << ncdpcnt << "/" << blocked_cnt << "\t" << m->name << "\n";
		}
	}
	std::cout << "Modules without a cmake file: " << count << std::endl;
}

// This is the list of boost libraries that is ignored in the following process

std::ostream& operator<<( std::ostream& stream, const QString& str )
{
	stream << str.toStdString();
	return stream;
}

std::filesystem::path determine_boost_root()
{

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

	auto default_folder = env.value( "BOOST_ROOT", "" );

	if( default_folder.isEmpty() ) {
		std::cout << "No BOOST_ROOT environment variable found" << std::endl;
		default_folder = QStandardPaths::standardLocations( QStandardPaths::HomeLocation ).first();
	}

	std::filesystem::path boost_root
		= QFileDialog::getExistingDirectory( nullptr, "Select boost root directory", default_folder ).toStdString();

	if( !std::filesystem::exists( boost_root / "Jamroot" ) ) {
		std::cerr << "Could not detect Jamroot file in selected folder :" << boost_root << std::endl;
		std::exit( 1 );
	} else {
		std::cout << "Selected boost installation: " << boost_root << std::endl;
	}

	return boost_root;
}

const std::vector<std::string> filter; // Add modules that should be ignored

int main( int argc, char** argv )
{
	QApplication app( argc, argv );
	QMainWindow main_window;
	//QMainWindow main_window2;

	auto graph_widget = new gui::GraphWidget();
	//auto graph_widget2 = new gui::GraphWidget();

	main_window.setCentralWidget( graph_widget );
	//main_window2.setCentralWidget( graph_widget2 );

	// This data is referenced from multiple places in the UI, so it has to stay alive as long as the app is running
	modules_data modules;
	//modules_data modules2;

	auto rescan = [&modules, &graph_widget] {
	//auto rescan = [&modules, &modules2, &graph_widget, &graph_widget2] {
		auto boost_root = determine_boost_root();

		bool    ok = false;
		QString text
			= QInputDialog::getText( nullptr, "Select root library", "RootLib:", QLineEdit::Normal, "none", &ok );
		if( !ok ) {
			exit( 1 );
		}
		if( text.isEmpty() || text == "none" ) {
			modules = generate_module_list( boost_root, filter );
		} else {
			modules = generate_module_list( boost_root, text.toStdString(), filter );
			// modules = generate_file_list( boost_root, "serialization", filter );
		}

		auto cs = cycles( modules );

		//print_stats( modules );
		graph_widget->set_data( &modules );
		//graph_widget2->set_data( &modules2 );
	};

	rescan();

	QObject::connect( graph_widget, &gui::GraphWidget::reload_requested, rescan );
	main_window.show();

	//QObject::connect( graph_widget2, &gui::GraphWidget::reload_requested, rescan );
	//main_window2.show();

	return app.exec();
}
