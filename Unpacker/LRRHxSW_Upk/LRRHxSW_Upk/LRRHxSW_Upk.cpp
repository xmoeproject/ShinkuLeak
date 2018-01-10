#include <my.h>
#include <string>
#include <vector>

#pragma comment(lib, "MyLibrary_x86_static.lib")

using std::wstring;
using std::vector;

int wmain(int argc, WCHAR* argv[])
{
	NTSTATUS      Status;
	NtFileDisk    File, Writer;
	ULONG         Size, FileSize, iPos, CurIndex, ChunkCnt, Index;
	PBYTE         Buffer;
	CHAR          SizeInfo[8];
	vector<ULONG> SizeList;

	ml::MlInitialize();

	if (argc != 2)
		return STATUS_INVALID_PARAMETER;

	LOOP_ONCE
	{
		Status = File.Open(argv[1]);
		if (NT_FAILED(Status))
			break;

		//archive size is very small, just load all.
		Size   = File.GetSize32();
		Buffer = (PBYTE)AllocateMemoryP(Size);
		Status = STATUS_NO_MEMORY;
		if (!Buffer)
			break;
			
		File.Read(Buffer, Size);
		//0x20 : Chunk break
		//0x71 : Chunk begin
		
		iPos = CurIndex = 0;
		RtlZeroMemory(SizeInfo, sizeof(SizeInfo));
		LOOP_FOREVER
		{
			if (Buffer[iPos] == 0x20)
				break;

			SizeInfo[CurIndex] = Buffer[iPos];
			iPos++;
			CurIndex++;
		};
		
		sscanf(SizeInfo, "%d", &ChunkCnt);
		RtlZeroMemory(SizeInfo, sizeof(SizeInfo));
		CurIndex = 0;
		Index = 0;
		iPos += 2;
		LOOP_FOREVER
		{
			if (Index == ChunkCnt)
				break;

			if (Buffer[iPos] == 0x20 && Buffer[iPos + 1] == 0x71)
			{
				iPos+=2;
				Index++;
				CurIndex = 0;
				sscanf(SizeInfo, "%d", &FileSize);
				PrintConsoleA("Chunk Size : %s\n", SizeInfo);
				RtlZeroMemory(SizeInfo, sizeof(SizeInfo));
				SizeList.push_back(FileSize);
			}

			SizeInfo[CurIndex] = Buffer[iPos];
			iPos++;
			CurIndex++;
		};


		wstring FileDir(argv[1]);
		FileDir = FileDir.substr(0, FileDir.find_last_of(L'.'));
		
		if (Nt_GetFileAttributes(FileDir.c_str()) == -1)
			CreateDirectoryW(FileDir.c_str(), NULL);

		auto GetIndexString = [](ULONG Index)->wstring
		{
			WCHAR Info[200];

			FormatStringW(Info, L"%d", Index);
			return Info;
		};

		auto GetFileExtension = [](PBYTE Buffer, ULONG Size)->wstring
		{
			if (Size < 4)                                                  return L".bin";
			else if (RtlCompareMemory(Buffer, "\x89\x50\x4E\x47", 4) == 4) return L".png";
			
			return L".bin";
		};
		
		Index = 0;
		iPos -= 1;
		for (auto CurFileSize : SizeList)
		{
			wstring FilePath = FileDir + L"\\";
			FilePath += GetIndexString(Index);
			FilePath += GetFileExtension(Buffer + iPos, CurFileSize);

			PrintConsoleW(L"debug : %s\n", FilePath.c_str());
			Status = Writer.Create(FilePath.c_str());
			Writer.Write(Buffer + iPos, CurFileSize);
			Writer.Close();

			iPos += CurFileSize;
			Index++;
		}

		FreeMemoryP(Buffer);
	};

	File.Close();
	return 0;
}

