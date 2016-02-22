#include "http_server.h"
#include "http_headers.h"
#include "http_content_type.h"

#include <iostream>
#include <sstream>
#include <mutex>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

static void skeleton_daemon()
{
	pid_t pid;

	/* Fork off the parent process */
	pid = fork();

	/* An error occurred */
	if (pid < 0)
		exit(EXIT_FAILURE);

	/* Success: Let the parent terminate */
	if (pid > 0)
		exit(EXIT_SUCCESS);

	/* On success: The child process becomes session leader */
	if (setsid() < 0)
		exit(EXIT_FAILURE);

	/* Catch, ignore and handle signals */
	//TODO: Implement a working signal handler */
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	/* Fork off for the second time*/
	pid = fork();

	/* An error occurred */
	if (pid < 0)
		exit(EXIT_FAILURE);

	/* Success: Let the parent terminate */
	if (pid > 0)
		exit(EXIT_SUCCESS);

	/* Set new file permissions */
	umask(0);

	/* Change the working directory to the root directory */
	/* or another appropriated directory */
	chdir("/");

	/* Close all open file descriptors */
	int x;
	for (x = sysconf(_SC_OPEN_MAX); x>0; x--)
	{
		close(x);
	}

	/* Open the log file */
	openlog("firstdaemon", LOG_PID, LOG_DAEMON);
}

int main(int argc, char *argv[])
{
	std::string		SrvAddress;
	std::string		RootDir;
	std::uint16_t	SrvPort;
	int opt = 0;

	while ((opt = getopt(argc, argv, "h:p:d")) != -1) {
		switch (opt) {
		case 'h':
			SrvAddress = std::string(optarg);
			break;
		case 'p':
			SrvPort = atoi(optarg);
			break;
		case 'd':
			RootDir = std::string(optarg);
			break;
		default: /* '?' */
			fprintf(stderr, "Usage: %s -h <ip> -p <port> -d <directory>\n",
				argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	skeleton_daemon();

	while (1)
	{
		//TODO: Insert daemon code here.
		syslog(LOG_NOTICE, "First daemon started.");

		std::uint16_t SrvThreadCount = 4;
		std::string const DefaultPage = "index.html";
		std::mutex Mtx;
		try
		{
			using namespace Network;
			HttpServer Srv(SrvAddress, SrvPort, SrvThreadCount,
				[&](IHttpRequestPtr req)
			{
				std::string Path = req->GetPath();
				Path = RootDir + Path + (Path == "/" ? DefaultPage : std::string());
				{
					std::stringstream Io;
					Io << "Path: " << Path << std::endl
						<< Http::Request::Header::Host::Name << ": "
						<< req->GetHeaderAttr(Http::Request::Header::Host::Value) << std::endl
						<< Http::Request::Header::Referer::Name << ": "
						<< req->GetHeaderAttr(Http::Request::Header::Referer::Value) << std::endl;
					std::lock_guard<std::mutex> Lock(Mtx);
					std::cout << Io.str() << std::endl;
				}
				req->SetResponseAttr(Http::Response::Header::Server::Value, "MyTestServer");
				req->SetResponseAttr(Http::Response::Header::ContentType::Value,
					Http::Content::TypeFromFileName(Path));
				req->SetResponseFile(Path);
			});
			std::cin.get();
		}
		catch (std::exception const &e)
		{
			std::cout << e.what() << std::endl;
		}



		sleep(20);
		break;
	}

	syslog(LOG_NOTICE, "First daemon terminated.");
	closelog();

	return EXIT_SUCCESS;

}
