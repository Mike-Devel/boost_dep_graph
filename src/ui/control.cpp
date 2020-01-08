#include "control.hpp"

#include <QFileDialog>
#include <QInputDialog>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

#include <filesystem>

#include <iostream>
#include <optional>
#include <string>

namespace mdev::bdg {

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
		std::cout << "Could not detect Jamroot file in selected folder : "<< boost_root << "\n";
		std::exit( 1 );
	} else {
		std::cout << "Selected boost installation: " << boost_root << "\n";
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
