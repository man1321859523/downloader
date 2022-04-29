#include <iostream>
#include <arpa/inet.h>
#include <cstring> // bzero
#include <sys/socket.h>	// shutdown
#include <netdb.h> // DNS
#include <stdio.h>

#define BUFFER_SIZE 2048

struct ResponseHeader{
	int status_code;
	char* content_type;
	long content_length;
	long range_start;
	long range_end;
};

class Urlparser{
public:
	Urlparser(){}
	Urlparser(const char* _url){
		getDomainName(_url);
		std::cout << std::string(m_domainName) << std::endl;
		std::cout << std::string(m_fileName) << std::endl;
		getIpByDomainName();
		//strlcpy(m_url, _url); // strcpy不安全
		int i= 0;
		while (i < 254 && *(_url + i) != '\0') {
			m_url[i] = *(_url + i);
			++i;
		}
		m_url[i] = '\0';
	}
	Urlparser(const char* _ip, const char* _port){}

	void getDomainName(const char* _url) {
		int i = 0;
		while (i < 255 && *(_url + i) != '\0') {
			if (*(_url + i) == '/') {
				i += 2;
				break;
			}
			++i;
		}
		int j = 0;
		while (i < 255 && *(_url + i) != '\0') {
			if (*(_url + i) == '/') {
				m_domainName[j] = '\0';
				break;
			}
			m_domainName[j] = *(_url + i);
			++i;
			++j;
		}
		m_port = 80;
		getFileName(_url, i + 1);
	}

	void getFileName(const char* _url, int i) {
		int j = 0;
		while (j < 255 && *(_url + i) != '\0') {
			m_fileName[j] = *(_url + i);
			++j;
			++i;
		}
		m_fileName[j] = '\0';
	}

	void getIpByDomainName() {
		struct hostent * host;
		host = gethostbyname(m_domainName);
		char **ptr;
		char str[16];
		if (host != nullptr) {
			std::cout << "The result from DNS server:" << std::endl;
			if (host->h_name != nullptr) {
				std::cout << "\tofficial name: " << std::string(host->h_name) << std::endl;
			}

			for(ptr = host->h_aliases; *ptr != nullptr; ++ptr) {
				std::cout << "\talias: " << std::string(*ptr) << std::endl;
			}

			if (host->h_addrtype == AF_INET || host->h_addrtype == AF_INET6) {
				for(ptr = host->h_addr_list; *ptr != nullptr; ++ptr) {
					const char *p = inet_ntop(host->h_addrtype, *ptr, str, sizeof(str));
					if (p != nullptr) {
						std::cout << "\tip: " << std::string(p) << std::endl;
						int i = 0;
				    	while (i < 16 && *(p + i) != '\0') {
				    		m_ip[i] = *(p + i);
				    		++i;
				    	}
				    	m_ip[i] = '\0';
						break;	// 有一个IP即可
					}
				}

			} else {
				std::cout << "unknown address type" << std::endl;
			}
		}
	}

	void getHttpHead(long range_start, long range_end) {
		//设置http请求头信息
		int Max = std::max(range_start, range_end);	//为了解决请求报文长度异常的bug
		int Min = std::min(range_start, range_end);
	    sprintf(m_buffer, \
    		"GET %s HTTP/1.1\r\n"\
	        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
	        "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537(KHTML, like Gecko) Chrome/47.0.2526Safari/537.36\r\n"\
	        "Host: %s\r\n"\
	        "Connection: keep-alive\r\n"\
	        "Range: bytes=%ld-%ld\r\n"\
        	"\r\n"\
    	,m_url ,m_domainName, Min, Max);
	}

	void getResponseHeader(int _sockfd) {
		int len = 0;
		len = recv(_sockfd, m_buffer, BUFFER_SIZE-1, 0);
		std::cout << len << std::endl;
		parseResponseHeader();
		std::cout << "status_code:" << m_ResponseHeader.status_code << std::endl;
	}	

	void parseResponseHeader() {
		/*获取响应头的信息*/
	    char *pos = strstr(m_buffer, "HTTP/");
	    if (pos)//获取返回代码
	        sscanf(pos, "%*s %d", &m_ResponseHeader.status_code);

	    pos = strstr(m_buffer, "Content-Type:");
	    if (pos)//获取返回文档类型
	        sscanf(pos, "%*s %s", m_ResponseHeader.content_type);

	    pos = strstr(m_buffer, "Content-Length:");
	    if (pos)//获取返回文档长度
	        sscanf(pos, "%*s %ld", &m_ResponseHeader.content_length);

	    pos = strstr(m_buffer, "Content-Range:");
	    if (pos)//获取返回文件区间
	        sscanf(pos, "%*s %*s %ld-%ld/%ld", 
	        	&m_ResponseHeader.range_start, &m_ResponseHeader.range_end, &m_ResponseHeader.content_length);
	}

public:
	char m_url[255];
	char m_domainName[255];
	char m_ip[16];
	int m_port;
	char m_fileName[255];
	char m_buffer[BUFFER_SIZE];
	ResponseHeader m_ResponseHeader;
};


int main(int argc, char* argv[]) {
	
	Urlparser * url;

	if(argc < 2) {
		std::cout << "wrong number of arguments." << std::endl;
		exit(1);
	} else if(argc == 2) {
		if (*argv[1] != '.' && (*argv[1] > '9' || *argv[1] < '0')) {
			url = new Urlparser(argv[1]);
			std::cout << url->m_ip << std::endl; 
		} else {
			std::cout << "wrong url format." << std::endl;
			exit(1);
		}
	} else {
		url = new Urlparser(argv[1], argv[2]);
		std::cout << url->m_ip << std::endl; 
	}
	
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_pton(AF_INET, url->m_ip, &server_addr.sin_addr);
	server_addr.sin_port = htons(url->m_port);

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		std::cout << "wrong socket." << std::endl;
		exit(1);
	}
	
	if(connect(sock, (struct sockaddr*)& server_addr, sizeof(server_addr)) != -1) {
		// 向buffer中写入数据
		url->getHttpHead(0, 0);
		std::cout << url->m_buffer << std::endl;
		send(sock, url->m_buffer, strlen(url->m_buffer), 0);
		url->getResponseHeader(sock);
		std::cout << url->m_buffer << std::endl;
	}
	shutdown(sock, SHUT_RDWR);
	delete url;

	return 0;
}







/*
	struct hostent {
	   char  *h_name;            // official name of host
	   char **h_aliases;         // alias list
	   int    h_addrtype;        // host address type
	   int    h_length;          // length of address
	   char **h_addr_list;       // list of addresses
	}
	struct hostent *gethostbyname(const char *name);
*/

// void getHttpHead(long range_start, long range_end);