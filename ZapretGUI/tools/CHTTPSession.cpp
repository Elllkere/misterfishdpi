#include "CHTTPSession.h"

///////////////////////////////////////////////
// Helper:
///////////////////////////////////////////////
bool CHTTPSession::SetHTTPMethod(int iVERBID)
{
	switch (iVERBID)
	{
	case HTTP_GET:
		wcHTTPVerb = L"GET";
		break;
	case HTTP_POST:
		wcHTTPVerb = L"POST";
		break;
	default:
		wcHTTPVerb = L"";
		return false;
		break;
	}
	iHTTPMethod = iVERBID;
	return true;
}

///////////////////////////////////////////////
// Helper:
///////////////////////////////////////////////
bool CHTTPSession::SetHTTPSecuirty(int iProtocolID)
{
	if (iProtocolID == HTTP_CLEAR)
	{
		iSESSION_PROTOCOL = HTTP_CLEAR;
		iSESSION_PORT = 80;
	}
	else if (iProtocolID == HTTP_SSL)
	{
		iSESSION_PROTOCOL = HTTP_SSL;
		iSESSION_PORT = 443;
	}
	else {
		printf("failed in secuirty\n");
		return false;
	}
	return true;
}

///////////////////////////////////////////////
// Wrapper:
///////////////////////////////////////////////
bool CHTTPSession::Send(LPCWSTR wcDomain, LPCWSTR wcDirectory)
{
	if (CreateSession() == false)
		return false;

	if (SetServer(wcDomain, iSESSION_PORT) == false)
		return false;

	if (SetRequest(wcDirectory) == false)
		return false;

	// Send a request.
	if (SendRequest() == false)
		return false;

	// End the request.
	if (SetResponseHandle() == false)
		return false;

	return true;
}

///////////////////////////////////////////////
// Wrapper:
///////////////////////////////////////////////
bool CHTTPSession::Recv(int& iStatusCode, std::string& sHeader, std::string& sData)
{
	iStatusCode = GetHTTPStatusCode();

	std::wstring wsTmpBuf;
	wsTmpBuf = GetResponseHeader();
	sHeader.append(wsTmpBuf.begin(), wsTmpBuf.end());

	if (ReciveData(sData) == false)
		return false;
	else
		return true;
}

///////////////////////////////////////////////
// Wrapper:
///////////////////////////////////////////////
int CHTTPSession::Transmit(int iHTTP_Method, int iHTTP_Protocol, std::string sTarget, std::string& sRecvData, std::string sParameters, int iEncyption)
{
	if (SetHTTPMethod(iHTTP_Method) == false)
		return 1;

	if (SetHTTPSecuirty(iHTTP_Protocol) == false)
		return 2;

	if (HTTPPhraseData(sTarget, sParameters) == false)
		return 3;

	if (Send(wsDomain.c_str(), wsDocument.c_str()) == false)
		return 4;

	int iHTTPStatus;
	std::string sHeader;

	if (Recv(iHTTPStatus, sHeader, sRecvData) == false)
		return 5;

	return iHTTPStatus;
}

///////////////////////////////////////////////
// Helper:
///////////////////////////////////////////////
bool CHTTPSession::HTTPPhraseData(std::string sTarget, std::string sParameters)
{
	if (sTarget.empty() == true)
		return false;
	//printf("RAW: %s\n", sTarget);

	size_t iLocation;
	std::string sDomain, sDocument;

	iLocation = sTarget.find("http://");
	if (iLocation != std::string::npos)
		sTarget.erase(0, 7 /*strlen("http://")*/);

	iLocation = sTarget.find("/");
	if (iLocation == std::string::npos)
		iLocation = sTarget.size();

	sDomain.assign(sTarget, 0, iLocation);

	if ((sTarget.length() - iLocation) > 0)
		sDocument.assign(sTarget, sDomain.length(), sTarget.length() - sDomain.length());

	if (iHTTPMethod == HTTP_GET && sParameters.empty() == false)
	{
		sDocument += "?";
		sDocument += sParameters;
	}
	else
		sPOSTData = sParameters;

	wsDomain.append(sDomain.begin(), sDomain.end());
	wsDocument.append(sDocument.begin(), sDocument.end());

#ifdef DEBUG
	printf("[CNetProtocols::HTTPPhraseData(...)] Host: %s\tDirectory: %s\n", sDomain.c_str(), sDocument.c_str());
#endif
	return true;
};

//------------------------------------------------------------
// Framework
//------------------------------------------------------------
bool CHTTPSession::CreateSession()
{
	// Use WinHttpOpen to obtain a session handle.
	hSession = WinHttpOpen(wcUserAgent,
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS, 0);

	if (hSession)
		return true;
	else
		return false;
};

bool CHTTPSession::SetServer(LPCWSTR wcServerDomain, INTERNET_PORT wdPort)
{
	if (!hSession)
		return false;

	if (sizeof(wcServerDomain) <= 0 || wdPort == 0)
		return false;

	hConnect = WinHttpConnect(hSession, wcServerDomain, wdPort, 0);

	if (!hConnect)
		return false;
	else
		return true;
};

bool CHTTPSession::SetRequest(LPCWSTR wcData)
{
	if (!hConnect)
		return false;

	if (sizeof(wcHTTPVerb) <= 0)
		return false;

	if (iHTTPMethod == 0 || iSESSION_PROTOCOL == 0)
		return false;

	if (iSESSION_PROTOCOL == HTTP_SSL)
		hRequest = WinHttpOpenRequest(hConnect, wcHTTPVerb, wcData, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	else
		hRequest = WinHttpOpenRequest(hConnect, wcHTTPVerb, wcData, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, NULL);

	if (!hRequest)
		return false;
	else
		return true;
};

bool CHTTPSession::SendRequest()
{
	if (!hRequest)
		return false;

	if (iHTTPMethod == HTTP_POST)
	{
		std::string sPOSTDATA = "submit=";
		sPOSTDATA += sPOSTData;

		LPCWSTR additionalHeaders = L"Content-Type: application/x-www-form-urlencoded\r\n";

		return WinHttpSendRequest(hRequest, additionalHeaders, (DWORD)-1, (LPVOID)sPOSTDATA.c_str(), (DWORD)sPOSTDATA.length(), (DWORD)sPOSTDATA.length(), 0);
	}
	else
		return WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
};

bool CHTTPSession::SetResponseHandle()
{
	if (!hRequest)
		return false;

	return WinHttpReceiveResponse(hRequest, NULL);
};

std::wstring CHTTPSession::GetResponseHeader()
{
	if (!hRequest)
		return L"";

	DWORD dwSize = 0;

	WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
		WINHTTP_HEADER_NAME_BY_INDEX, NULL,
		&dwSize, WINHTTP_NO_HEADER_INDEX);

	// Allocate memory for the buffer.
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		std::wstring wsTmpBuffer = L"-2L";
		wchar_t* lpOutBuffer = new WCHAR[dwSize / sizeof(WCHAR)];

		// Now, use WinHttpQueryHeaders to retrieve the header.
		bool bResults = WinHttpQueryHeaders(hRequest,
			WINHTTP_QUERY_RAW_HEADERS_CRLF,
			WINHTTP_HEADER_NAME_BY_INDEX,
			lpOutBuffer, &dwSize,
			WINHTTP_NO_HEADER_INDEX);

		// Print the header contents.
		if (bResults)
			wsTmpBuffer = lpOutBuffer;

		// Free the allocated memory.
		delete[] lpOutBuffer;

		return wsTmpBuffer;
	}
	else
		return L"-1L";
};

int CHTTPSession::GetHTTPStatusCode()
{
	if (!hRequest)
		return false;

	DWORD dwStatusCode = 0;
	DWORD dwTemp = sizeof(dwStatusCode);

	WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
		NULL, &dwStatusCode, &dwTemp, NULL);

	return dwStatusCode;
};

bool CHTTPSession::ReciveData(std::string& sData)
{
	if (!hRequest)
		return false;

	LPSTR pszOutBuffer;
	DWORD dwSize = 0;
	DWORD dwDownloaded = 0;

	do
	{
		// Check for available data.
		dwSize = 0;
		if (WinHttpQueryDataAvailable(hRequest, &dwSize) == false)
			return false;

		// Allocate space for the buffer.
		pszOutBuffer = new char[dwSize + 1];
		if (!pszOutBuffer)
		{
			printf("Out of memory\n");
			return false;
		}
		else
		{
			// Read the data.
			ZeroMemory(pszOutBuffer, dwSize + 1);

			if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded) == false)
				printf("Error %u in WinHttpReadData.\n", GetLastError());
			else
				sData.append(pszOutBuffer, dwDownloaded);

			// Free the memory allocated to the buffer.
			delete[] pszOutBuffer;
		}
	} while (dwSize > 0);

	return true;
};

void CHTTPSession::CloseHandles()
{
	// Close any open handles.
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);
};