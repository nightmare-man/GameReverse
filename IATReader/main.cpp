#include <iostream>
#include <fstream>
#include "windows.h"

using namespace std;

//有个很坑的地方就是， window内核是utf-16但是输入输出不支持utf16， 宽字符的实现原理是utf16，每个字符都是两字节
//当控制台有多字节字符输输入的时候，实际上是拆成逐字节输入的
//当cout多字节字符的时候，实际上是拆成逐字节输出的

//设置windows不同编码，无非是终端收到字节流后，怎么解释这样的字节。

//windows内核使用的是unicode的utf-16实现方式，但是外部输入输出没有utf-16的方法
//所以在输入输出的时候，都要转换成utf-8或者本地编码。
//所以最好用char而不是wchar， 因为不论哪种编码，都可以使用char储存
//wchar理论上也可以储存，但是处理起来不方便（不是最小字节单位） wchar本意是给utf-16用的， char本意是给ascii用的，但是现在
//可以用多个char表示一个多字节字符， 也可以中一个wchar表示两个单字节ascii字符，或者一个utf-8的双字节
//对于编码， 只有这样两个事实： 1你要表示的字就在那儿，用不同的编码产生不同的字节,而如何储存这些字节， char 和wchar都可以，只是方便与否
//2对于已经有的一个char或者wchar数组，用不同的编码，得到不同的字符，山就在那里取决于你如何看。

//visual studio的默认文本编码和系统编码一样。
//windows上有个坑就是，如果之前用的gbk 936命名的中文路径，切换成unicode（utf-8）后,通过输入utf-8的字符串来打开路径的时候
//会找不到，因为以前使用gbk编码的文件夹名和文件名


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
	DIRECTORY_TABLE import_table;//dll导入表
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
	unsigned int file_offset;//对应的节在文件里的位置
	unsigned int file_reloc_offset;//对于可执行文件，这里总是0
	unsigned int unused1;
	unsigned short reloc_num;//可执行文件为0
	unsigned short unused2;
	unsigned int characteristic;//里面有对应节的各种属性，如读写等
}SECTION_TABLE, *PSECTION_TABLE;

typedef struct _IMPORT_DIRECTORY_TABLE {
	unsigned int import_lookup_table_rva;//dll里每个函数的导入查找表地址，
	unsigned int timstamp;//函数未绑定的时候为0，绑定时写入时间
	unsigned int forward_chain;//用户dll转发，暂未用到
	unsigned int name_rva;//dll名字
	unsigned int import_adress_table_rva; //最终的函数地址表地址，但一开始内容和导入查找表一样，记录的是函数的序号或者函数名的地址
	//导入表
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
			log_info(in, -4,"找到PE签名\n");
			break;
		}
		if (in.eof()) {
			log_info(in, 0,"到达文件末尾\n");
			break;
		}
	}
	cout << '\n';
}

void read_file_header(ifstream& in) {
	FILE_HEADER h;
	in.read((char*)&h, sizeof(FILE_HEADER));
	log_info(in, -18, "检测到机器码");
	cout << h.machine_code;
	if (h.machine_code == 332) {
		cout << "是32位可执行文件\n";
		section_size = h.section_numbers;
	}
	else {
		cout << "暂不支持的格式\n";
	}
	cout << '\n';
	return;
}


void read_option_header(ifstream& in) {
	OPTION_HEADER h;
	in.read((char*)&h, sizeof(OPTION_HEADER));
	cout << "程序基础地址";
	cout << h.image_base_va<<'\n';
	cout << "程序入口相对偏移";
	cout << h.entry_point_rva << '\n';
	cout << "程序代码段偏移";
	cout << h.code_base_rva<<'\n';
	cout << "程序数据段偏移";
	cout << h.data_base_rva << '\n';
	cout << "节对齐大小";
	cout << h.section_alignment << '\n';
	cout << "数据目录条数";
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
		cout << "数据目录错误，最后一条不为0\n";
	}
	else {
		import_table_rva = d.import_table.rva;
		cout << "导入表偏移";
		cout << d.import_table.rva << " 大小" << d.import_table.size << "\n";
		cout << "iat表偏移";
		cout << d.iat_table.rva << " 大小" << d.iat_table.size << "\n";
		cout << "重定向表偏移";
		cout << d.base_reloc_table.rva << " 大小" << d.base_reloc_table.size << "\n";
	}
	cout << '\n';
	return;
}

void read_section_table(ifstream& in) {
	SECTION_TABLE s;
	for (auto i = 0; i < section_size; i++) {
		in.read((char*)&s, sizeof(s));
		cout << "检测到节:" << s.name << '\t';
		cout << "对应内存偏移是：" << s.memory_rva << '\n';
		if (strcmp(s.name, ".rdata") == 0) {
			rdata_offset = s.file_offset;
			rdata_rva = s.memory_rva;
			import_table_offset = rdata_offset + import_table_rva - rdata_rva;
			cout << "计算得到dll导入表（import table）的文件偏移是:" << import_table_offset << '\n';
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

	cout << "请输入PE文件路径\n"<<endl;
	cin >> path;
	ifstream in{ path , ios::binary|ios::ate};
	auto file_size = in.tellg();
    cout << "文件大小" << file_size << "Byte\n";
	cout << "以下数据均为16进制:\n";
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