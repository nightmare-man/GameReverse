#include <iostream>
#include <fstream>
#include "windows.h"

using namespace std;

//�и��ܿӵĵط����ǣ� window�ں���utf-16�������������֧��utf16�� ���ַ���ʵ��ԭ����utf16��ÿ���ַ��������ֽ�
//������̨�ж��ֽ��ַ��������ʱ��ʵ�����ǲ�����ֽ������
//��cout���ֽ��ַ���ʱ��ʵ�����ǲ�����ֽ������

//����windows��ͬ���룬�޷����ն��յ��ֽ�������ô�����������ֽڡ�

//windows�ں�ʹ�õ���unicode��utf-16ʵ�ַ�ʽ�������ⲿ�������û��utf-16�ķ���
//���������������ʱ�򣬶�Ҫת����utf-8���߱��ر��롣
//���������char������wchar�� ��Ϊ�������ֱ��룬������ʹ��char����
//wchar������Ҳ���Դ��棬���Ǵ������������㣨������С�ֽڵ�λ�� wchar�����Ǹ�utf-16�õģ� char�����Ǹ�ascii�õģ���������
//�����ö��char��ʾһ�����ֽ��ַ��� Ҳ������һ��wchar��ʾ�������ֽ�ascii�ַ�������һ��utf-8��˫�ֽ�
//���ڱ��룬 ֻ������������ʵ�� 1��Ҫ��ʾ���־����Ƕ����ò�ͬ�ı��������ͬ���ֽ�,����δ�����Щ�ֽڣ� char ��wchar�����ԣ�ֻ�Ƿ������
//2�����Ѿ��е�һ��char����wchar���飬�ò�ͬ�ı��룬�õ���ͬ���ַ���ɽ��������ȡ��������ο���

//visual studio��Ĭ���ı������ϵͳ����һ����
//windows���и��Ӿ��ǣ����֮ǰ�õ�gbk 936����������·�����л���unicode��utf-8����,ͨ������utf-8���ַ�������·����ʱ��
//���Ҳ�������Ϊ��ǰʹ��gbk������ļ��������ļ���


typedef struct _FILE_HEADER {
	unsigned short machine_code;
	unsigned short section_numbers;
	unsigned int timestamp;
	unsigned int file_offset_symbol;
	unsigned int symbol_number;
	unsigned short  option_headers_size;
	unsigned short characteristic;
}FILE_HEADER, *PFILE_HEADER;

typedef struct _OPTION_HEADER {
	unsigned short magic;
	unsigned char major_link_version;
	unsigned char minor_link_version;
	unsigned int code_size;
	unsigned int initialized_data_size; //
	unsigned int uninitialized_data_size; //bss
	unsigned int entry_point_rva;
	unsigned int code_base_rva;
	unsigned int data_base_rva;
	unsigned int image_base_va;
	unsigned int section_alignment;
	unsigned int file_alignment;
	unsigned short max_os_version;
	unsigned short min_os_version;
	unsigned short max_image_version;
	unsigned short min_image_version;
	unsigned short max_sub_os_v;
	unsigned short min_sub_os_v;
	unsigned int not_used_1;
	unsigned int image_size;
	unsigned int headers_size;
	unsigned int check_sum;
	unsigned short sub_os_v;
	unsigned short dll_characteristic;
	unsigned int stack_reserve_size;
	unsigned int star_commit_size;
	unsigned int heap_reserve_size;
	unsigned int heap_commit_size;
	unsigned int not_used_2;
	unsigned int image_data_directory_num;
} OPTION_HEADER, *POPTION_HEADER;

typedef struct _DIRECTORY_TABLE {
	unsigned int rva;
	unsigned int size;
}DIRECTORY_TABLE,* PDIRECTORY_TABLE;

typedef struct _DATA_DIRECTORY {
	DIRECTORY_TABLE export_table;
	DIRECTORY_TABLE import_table;//dll�����
	DIRECTORY_TABLE resource_table;
	DIRECTORY_TABLE exception_table;
	DIRECTORY_TABLE certificate_table;
	DIRECTORY_TABLE base_reloc_table;
	DIRECTORY_TABLE debug_table;
	DIRECTORY_TABLE architecture_table;
	DIRECTORY_TABLE global_ptr_table;
	DIRECTORY_TABLE tls_table;
	DIRECTORY_TABLE load_config_table;
	DIRECTORY_TABLE bound_import_table;
	DIRECTORY_TABLE iat_table;
	DIRECTORY_TABLE delay_import_descriptor;
	DIRECTORY_TABLE clr_table;
	DIRECTORY_TABLE preserved_table;
}DATA_DIRECTORY, *PDATA_DIRECTORY;

typedef struct _SECTION_TABLE {
	char name[8];
	unsigned int memory_size;
	unsigned int memory_rva;
	unsigned int file_size;
	unsigned int file_offset;//��Ӧ�Ľ����ļ����λ��
	unsigned int file_reloc_offset;//���ڿ�ִ���ļ�����������0
	unsigned int unused1;
	unsigned short reloc_num;//��ִ���ļ�Ϊ0
	unsigned short unused2;
	unsigned int characteristic;//�����ж�Ӧ�ڵĸ������ԣ����д��
}SECTION_TABLE, *PSECTION_TABLE;

typedef struct _IMPORT_DIRECTORY_TABLE {
	unsigned int import_lookup_table_rva;//dll��ÿ�������ĵ�����ұ��ַ��
	unsigned int timstamp;//����δ�󶨵�ʱ��Ϊ0����ʱд��ʱ��
	unsigned int forward_chain;//�û�dllת������δ�õ�
	unsigned int name_rva;//dll����
	unsigned int import_adress_table_rva; //���յĺ�����ַ���ַ����һ��ʼ���ݺ͵�����ұ�һ������¼���Ǻ�������Ż��ߺ������ĵ�ַ
	//�����
}IMPORT_DIRECTORY_TABLE, *PIMPORT_DIRECTORY_TABLE;

static unsigned int data_table_size = 0;
static unsigned int section_size = 0;
static unsigned int import_table_rva = 0;
static unsigned int import_table_offset = 0;
static unsigned int rdata_rva = 0;
static unsigned int rdata_offset = 0;
BOOL need_char(ifstream& in, char target) {
	BOOL result = FALSE;
	char buf = -1;
	
	in.read(&buf, 1);
	if (buf == target) {
		result = TRUE;
	}
	else {
		result = FALSE;
		in.putback(buf);
	}
	return result;
}

unsigned char read_char(ifstream& in) {
	char buf = -1;
	in.read(&buf, 1);
	return (unsigned char)buf;
}

unsigned short read_short(ifstream& in) {
	unsigned short buf = 0;
	char unit[] = { -1,-1 };
	in.read(unit, 1);
	in.read(unit + 1, 1);
	buf = (unsigned char)(unit[0]) + ((unsigned char)(unit[1])) * 256;
	return buf;
}

unsigned int read_int(ifstream& in) {
	unsigned int buf = 0;
	char unit[] = { -1,-1,-1,-1 };
	in.read(unit, 1);
	in.read(unit + 1, 1);
	in.read(unit + 2, 1);
	in.read(unit + 3, 1);
	buf = (unsigned char)(unit[0]) + ((unsigned char)(unit[1])) * 256 + ((unsigned char)(unit[2])) * 256 * 256 + ((unsigned char)(unit[3])) * 256 * 256 * 256;
	return buf;
}

void log_info(ifstream& in, int off1,const char* text) {
	auto off = in.tellg();
	off += off1;
	cout << off << text;
}

void print_file_str(ifstream& in, unsigned int off) {
	auto origin_off = in.tellg();
	in.seekg(off);
	char buf = -1;
	while (1) {
		in.read(&buf, 1);
		if (buf == NULL) break;
		cout << buf;
	}
	in.seekg(origin_off);
	return;
}

void read_signature(ifstream& in) {
	while (1) {
		unsigned char f1= read_char(in);
		if (f1 == 'P' && need_char(in, 'E') && need_char(in, 0) && need_char(in, 0)) {
			log_info(in, -4,"�ҵ�PEǩ��\n");
			break;
		}
		if (in.eof()) {
			log_info(in, 0,"�����ļ�ĩβ\n");
			break;
		}
	}
	cout << '\n';
}

void read_file_header(ifstream& in) {
	FILE_HEADER h;
	in.read((char*)&h, sizeof(FILE_HEADER));
	log_info(in, -18, "��⵽������");
	cout << h.machine_code;
	if (h.machine_code == 332) {
		cout << "��32λ��ִ���ļ�\n";
		section_size = h.section_numbers;
	}
	else {
		cout << "�ݲ�֧�ֵĸ�ʽ\n";
	}
	cout << '\n';
	return;
}


void read_option_header(ifstream& in) {
	OPTION_HEADER h;
	in.read((char*)&h, sizeof(OPTION_HEADER));
	cout << "���������ַ";
	cout << h.image_base_va<<'\n';
	cout << "����������ƫ��";
	cout << h.entry_point_rva << '\n';
	cout << "��������ƫ��";
	cout << h.code_base_rva<<'\n';
	cout << "�������ݶ�ƫ��";
	cout << h.data_base_rva << '\n';
	cout << "�ڶ����С";
	cout << h.section_alignment << '\n';
	cout << "����Ŀ¼����";
	cout << h.image_data_directory_num << '\n';
	cout << '\n';
	data_table_size = h.image_data_directory_num;
	return;
}

void read_data_directorys(ifstream& in) {
	DATA_DIRECTORY d;
	in.read((char*)&d, sizeof(DATA_DIRECTORY));
	PDIRECTORY_TABLE last = ((((PDIRECTORY_TABLE)(&d)) + data_table_size-1));
	if (last->rva != 0 || last->size != 0) {
		cout << "����Ŀ¼�������һ����Ϊ0\n";
	}
	else {
		import_table_rva = d.import_table.rva;
		cout << "�����ƫ��";
		cout << d.import_table.rva << " ��С" << d.import_table.size << "\n";
		cout << "iat��ƫ��";
		cout << d.iat_table.rva << " ��С" << d.iat_table.size << "\n";
		cout << "�ض����ƫ��";
		cout << d.base_reloc_table.rva << " ��С" << d.base_reloc_table.size << "\n";
	}
	cout << '\n';
	return;
}

void read_section_table(ifstream& in) {
	SECTION_TABLE s;
	for (auto i = 0; i < section_size; i++) {
		in.read((char*)&s, sizeof(s));
		cout << "��⵽��:" << s.name << '\t';
		cout << "��Ӧ�ڴ�ƫ���ǣ�" << s.memory_rva << '\n';
		if (strcmp(s.name, ".rdata") == 0) {
			rdata_offset = s.file_offset;
			rdata_rva = s.memory_rva;
			import_table_offset = rdata_offset + import_table_rva - rdata_rva;
			cout << "����õ�dll�����import table�����ļ�ƫ����:" << import_table_offset << '\n';
		}
	}
	cout << '\n';
	return;
}

void read_dll_func_name_table(ifstream& in, unsigned int off, unsigned int iat_rva) {
	unsigned int origin_off = in.tellg();
	in.seekg(off);
	unsigned int buf = 1;
	unsigned int idx = 0;
	while (1) {
		in.read((char*)&buf, sizeof(buf));
		if (buf == NULL) {
			break;
		}
		if (buf & 0x80000000 ) {
			continue;
		}
		unsigned int hint_name_rva = buf;
		unsigned int hint_name_off = rdata_offset + hint_name_rva - rdata_rva;
		unsigned int str_off = hint_name_off + 2;
		cout << "\t";
		print_file_str(in, str_off);
		cout << '\n';
		cout << "\t";
		cout << "rva:";
		cout << iat_rva + idx * 4;
		cout << "\n";

		idx++;
	}
	in.seekg(origin_off);
	return;
}

void read_import_directory_table(ifstream& in) {
	auto origin_off = in.tellg();
	in.seekg(import_table_offset);
	IMPORT_DIRECTORY_TABLE t;
	while (1) {

		in.read((char*)&t, sizeof(t));
		if (t.name_rva == NULL) break;
		print_file_str(in, rdata_offset +  t.name_rva-rdata_rva);
		cout << '\n';
		unsigned int import_lookup_table_off = rdata_offset + t.import_lookup_table_rva - rdata_rva;
		read_dll_func_name_table(in, import_lookup_table_off, t.import_adress_table_rva);
	}
	return;
	in.seekg(origin_off);
}
int main(){
	string path;

	cout << "������PE�ļ�·��\n"<<endl;
	cin >> path;
	ifstream in{ path , ios::binary|ios::ate};
	auto file_size = in.tellg();
    cout << "�ļ���С" << file_size << "Byte\n";
	cout << "�������ݾ�Ϊ16����:\n";
	cout << '\n';

	cout << hex;
	in.seekg(0);
	read_signature(in);
	read_file_header(in);
	read_option_header(in);
	read_data_directorys(in);
	read_section_table(in);
	read_import_directory_table(in);
	return 0;
}