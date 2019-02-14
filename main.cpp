
#include <ui/graphwidget.hpp>
#include <ui/layout.hpp>


#include <core/ModuleInfo.hpp>
#include <core/analysis.hpp>

#include <QApplication>
#include <QFileDialog>
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
	for( auto m : modules_sorted_by_dep_count ) {
		if( !m->has_cmake ) {
			count++;
			auto ncdpcnt = std::count_if(
				m->all_rev_deps.begin(), m->all_rev_deps.end(), []( const auto& p ) { return !p->has_cmake; } );
			std::cout << m->all_rev_deps.size() << "/" << ncdpcnt << "\t" << m->name << "\n";
		}
	}
	std::cout << "Modules without a cmake file: " << count << std::endl;
}

// This is the list of boost libraries that is ignored in the following process


std::ostream& operator<<( std::ostream& stream, const QString& str )
{
	stream << str.toStdString(); // or: stream << str.toStdString(); //??
	return stream;
}

std::filesystem::path determine_boost_root()
{

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

	auto env_boost_root = env.value( "BOOST_ROOT", "" );

	if( env_boost_root.isEmpty() ) {
		std::cout << "No BOOST_ROOT environment variable found" << std::endl;
		env_boost_root = QStandardPaths::standardLocations( QStandardPaths::HomeLocation ).first();
	}

	std::filesystem::path boost_root
		= QFileDialog::getExistingDirectory( nullptr, "Select boost root directory", env_boost_root ).toStdString();

	if( !std::filesystem::exists( boost_root / "Jamroot" ) ) {
		std::cerr << "Could not detect Jamroot file in selected folder :" << boost_root << std::endl;
		std::exit( 1 );
	} else {
		std::cout << "Selected boost installation: " << boost_root << std::endl;
	}

	return boost_root;
}


const std::vector<std::string> filter; // ={mpl, smart_ptr};
int main( int argc, char** argv )
{
	QApplication app( argc, argv );
	QMainWindow main_window;

	auto graph_widget = new gui::GraphWidget();
	main_window.setCentralWidget( graph_widget );

	modules_data modules;

	auto rescan = [&modules, &graph_widget] {
		auto boost_root = determine_boost_root();

		modules = generate_module_list( boost_root, filter );

		print_stats( modules );
		graph_widget->set_data( gui::layout_boost_modules( modules ) );
	};

	rescan();

	QObject::connect( graph_widget, &gui::GraphWidget::reload_requested, rescan );
	main_window.show();
	return app.exec();
}
