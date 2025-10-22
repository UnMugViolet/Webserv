#include "Logger.hpp"

ofstream Logger::_accessLogStream;
ofstream Logger::_errorLogStream;
string Logger::_accessFile;
string Logger::_errorFile;

Logger::Logger(ConfigParser &config)
{
	_accessFile = config.getValue("access_log");
	_errorFile = config.getValue("error_log");

	// Creating Access file if nothing provided fallback to default
	if (_accessFile.empty())
		_accessFile = DEFAULT_ACCESS_LOG_FILE;
	
	// Creating Error file same way as Access
	if (_errorFile.empty())
		_errorFile = DEFAULT_ERROR_LOG_FILE;
	
	_accessFile = LOG_FOLDER_PATH + _accessFile;
	_errorFile = LOG_FOLDER_PATH + _errorFile;

	_accessLogStream.open(_accessFile.c_str(), ios::app);
	_errorLogStream.open(_errorFile.c_str(), ios::app);
}

Logger::~Logger()
{
	if (_accessLogStream.is_open())
		_accessLogStream.close();
	if (_errorLogStream.is_open())
		_errorLogStream.close();
}

void	Logger::init()
{
	ofstream a(_accessFile.c_str(), ios::trunc);
	ofstream e(_errorFile.c_str(), ios::trunc);
	
	if (_accessLogStream.is_open())
        _accessLogStream.close();
    _accessLogStream.open(_accessFile.c_str(), ios::app);

    if (_errorLogStream.is_open())
        _errorLogStream.close();
    _errorLogStream.open(_errorFile.c_str(), ios::app);
}

void	Logger::info(const string &msg)
{
	if (msg.empty())
		return ;
	cout << BOLD << "[INFO] " << NEUTRAL << msg  << endl;
}

void	Logger::access(const string &serverUid, const string &msg)
{
	if (!_accessLogStream.is_open())
        cerr << "Logger: _accessLogStream not open!" << endl;
	else {
		// Check if rotation is needed before writing
		if (countLines(_accessFile) >= MAX_LOG_LINES) {
			rotateLogFile(_accessFile, _accessLogStream);
		}
		_accessLogStream << "[" << serverUid << "]\n" << msg << endl << endl;
		_accessLogStream.flush();
	}
}

void	Logger::error(const string &serverUid, const string &msg)
{
	if (!_errorLogStream.is_open())
        cerr << "Logger: _errorLogStream not open!" << endl << endl;
	else {
		// Check if rotation is needed before writing
		if (countLines(_errorFile) >= MAX_LOG_LINES) {
			rotateLogFile(_errorFile, _errorLogStream);
		}
		_errorLogStream << "[" << serverUid << "]\n" << "ERROR: " << msg << endl;
		_errorLogStream.flush();
	}
}

// Count the number of lines in a file
int Logger::countLines(const string &filename)
{
	ifstream file(filename.c_str());
	if (!file.is_open())
		return (0);
		
	int lineCount = 0;
	string line;
	
	while (getline(file, line))
		lineCount++;
		
	file.close();
	return (lineCount);
}

// Rotate log file by keeping only the most recent lines
void Logger::rotateLogFile(const string &filename, ofstream &stream)
{
	const int linesToKeep = MAX_LOG_LINES / 2; // Keep half the max amount of lines
	
	// Read all lines
	ifstream file(filename.c_str());
	if (!file.is_open())
		return ;
		
	vector<string> lines;
	string line;

	while (getline(file, line))
		lines.push_back(line);
	file.close();
	
	// Close the current stream
	if (stream.is_open())
		stream.close();
		
	// Rewrite file with only the most recent lines
	stream.open(filename.c_str(), ios::trunc);
	if (stream.is_open()) {
		int startIndex = (lines.size() > linesToKeep) ? lines.size() - linesToKeep : 0;
		for (int i = startIndex; i < (int)lines.size(); i++) {
			stream << lines[i] << endl;
		}
		stream.flush();
	}
}
