#include "osconf.h"
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <vector>

#include "VoiceEncoder_Silk.h"
#include "SteamP2PCodec.h"
#include "VoiceEncoder_Speex.h"
#include "VoiceEncoder_Opus.h"
#include "voice_codec_frame.h"
#include "wav_enc.h"

CSteamP2PCodec* m_SilkCodec;
CSteamP2PCodec* m_OpusCodec;
VoiceCodec_Frame* m_SpeexCodec;

// my small binary reader(like c# BinReader)
class SmallBinReader
{
public:
	std::vector<unsigned char> internaldata;
	bool ErrorFound = false;
	int Operations = 0; // for detect error

	SmallBinReader(const unsigned char* data, int size)
	{
		internaldata = std::vector<unsigned char>(data, data + size);
	}

	SmallBinReader(const char* data, int size)
	{
		internaldata = std::vector<unsigned char>(data, data + size);
	}

	std::string ReadStr()
	{
		if (internaldata.empty())
		{
			ErrorFound = true;
			return "";
		}
		Operations++;
		std::vector<unsigned char> outstr;

		while (!internaldata.empty() && internaldata[0] != '\0')
		{
			outstr.push_back(internaldata[0]);
			internaldata.erase(internaldata.begin());
		}
		if (!internaldata.empty())
			internaldata.erase(internaldata.begin());

		return std::string(&outstr[0], &outstr[0] + outstr.size());
	}

	std::vector<unsigned char> ReadBytes(int size)
	{
		std::vector<unsigned char> retval = std::vector<unsigned char>();
		if (internaldata.empty())
		{
			ErrorFound = true;
			retval.clear();
			return retval;
		}
		Operations++;
		while (!internaldata.empty() && size)
		{
			retval.push_back(internaldata[0]);
			internaldata.erase(internaldata.begin());
			size--;
		}

		return retval;
	}

	int ReadBytes(int size, unsigned char* bytes)
	{
		if (internaldata.empty())
		{
			ErrorFound = true;
			return 0;
		}
		Operations++;
		int i = 0;
		while (!internaldata.empty() && size)
		{
			bytes[i] = internaldata[0];
			internaldata.erase(internaldata.begin());
			size--;
			i++;
		}

		return i;
	}

	unsigned char ReadByte()
	{
		if (internaldata.empty())
		{
			ErrorFound = true;
			return 0;
		}
		Operations++;
		unsigned char result = internaldata[0];
		internaldata.erase(internaldata.begin());
		return result;
	}

	char ReadChar()
	{
		if (internaldata.empty())
		{
			ErrorFound = true;
			return 0;
		}
		Operations++;

		char result = *(char*)&internaldata[0];
		internaldata.erase(internaldata.begin());
		return result;
	}

	short ReadShort()
	{
		if (internaldata.empty() || internaldata.size() < 2)
		{
			ErrorFound = true;
			return 0;
		}
		Operations++;

		short result = *(short*)&internaldata[0];
		internaldata.erase(internaldata.begin());
		return result;
	}

	unsigned short ReadUShort()
	{
		if (internaldata.empty() || internaldata.size() < 2)
		{
			ErrorFound = true;
			return 0;
		}
		Operations++;

		unsigned short result = *(unsigned short*)&internaldata[0];
		internaldata.erase(internaldata.begin());
		return result;
	}

	int ReadInt()
	{
		if (internaldata.empty() || internaldata.size() < 4)
		{
			ErrorFound = true;
			return 0;
		}
		Operations++;

		int result = *(int*)&internaldata[0];
		internaldata.erase(internaldata.begin());
		internaldata.erase(internaldata.begin());
		internaldata.erase(internaldata.begin());
		internaldata.erase(internaldata.begin());
		return result;
	}

	int ReadUInt()
	{
		if (internaldata.empty() || internaldata.size() < 4)
		{
			ErrorFound = true;
			return 0;
		}
		Operations++;

		unsigned int result = *(unsigned int*)&internaldata[0];
		internaldata.erase(internaldata.begin());
		internaldata.erase(internaldata.begin());
		internaldata.erase(internaldata.begin());
		internaldata.erase(internaldata.begin());
		return result;
	}

};


void DecodeSpeex()
{
	
	char buffer_out[1005000];
	int i = 0;

	FILE* f;
	fopen_s(&f, "input.wav.enc", "rb");
	if (f)
	{
		int samples = 0;
		WavFile fout("output.speex.wav",
			WavFile::WAV_16BIT,
			WavFile::WAV_MONO,
			8000);
		fseek(f, 0, SEEK_END);

		/*Get the current position of the file pointer.*/
		auto filelength = ftell(f);
		fseek(f, 0, SEEK_SET);
		unsigned char* buffer = new unsigned char[filelength];

		fread_s(buffer, filelength, 1, filelength, f);

		SmallBinReader tmpSmallBinReader(buffer, filelength);

		delete[] buffer;
		//read file
		while (!tmpSmallBinReader.ErrorFound)
		{
			// length of frame
			int length = tmpSmallBinReader.ReadInt();
			if (length > 0)
			{
				// length of frame
				auto data = tmpSmallBinReader.ReadBytes(length);

				int len = m_SpeexCodec->Decompress((const char*)&data[0], data.size(), buffer_out, 1005000);
				if (len <= 0)
				{
					fclose(f);
					fout.close();
					DeleteFileA("output.silk.wav");
					return;
				}
				int16_t* samples = (int16_t*)buffer_out;
				for (int n = 0; n < len; n++)
					fout.write_mono_16bit(samples[n]);
			}
		}

		fclose(f);		
		fout.close();
	}
}

void DecodeSilk()
{
	char buffer_out[1005000];
	int i = 0;

	FILE* f;
	fopen_s(&f, "input.wav.enc", "rb");
	if (f)
	{
		WavFile fout("output.silk.wav",
			WavFile::WAV_16BIT,
			WavFile::WAV_MONO,
			8000);
		fseek(f, 0, SEEK_END);

		/*Get the current position of the file pointer.*/
		auto filelength = ftell(f);
		fseek(f, 0, SEEK_SET);
		unsigned char* buffer = new unsigned char[filelength];

		fread_s(buffer, filelength, 1, filelength, f);

		SmallBinReader tmpSmallBinReader(buffer, filelength);

		delete[] buffer;
		//read file
		while (!tmpSmallBinReader.ErrorFound)
		{
			int length = tmpSmallBinReader.ReadInt();
			if (length > 0)
			{
				auto data = tmpSmallBinReader.ReadBytes(length);

				int len = m_SilkCodec->Decompress((const char*)&data[0], data.size(), buffer_out, 1005000);
				if (len <= 0)
				{
					fclose(f);
					fout.close();
					DeleteFileA("output.silk.wav");
					return;
				}
				int16_t* samples = (int16_t*)buffer_out;
				for (int n = 0; n < len; n++)
					fout.write_mono_16bit(samples[n]);
			}
		}
		fclose(f);
		fout.close();
	}
}

void DecodeOpus()
{
	char buffer_out[1005000];
	int i = 0;

	FILE* f;
	fopen_s(&f, "input.wav.enc", "rb");
	if (f)
	{
		WavFile fout("output.opus.wav",
			WavFile::WAV_16BIT,
			WavFile::WAV_MONO,
			8000);
		fseek(f, 0, SEEK_END);

		/*Get the current position of the file pointer.*/
		auto filelength = ftell(f);
		fseek(f, 0, SEEK_SET);
		unsigned char* buffer = new unsigned char[filelength];

		fread_s(buffer, filelength, 1, filelength, f);

		SmallBinReader tmpSmallBinReader(buffer, filelength);

		delete[] buffer;
		//read file
		while (!tmpSmallBinReader.ErrorFound)
		{
			int length = tmpSmallBinReader.ReadInt();
			if (length > 0)
			{
				auto data = tmpSmallBinReader.ReadBytes(length);

				int len = m_OpusCodec->Decompress((const char*)&data[0], data.size(), buffer_out, 1005000);
				if (len <= 0)
				{
					fclose(f);
					fout.close();
					DeleteFileA("output.opus.wav");
					return;
				}
				int16_t* samples = (int16_t*)buffer_out;
				for (int n = 0; n < len; n++)
					fout.write_mono_16bit(samples[n]);
			}
		}
		fclose(f);
		fout.close();
	}
}


DWORD WINAPI CODEC1(LPVOID)
{
	try
	{
		DecodeSpeex();
	}
	catch (...) // 1 level error bypass
	{
		DeleteFileA("output.speex.wav");
	}
	return 0;
}

DWORD WINAPI CODEC2(LPVOID)
{
	try
	{
		DecodeOpus();
	}
	catch (...) // 1 level error bypass
	{
		DeleteFileA("output.opus.wav");
	}
	return 0;
}

DWORD WINAPI CODEC3(LPVOID)
{
	try
	{
		DecodeSilk();
	}
	catch (...) // 1 level error bypass
	{
		DeleteFileA("output.silk.wav");
	}
	return 0;
}


HANDLE  hThreadArray[3];

DWORD WINAPI TERMINATETHREAD(HANDLE hndl)
{
	if (hndl == hThreadArray[0])
	{
		DeleteFileA("output.speex.wav");
	}
	if (hndl == hThreadArray[1])
	{
		DeleteFileA("output.opus.wav");
	}
	if (hndl == hThreadArray[2])
	{
		DeleteFileA("output.silk.wav");
	}
	TerminateThread(hndl, -1);
	return 0;
}

// 2 level error bypass
LONG WINAPI OurCrashHandler(EXCEPTION_POINTERS* pExcept)
{
	HANDLE currentthread = GetCurrentThread();
	CreateThread(0, 0, TERMINATETHREAD, currentthread, 0, 0);
	Sleep(5000);
	return ExceptionContinueExecution;
}


INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PSTR lpCmdLine, INT nCmdShow)
{
	int quality = 5;

	SetUnhandledExceptionFilter( OurCrashHandler );
	AddVectoredExceptionHandler(1, OurCrashHandler);

	// change dir from startup to startup\wav
	wchar_t tmpcurdir2[1024];
	wchar_t tmpcurdir[512];
	GetCurrentDirectoryW(512, tmpcurdir);
	swprintf_s(tmpcurdir2, L"%s\\wav",tmpcurdir);
	SetCurrentDirectoryW(tmpcurdir2);

	m_SpeexCodec = new VoiceCodec_Frame(new VoiceEncoder_Speex());
	m_SilkCodec = new CSteamP2PCodec(new VoiceEncoder_Silk());
	m_OpusCodec = new CSteamP2PCodec(new VoiceEncoder_Opus());

	m_SpeexCodec->Init(quality);
	m_SilkCodec->Init(quality);
	m_OpusCodec->Init(quality);

	// Multithread
	hThreadArray[1] = CreateThread(0, 0, CODEC3, 0, 0, 0);
	hThreadArray[2] = CreateThread(0, 0, CODEC2, 0, 0, 0);
	hThreadArray[0] = CreateThread(0, 0, CODEC1, 0, 0, 0);

	// Wait for multiple objects
	WaitForSingleObject(hThreadArray[1], INFINITE);
	WaitForSingleObject(hThreadArray[2], INFINITE);
	WaitForSingleObject(hThreadArray[0], INFINITE);

	return 0;
}