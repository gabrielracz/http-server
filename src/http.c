#include "http.h"

char* get_status_code(int code){
	switch(code){
		case 200: return "200 OK";
		case 404: return "404 Not Found";
		case 500: return "500 Server Error";
		case 503: return "503 Service Unavailable";
	}
	return NULL;
}

int build_http_response(HTTP_req* req, char* buffer, size_t buflen) {
	return 0;
}



