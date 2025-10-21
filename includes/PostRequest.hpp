#pragma once

#include <string>
#include <iostream>
#include <sstream>

#include "ARequest.hpp"
#include "Logger.hpp"

class PostRequest : public ARequest
{
private:
	string _Content_type;
	string _body;

public:
	PostRequest();
	PostRequest(map<string, string> header, const string &session_id);
	PostRequest(const PostRequest &src);
	PostRequest &operator=(PostRequest &src);
	~PostRequest();

	int UploadFile(string body, string path);
	int UploadContent(map<string, string> content, string path);
	int createPost(string body, string postpath, string uploadpath);
	int handlePost(int fd, Server &server, const string &body, const ConfigParser *config);

	ARequest *clone() const;
};
