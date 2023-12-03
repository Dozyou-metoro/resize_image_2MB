#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
#define HAVE_STRUCT_TIMESPEC

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<stb_image.h>
#include<stb_image_resize.h>
#include<stb_image_write.h>

#include<Windows.h>
#include<io.h>

#include<array>
#include<vector>
#include<list>
#include<thread>

#include"error_macro.h"



unsigned char* pixel = nullptr;//画像データを入れる
int in_image_x = 0, in_image_y = 0, in_image_bpp = 0;
std::array<char, 1024> file_name = {};//入力ファイル名を入れておく

int file_no = 1;//ファイル番号割り当て用

const int default_bpp = 3;




class image_data {//画像データを管理するクラス
public:
	//リサイズ先画像関係
	int out_image_x = 0;
	int out_image_y = 0;
	
private:
	//リサイズ先画像関係
	unsigned char* out_pixel = nullptr;

	int image_no = 0;//ファイル番号を入れる
	size_t file_size = 0;

	std::array<char, 1024> name_buf = {};//仮ファイル名を入れる

public:

	void resize(void) {
		try {//リサイズ先のメモリを確保
			out_pixel = new unsigned char[out_image_x * out_image_y * default_bpp * sizeof(unsigned char)];
		}
		catch (std::bad_alloc) {
			ERROR_PRINT("MEM_ERROR", -1)
		}

		stbir_resize_uint8(pixel, in_image_x, in_image_y, 0, out_pixel, out_image_x, out_image_y, 0, default_bpp);
	}

	void image_output(void) {

		name_buf.fill('\0');

		sprintf(&name_buf.front(), "%s%03d", &file_name.front(), image_no);//仮名で出力

		if (out_pixel) {
			stbi_write_png(&name_buf.front(), out_image_x, out_image_y, default_bpp, out_pixel, 0);
			delete[] out_pixel;
			out_pixel = nullptr;
		}
	}

	void get_filesize(void) {
		FILE* fp = nullptr;
		fp = fopen(&name_buf.front(), "r");
		if (!fp) {
			ERROR_PRINT("file_not_open", -2)
		}

		file_size = _filelengthi64(_fileno(fp));

		fclose(fp);
	}
	
	_inline size_t return_filesize(void) {//file_sizeを読みだし専用にする関数
		return file_size;
	}

	_inline int set_fileno(int no) {//file_noを一度だけ書き込みする関数
		if ((no > 0)&&(image_no==0)) {
			image_no = no;
			return 0;
		} else {
			return -1;
		}
	}

	void image_rename(void) {
		std::array<char, 1024> str_buf = {};//リサイズ後ファイル名を入れる

		str_buf = name_buf;

		str_buf[strlen(&str_buf.front()) + 1  - 7] = '\0';//.pngxxxを消す
		sprintf(&str_buf.front(), "%s_2MB.png", &str_buf.front());

		rename(&name_buf.front(), &str_buf.front());
	}

	~image_data() {
		if (out_pixel) {
			delete[] out_pixel;
		}
		remove(&name_buf.front());
	}
};

void image_resize(void* data) {
	image_data* data_class = (image_data*)data;
	data_class->set_fileno(file_no);
	file_no++;
	data_class->resize();
	data_class->image_output();
	data_class->get_filesize();


}

size_t get_filesize(char* filename) {
	FILE* fp = nullptr;
	size_t file_size = 0;
	fp = fopen(filename, "r");
	if (!fp) {
		ERROR_PRINT("file_not_open", -2)
	}

	file_size = _filelengthi64(_fileno(fp));

	fclose(fp);

	return file_size;
}

int main(int argc, char** argv) {


	//ドラッグされなかった時の処理
	if (argc < 2) {
		printf("ファイルをドラッグして起動してください\n");
		Sleep(5000);
		return 0;
	}

	for (int i = 1; i < argc; i++) {

		//ここからメイン処理
		size_t in_image_size = get_filesize(argv[i]);
		if (in_image_size < 1024 * 1024 * 2) {//もう2MB以下の物はスキップ
			continue;
		}

		strcpy(&file_name.front(), argv[i]);//ファイル名をグローバル変数にコピー

		pixel = stbi_load(argv[i], &in_image_x, &in_image_y, &in_image_bpp, 3);

		if (!pixel) {
			printf("%d枚目の画像の読み込みに失敗しました。\n", i);
			continue;
		}

		int core_num = std::thread::hardware_concurrency();

		int image_notch = in_image_x / 16;


		std::vector<image_data> image_vec(image_notch);//サイズを記録しておく配列

		std::vector<std::thread*> thread_list(core_num);

		int th_count_create = 0;//処理枚数カウント(作成側)
		int th_count_join = 0;//処理枚数カウント(join側)
		while (1) {
			for (int th_j = 0; th_j < core_num; th_j++) {//プロセッサぶんスレッド生成
				if (th_count_create < image_notch) {
					image_vec[th_count_create].out_image_x = in_image_x - (16 * th_count_create);
					image_vec[th_count_create].out_image_y = in_image_y - (9 * th_count_create);
					std::thread* p_thread = new std::thread(image_resize, &image_vec[th_count_create]);//スレッド生成
					
					thread_list[th_j]=p_thread;//スレッド情報を記録
					th_count_create++;
				}
				
			}
			



			for (int th_j = 0; th_j < core_num; th_j++) {
				if (th_count_join < image_notch) {
					(*thread_list[th_j]).join();
					th_count_join++;
				}
			}

			if (th_count_join >= image_notch) {
				break;
			}
		}
			
		
		

		stbi_image_free(pixel);

		for (int j = 0; j < image_notch; j++) {
			if (image_vec[j].return_filesize() < 1024 * 1024 * 2) {
				image_vec[j].image_rename();
			}
		}




	}
}


