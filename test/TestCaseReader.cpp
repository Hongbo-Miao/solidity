/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-3.0

#include <test/TestCaseReader.h>

#include <libsolidity/parsing/Parser.h>
#include <liblangutil/ErrorReporter.h>
#include <libsolutil/StringUtils.h>
#include <libsolutil/CommonIO.h>

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/throw_exception.hpp>
#include <boost/filesystem.hpp>

using namespace std;
using namespace solidity::langutil;
using namespace solidity::frontend::test;

namespace fs = boost::filesystem;

TestCaseReader::TestCaseReader(string const& _filename): m_filestream(_filename), m_filename(_filename)
{
	if (!m_filestream)
		BOOST_THROW_EXCEPTION(runtime_error("Cannot open file: \"" + _filename + "\"."));
	m_filestream.exceptions(ios::badbit);

	tie(m_sources, m_lineNumber) = parseSourcesAndSettingsWithLineNumber(m_filestream);
	m_unreadSettings = m_settings;
}

TestCaseReader::TestCaseReader(istringstream const& _str)
{
	tie(m_sources, m_lineNumber) = parseSourcesAndSettingsWithLineNumber(
		static_cast<istream&>(const_cast<istringstream&>(_str))
	);
}

string const& TestCaseReader::source() const
{
	if (m_sources.sources.size() != 1)
		BOOST_THROW_EXCEPTION(runtime_error("Expected single source definition, but got multiple sources."));
	return m_sources.sources.at(m_sources.mainSourceFile);
}

string TestCaseReader::simpleExpectations()
{
	return parseSimpleExpectations(m_filestream);
}

bool TestCaseReader::boolSetting(std::string const& _name, bool _defaultValue)
{
	if (m_settings.count(_name) == 0)
		return _defaultValue;

	m_unreadSettings.erase(_name);
	string value = m_settings.at(_name);
	if (value == "false")
		return false;
	if (value == "true")
		return true;

	BOOST_THROW_EXCEPTION(runtime_error("Invalid Boolean value: " + value + "."));
}

size_t TestCaseReader::sizetSetting(std::string const& _name, size_t _defaultValue)
{
	if (m_settings.count(_name) == 0)
		return _defaultValue;

	m_unreadSettings.erase(_name);

	static_assert(sizeof(unsigned long) <= sizeof(size_t));
	return stoul(m_settings.at(_name));
}

string TestCaseReader::stringSetting(string const& _name, string const& _defaultValue)
{
	if (m_settings.count(_name) == 0)
		return _defaultValue;

	m_unreadSettings.erase(_name);
	return m_settings.at(_name);
}

void TestCaseReader::ensureAllSettingsRead() const
{
	if (!m_unreadSettings.empty())
		BOOST_THROW_EXCEPTION(runtime_error(
			"Unknown setting(s): " +
			util::joinHumanReadable(m_unreadSettings | boost::adaptors::map_keys)
		));
}

pair<SourceMap, size_t> TestCaseReader::parseSourcesAndSettingsWithLineNumber(istream& _stream)
{
	map<string, string> sources;
	string currentSourceName;
	string currentSource;
	string line;
	size_t lineNumber = 1;
	static string const externalSourceDelimiterStart("==== ExternalSource:");
	static string const sourceDelimiterStart("==== Source:");
	static string const sourceDelimiterEnd("====");
	static string const comment("// ");
	static string const settingsDelimiter("// ====");
	static string const delimiter("// ----");
	bool sourcePart = true;
	while (getline(_stream, line))
	{
		lineNumber++;

		if (boost::algorithm::starts_with(line, delimiter))
			break;
		else if (boost::algorithm::starts_with(line, settingsDelimiter))
			sourcePart = false;
		else if (sourcePart)
		{
			if (boost::algorithm::starts_with(line, sourceDelimiterStart) && boost::algorithm::ends_with(line, sourceDelimiterEnd))
			{
				if (!(currentSourceName.empty() && currentSource.empty()))
					sources[currentSourceName] = std::move(currentSource);
				currentSource = {};
				currentSourceName = boost::trim_copy(line.substr(
					sourceDelimiterStart.size(),
					line.size() - sourceDelimiterEnd.size() - sourceDelimiterStart.size()
				));
				if (sources.count(currentSourceName))
					BOOST_THROW_EXCEPTION(runtime_error("Multiple definitions of test source \"" + currentSourceName + "\"."));
			}
			else if (boost::algorithm::starts_with(line, externalSourceDelimiterStart) && boost::algorithm::ends_with(line, sourceDelimiterEnd))
			{
				string externalSourcePath = boost::trim_copy(line.substr(
					externalSourceDelimiterStart.size(),
					line.size() - sourceDelimiterEnd.size() - externalSourceDelimiterStart.size()
				));
				string externalSourceName = externalSourcePath;

				// Does the external source define a remapping?
				size_t remappingPos = externalSourcePath.find('=');
				if (remappingPos != string::npos)
				{
					externalSourceName = boost::trim_copy(externalSourcePath.substr(0, remappingPos));
					externalSourcePath = boost::trim_copy(externalSourcePath.substr(remappingPos + 1));
				}

				string externalSourceContent;
				fs::path parent_path(m_filename);
				parent_path = parent_path.normalize().remove_filename();

				fs::path path = parent_path / fs::path(externalSourcePath);

				if (fs::exists(path))
					externalSourceContent = util::readFileAsString(path.c_str());
				else
					BOOST_THROW_EXCEPTION(runtime_error(string("External Source '"+ externalSourcePath +"' not found.")));

				if (!externalSourceName.empty())
				{
					ErrorList errorList;
					ErrorReporter errorReporter(errorList);
					Parser parser(errorReporter, langutil::EVMVersion::berlin());
					shared_ptr<Scanner> scanner = make_shared<Scanner>(CharStream(externalSourceContent, externalSourceName));
					ASTPointer<SourceUnit> sourceUnit = parser.parse(scanner);
					if (sourceUnit)
						for (auto const& node: sourceUnit->nodes())
							if (auto import = dynamic_cast<ImportDirective const*>(node.get()))
							{
								fs::path parent(externalSourcePath);
								parent = parent.parent_path();
								string importedSourceContent = util::readFileAsString(parent_path.string() + "/" + parent.string() + "/" + import->path());
								sources[parent.string() + "/" + import->path()] = importedSourceContent;
								sources[import->path()] = importedSourceContent;
							}

					sources[externalSourceName] = externalSourceContent;
				}
			}
			else
				currentSource += line + "\n";
		}
		else if (boost::algorithm::starts_with(line, comment))
		{
			size_t colon = line.find(':');
			if (colon == string::npos)
				BOOST_THROW_EXCEPTION(runtime_error(string("Expected \":\" inside setting.")));
			string key = line.substr(comment.size(), colon - comment.size());
			string value = line.substr(colon + 1);
			boost::algorithm::trim(key);
			boost::algorithm::trim(value);
			m_settings[key] = value;
		}
		else
			BOOST_THROW_EXCEPTION(runtime_error(string("Expected \"//\" or \"// ---\" to terminate settings and source.")));
	}
	// Register the last source as the main one
	sources[currentSourceName] = currentSource;
	return {{move(sources), move(currentSourceName)}, lineNumber};
}

string TestCaseReader::parseSimpleExpectations(istream& _file)
{
	string result;
	string line;
	while (getline(_file, line))
		if (boost::algorithm::starts_with(line, "// "))
			result += line.substr(3) + "\n";
		else if (line == "//")
			result += "\n";
		else
			BOOST_THROW_EXCEPTION(runtime_error("Test expectations must start with \"// \"."));
	return result;
}
