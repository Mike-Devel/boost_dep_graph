#include "control.hpp"

#include <QFileDialog>
#include <QInputDialog>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

#include <filesystem>

#include <optional>
#include <string>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace mdev::bdg {

std::filesystem::path determine_boost_root()
{

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

	auto default_folder = env.value( "BOOST_ROOT", "" );

	if( default_folder.isEmpty() ) {
		fmt::print( "No BOOST_ROOT environment variable found\n" );
		default_folder = QStandardPaths::standardLocations( QStandardPaths::HomeLocation ).first();
	}

	std::filesystem::path boost_root
		= QFileDialog::getExistingDirectory( nullptr, "Select boost root directory", default_folder ).toStdString();

	if( !std::filesystem::exists( boost_root / "Jamroot" ) ) {
		fmt::print( "Could not detect Jamroot file in selected folder : {}\n", boost_root );
		std::exit( 1 );
	} else {
		fmt::print( "Selected boost installation: {}\n", boost_root );
	}

	return boost_root;
}

std::optional<String_t> get_root_library_name()
{
	bool    ok   = false;
	QString text = QInputDialog::getText( nullptr, "Select root library", "RootLib:", QLineEdit::Normal, "none", &ok );

	if( !ok || text.isEmpty() || text == "none" ) {
		return {};
	} else {
		return String_t{text.toStdString()};
	}
}

} // namespace mdev::bdg
