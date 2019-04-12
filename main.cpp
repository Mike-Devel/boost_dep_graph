
#include <ui/graphwidget.hpp>

#include <core/ModuleInfo.hpp>
#include <core/analysis.hpp>
#include <core/boostdep.hpp>

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
	QMainWindow  main_window;

	auto graph_widget = new gui::GraphWidget();


	main_window.setCentralWidget( graph_widget );

		// This data is referenced from multiple places in the UI, so it has to stay alive as long as the app is running
	modules_data modules;


	auto rescan = [&modules, &graph_widget] {

		auto boost_root = determine_boost_root();
		const auto files
			= boostdep::scan_all_boost_modules( boost_root, boostdep::TrackSources::Yes, boostdep::TrackTests::No );

		bool    ok = false;
		QString text
			= QInputDialog::getText( nullptr, "Select root library", "RootLib:", QLineEdit::Normal, "none", &ok );
		if( !ok ) {
			exit( 1 );
		}
		if( text.isEmpty() || text == "none" ) {
			modules = generate_module_list( files, boost_root, filter );
		} else {
			modules = generate_module_list( files, boost_root, text.toStdString(), filter );
		}

		auto cs = cycles( modules );
		std::cout << "Cycles:\n";
		for( auto& c : cs ) {
			for( auto& e : c ) {
				std::cout << e << " ";
			}
			std::cout << std::endl;
		}
		print_cmake_stats( modules );
		graph_widget->set_data( &modules );
	};

	auto print_stats = [&] { print_cmake_stats( modules ); };

	rescan();

	QObject::connect( graph_widget, &gui::GraphWidget::reload_requested, rescan );
	QObject::connect( graph_widget, &gui::GraphWidget::reprint_stats_requested, print_stats );
	main_window.show();

	return app.exec();
}
