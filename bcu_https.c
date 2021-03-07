/*
* Copyright 2020 NXP.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* Redistributions in binary form must reproduce the above copyright notice, this
* list of conditions and the following disclaimer in the documentation and/or
* other materials provided with the distribution.
*
* Neither the name of the NXP Semiconductor nor the names of its
* contributors may be used to endorse or promote products derived from this
* software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*/
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS //in order to use strcpy without error
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bcu_https.h"
#include "version.h"

#if defined(linux) || defined(__APPLE__)
struct MemoryStruct {
	char* memory;
	size_t size;
};

static size_t writeMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct* mem = (struct MemoryStruct*)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

int _https_get_by_url(char* remote_url, struct latest_git_info* get_info)
{
	struct curl_slist* chunk = NULL;
	struct MemoryStruct response;
	CURL* curl;
	int res;

	chunk = curl_slist_append(chunk, "User-Agent: bcu");
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	response.memory = malloc(1);
	response.size = 0; /* no data at this point */

	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, remote_url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);
		curl_easy_setopt(curl, CURLOPT_CRLF, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
			printf("res = %d curl_easy_perform() failed: %s\n", res, curl_easy_strerror(res));
		else
		{
			// printf("response<%s>\n",response.memory);
			if (response.size > HTTPS_GET_STRING_LEN)
			{
				strncpy(get_info->http_get_string, response.memory, HTTPS_GET_STRING_LEN - 1);
				get_info->http_get_string[HTTPS_GET_STRING_LEN - 1] = 0;
			}
			else
			{
				strcpy(get_info->http_get_string, response.memory);
			}
		}
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	free(response.memory);

	return res;
}

size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	int written = fwrite(ptr, size, nmemb, stream);
	return written;
}

int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	static int i=0;
	i++;

	if (dltotal != 0 && dltotal > 10000)
		printf("\rDownloading progress: %.1fKB/%.1fKB\x1B[K", dlnow / 1024, dltotal / 1024);
	else
		printf("\rDownloading progress: prepare to download...\x1B[K");
	fflush(stdout);

	return 0;
}

int _download(char* url, char* out, char* extname)
{
	CURL* curl = NULL;
	FILE* fp = NULL;
	struct curl_slist* chunk = NULL;
	int res;
	char outname[50] = "";

	strcat(outname, out);
	if (!strcmp(extname, ".pdf"))
		strcat(outname, extname);

	chunk = curl_slist_append(chunk, "User-Agent: bcu");
	curl = curl_easy_init();
	if (curl)
	{
		fp = fopen(outname, "wb");

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_callback);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, NULL);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

		res = curl_easy_perform(curl);

		curl_easy_cleanup(curl);

		fclose(fp);
		printf("\n");
		return res;
	}
	else
	{
		return -1;
	}
}
#endif

#ifdef WIN32
int _https_get_by_url(char* remote_url, struct latest_git_info* get_info)
{
	int res = 0;
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	BYTE* pszOutBuffer;
	BOOL  bResults = FALSE;
	HINTERNET  hSession = NULL,
		hConnect = NULL,
		hRequest = NULL;
	unsigned int strsize = 0;
	char host[30];
	char objname[256];
	wchar_t whost[30];
	wchar_t wobjname[256];
	char remote_temp[2048];

	strcpy(remote_temp, remote_url);
	strcpy(host, strtok(&remote_temp[8], "/"));
	strcpy(objname, strtok(NULL, "\\"));
	swprintf(whost, 30, L"%hs", host);
	swprintf(wobjname, 256, L"%hs", objname);

	// Use WinHttpOpen to obtain a session handle.
	hSession = WinHttpOpen(L"WinHTTP Example/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);

	// Specify an HTTP server.
	if (hSession)
		hConnect = WinHttpConnect(hSession, whost,
			INTERNET_DEFAULT_HTTPS_PORT, 0);

	// Create an HTTP request handle.
	if (hConnect)
		hRequest = WinHttpOpenRequest(hConnect, L"GET", wobjname,
			NULL, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			WINHTTP_FLAG_SECURE);

	// Send a request.
	if (hRequest)
		bResults = WinHttpSendRequest(hRequest,
			WINHTTP_NO_ADDITIONAL_HEADERS, 0,
			WINHTTP_NO_REQUEST_DATA, 0,
			0, 0);

	// End the request.
	if (bResults)
		bResults = WinHttpReceiveResponse(hRequest, NULL);

	// Keep checking for data until there is nothing left.
	if (bResults)
	{
		do
		{
			// Check for available data.
			dwSize = 0;
			if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
				printf("Error %u in WinHttpQueryDataAvailable.\n",
					GetLastError());

			// Allocate space for the buffer.
			pszOutBuffer = malloc(sizeof(char) * (dwSize + 1));
			if (!pszOutBuffer)
			{
				printf("Out of memory\n");
				dwSize = 0;
			}
			else
			{
				// Read the data.
				ZeroMemory(pszOutBuffer, dwSize + 1);

				if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer,
					dwSize, &dwDownloaded))
				{
					printf("Error %u in WinHttpReadData.\n", GetLastError());
					res = -1;
				}
				else
				{
					if (strsize < HTTPS_GET_STRING_LEN)
					{
						if (dwSize > HTTPS_GET_STRING_LEN - strsize)
						{
							strncat(get_info->http_get_string, pszOutBuffer, HTTPS_GET_STRING_LEN - 1 - strsize);
							get_info->http_get_string[HTTPS_GET_STRING_LEN - 1] = 0;
							strsize = HTTPS_GET_STRING_LEN;
						}
						else
						{
							if (strsize == 0)
								strcpy(get_info->http_get_string, pszOutBuffer);
							else
								strcat(get_info->http_get_string, pszOutBuffer);
							strsize += dwSize;
						}
					}
				}

				// Free the memory allocated to the buffer.
				free(pszOutBuffer);
			}
		} while (dwSize > 0);
	}

	// Report any errors.
	if (!bResults)
	{
		printf("Error %d has occurred.\n", GetLastError());
		res = -1;
	}

	// Close any open handles.
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);

	return res;
}

char* wchar2char(const wchar_t* wchar)
{
	char* m_char;
	int len = WideCharToMultiByte(CP_ACP, 0, wchar, wcslen(wchar), NULL, 0, NULL, NULL);
	m_char = malloc(len * sizeof(char) + 5);
	WideCharToMultiByte(CP_ACP, 0, wchar, wcslen(wchar), m_char, len, NULL, NULL);
	if (m_char != NULL)
		m_char[len] = '\0';
	return m_char;
}

unsigned int fileSize = 0;

void CALLBACK winHttpSslStatusCallback(HINTERNET hInternet,
	DWORD_PTR context,
	DWORD code,
	void* pInfo,
	DWORD infoLength)
{
	char* buffer;
	char* temp;

	if (code == WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED)
	{
		DWORD headerBufferLength = 0;
		LPVOID tempHeaderBuffer = NULL;
		WinHttpQueryHeaders(hInternet,
			WINHTTP_QUERY_RAW_HEADERS_CRLF,
			WINHTTP_HEADER_NAME_BY_INDEX,
			NULL,
			&headerBufferLength,
			WINHTTP_NO_HEADER_INDEX);

		tempHeaderBuffer = (LPWSTR)malloc(headerBufferLength * sizeof(LPWSTR) + 5);
		WinHttpQueryHeaders(hInternet,
			WINHTTP_QUERY_RAW_HEADERS_CRLF,
			WINHTTP_HEADER_NAME_BY_INDEX,
			tempHeaderBuffer,
			&headerBufferLength,
			WINHTTP_NO_HEADER_INDEX);

		buffer = wchar2char(tempHeaderBuffer);
		//printf("GetHTTPResponse : Header Request Contents : \n\n%s", buffer);

		temp = strstr(buffer, "Content-Length:");
		temp += 16;
		temp = strtok(temp, "\r");
		fileSize = atoi(temp);
		if (fileSize < 50000)
			fileSize = 0;
	}
}

int _download(char* url, char* out, char* extname)
{
	int res = 0;
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;
	BYTE* pszOutBuffer;
	BOOL  bResults = FALSE;
	HINTERNET  hSession = NULL,
		hConnect = NULL,
		hRequest = NULL;
	int strsize = 0;
	char host[30];
	char objname[256];
	wchar_t whost[30];
	wchar_t wobjname[256];
	wchar_t wout[50];
	char remote_temp[2048];
	char outputname[50] = "";

	strcat(outputname, out);
	strcat(outputname, extname);
	strcpy(remote_temp, url);
	strcpy(host, strtok(&remote_temp[8], "/"));
	strcpy(objname, strtok(NULL, "\\"));
	swprintf(whost, 30, L"%hs", host);
	swprintf(wobjname, 256, L"%hs", objname);
	swprintf(wout, 50, L"%hs", outputname);

	printf("\rDownloading progress: prepare to download...\x1B[K");

	// Use WinHttpOpen to obtain a session handle.
	hSession = WinHttpOpen(L"WinHTTP Example/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);

	// Specify an HTTP server.
	if (hSession)
		hConnect = WinHttpConnect(hSession, whost,
			INTERNET_DEFAULT_HTTPS_PORT, 0);

	// Create an HTTP request handle.
	if (hConnect)
		hRequest = WinHttpOpenRequest(hConnect, L"GET", wobjname,
			NULL, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			WINHTTP_FLAG_SECURE);

	// Send a request.
	if (hRequest)
	{
		WinHttpSetStatusCallback(hRequest,
			winHttpSslStatusCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0);
		bResults = WinHttpSendRequest(hRequest,
			WINHTTP_NO_ADDITIONAL_HEADERS, 0,
			WINHTTP_NO_REQUEST_DATA, 0,
			0, 0);
	}

	// End the request.
	if (bResults)
		bResults = WinHttpReceiveResponse(hRequest, NULL);

	// Keep checking for data until there is nothing left.
	if (bResults)
	{
		unsigned int sizeDownloaded = 0;
		HANDLE hFile = CreateFileW(wout, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		do
		{
			// Check for available data.
			dwSize = 0;
			if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
				printf("Error %u in WinHttpQueryDataAvailable.\n",
					GetLastError());

			// Allocate space for the buffer.
			pszOutBuffer = malloc(sizeof(char) * (dwSize + 1));
			if (!pszOutBuffer)
			{
				printf("Out of memory\n");
				dwSize = 0;
			}
			else
			{
				// Read the data.
				ZeroMemory(pszOutBuffer, dwSize + 1);

				if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer,
					dwSize, &dwDownloaded))
				{
					printf("Error %u in WinHttpReadData.\n", GetLastError());
					res = -1;
				}
				else
				{
					sizeDownloaded += dwSize;
					printf("\rDownloading progress: %.1fKB/%.1fKB\x1B[K", (float)sizeDownloaded / 1024.0, (float)fileSize / 1024.0);
					WriteFile(hFile, pszOutBuffer, dwSize, &dwDownloaded, NULL);
				}

				// Free the memory allocated to the buffer.
				free(pszOutBuffer);
			}
		} while (dwSize > 0);
		printf("\n");

		CloseHandle(hFile);
	}

	// Report any errors.
	if (!bResults)
	{
		printf("Error %d has occurred.\n", GetLastError());
		res = -1;
	}

	// Close any open handles.
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);

	return res;
}
#endif

void https_response_parse(struct latest_git_info* get_info)
{
	char* remote_temp;

	remote_temp = strstr(get_info->http_get_string, "tag_name");
	remote_temp += 11;
	remote_temp = strtok(remote_temp, "\"");
	strcpy(get_info->tag_name, remote_temp);
	// printf("tag_name: %s\n", get_info->tag_name);

	remote_temp = strtok(NULL, "");
	remote_temp = strstr(remote_temp, "body");
	remote_temp += 7;
	remote_temp = strtok(remote_temp, "\"");
	strcpy(get_info->release_note, remote_temp);
	str_replace(get_info->release_note, "\\r\\n", "\r\n");
	str_replace(get_info->release_note, "\\t", "\t");

}

int https_download(struct latest_git_info* get_info)
{
	strcpy(get_info->download_url, get_info->download_url_base);
	strcat(get_info->download_url, get_info->tag_name);
	strcat(get_info->download_url, "/");
	strcat(get_info->download_url, get_info->download_name);
	strcat(get_info->download_url, get_info->extension_name);

#if defined(WIN32)
	printf("Downloading %s%s from %s\n", get_info->tag_name,
			get_info->extension_name, get_info->download_url);
#else
	printf("Downloading %s%s from %s\n", get_info->tag_name,
			!strcmp(get_info->extension_name, ".pdf") ? ".pdf" : "",
			get_info->download_url);
#endif
	if(_download(get_info->download_url, get_info->tag_name, get_info->extension_name))
	{
		printf("Download Failed!\n");
		return -1;
	}
	else
		printf("Download successfully!\n");

	return 0;
}

int https_get_by_url(char* remote_url, struct latest_git_info* get_info)
{
	return _https_get_by_url(remote_url, get_info);
}