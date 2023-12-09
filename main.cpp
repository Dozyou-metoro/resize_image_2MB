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
#include<thread>

#include"error_macro.h"



unsigned char* pixel = nullptr;//画像データを入れる
int in_image_x = 0, in_image_y = 0, in_image_bpp = 0;
std::array<char, 1024> file_name = {};//入力ファイル名を入れておく

int file_no = 1;//ファイル番号割り当て用

const int default_bpp = 3;
const size_t limit_size = 1024 * 1024 * 2;




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
		if ((no > 0) && (image_no == 0)) {
			image_no = no;
			return 0;
		} else {
			return -1;
		}
	}

	void image_rename(void) {
		std::array<char, 1024> str_buf = {};//リサイズ後ファイル名を入れる

		str_buf = name_buf;

		str_buf[strlen(&str_buf.front()) + 1 - 7] = '\0';//.pngxxxを消す
		sprintf(&str_buf.front(), "%s_2MB.png", &str_buf.front());//_2MB.pngを付けたす

		rename(&name_buf.front(), &str_buf.front());
	}

	~image_data() {
		if (out_pixel) {//まだ解放されてないなら解放
			delete[] out_pixel;
		}
		remove(&name_buf.front());//一時ファイルを消す
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
		printf("ver:1.1\n");
		printf("ファイルをドラッグして起動してください\n");
		Sleep(5000);
		return 0;
	}

	for (int i = 1; i < argc; i++) {

		//ここからメイン処理
		size_t in_image_size = get_filesize(argv[i]);
		if (in_image_size < limit_size) {//もう2MB以下の物はスキップ
			printf("%d枚目の2MB以下の画像をスキップしました。\n", i);
			continue;
		}

		strcpy(&file_name.front(), argv[i]);//ファイル名をグローバル変数にコピー

		pixel = stbi_load(argv[i], &in_image_x, &in_image_y, &in_image_bpp, 3);

		if (!pixel) {
			printf("%d枚目の画像の読み込みに失敗しました。\n", i);
			continue;
		}
		printf("%d枚目の画像を読み込みました。\n", i);

		int core_num = std::thread::hardware_concurrency();

		int image_notch = in_image_x / 16;

		file_no = 0;//仮ファイル名カウンタカンスト対策


		std::vector<image_data> image_vec(image_notch);//サイズを記録しておく配列

		std::vector<std::thread*> thread_list(image_notch);//スレッド情報を記録する配列

		int th_count_create = 0;//処理枚数カウント(作成側)
		int th_count_join = 0;//処理枚数カウント(join側)
		int image_vec_num = 0;//最終的に処理した数
		int limit_clear_no = ~0;//基準を満たした配列の番号

		while (1) {//一次探索
			for (int th_j = 0; th_j < core_num; th_j++) {//プロセッサぶんスレッド生成
				if (th_count_create < image_notch) {//リサイズ値がマイナスしないように確認
					image_vec[th_count_create].out_image_x = in_image_x - (16 * th_count_create);
					image_vec[th_count_create].out_image_y = in_image_y - (9 * th_count_create);
					try {
						std::thread* p_thread = new std::thread(image_resize, &image_vec[th_count_create]);//スレッド生成
						thread_list[th_count_create] = p_thread;//スレッド情報を記録
						th_count_create = th_count_create + core_num + 1;//プロセッサ数分開けて探索
					}
					catch (std::bad_alloc) {
						ERROR_PRINT("MEM_ERROR", -1)
					}
				}
			}

			for (int th_j = 0; th_j < core_num; th_j++) {//スレッド処理が終わるのを待つ
				if (th_count_join < image_notch) {

					printf("\033[1K\033[0Gファイルを出力して探索中。進捗%d%%。", (int)(th_count_join * 100) / image_notch);
					fflush(stdout);

					(*thread_list[th_count_join]).join();

					if ((image_vec[th_count_join].return_filesize() < limit_size) && (limit_clear_no == ~0)) {
						if (th_count_join == 0) {
							limit_clear_no = th_count_join;//条件を満たす寸前の場所を記録
						} else {
							limit_clear_no = th_count_join - (core_num + 1);
						}
					}

					th_count_join = th_count_join + core_num + 1;
				}
			}

			if (limit_clear_no < 0xffffffff) {
				break;
			}
		}

		printf("\033[1K\033[0Gファイルを出力して探索中。進捗%d%%。\n", 100);
		fflush(stdout);

		th_count_create = limit_clear_no;
		th_count_join = limit_clear_no;


		for (int th_j = 0; th_j < core_num; th_j++) {//プロセッサぶんスレッド生成
			if (th_count_create < image_notch) {//リサイズ値がマイナスしないように確認
				image_vec[th_count_create].out_image_x = in_image_x - (16 * th_count_create);
				image_vec[th_count_create].out_image_y = in_image_y - (9 * th_count_create);
				try {
					std::thread* p_thread = new std::thread(image_resize, &image_vec[th_count_create]);//スレッド生成
					thread_list[th_count_create] = p_thread;//スレッド情報を記録
					th_count_create++;
				}
				catch (std::bad_alloc) {
					ERROR_PRINT("MEM_ERROR", -1)
				}
			}
		}

		for (int th_j = 0; th_j < core_num; th_j++) {//スレッド処理が終わるのを待つ
			if (th_count_join < image_notch) {

				printf("\033[1K\033[0Gファイルを出力して探索中。進捗%d%%。", (int)(th_count_join * 100) / image_notch);
				fflush(stdout);

				(*thread_list[th_count_join]).join();
				th_count_join++;
			}
		}




		stbi_image_free(pixel);


		for (int j = limit_clear_no; j < limit_clear_no + core_num + 1; j++) {//条件内でいちばん大きいものを選択してリネーム
			printf("\033[1K\033[0G条件に合う画像を探索中。進捗%d%%。", (int)(j * 100) / image_notch);
			fflush(stdout);
			if (image_vec[j].return_filesize() < limit_size) {
				image_vec[j].image_rename();
				printf("\033[1K\033[0G条件に合う画像を探索中。進捗%d%%。\n", 100);
				fflush(stdout);
				printf("%d枚目の画像を%3.2fMBで書き出しました。\n", i, (double)image_vec[j].return_filesize() * 2 / (limit_size));
				fflush(stdout);
				break;
			}
		}
	}
	printf("完了しました。ご利用ありがとうございました。\n");
	fflush(stdout);
	Sleep(5000);
}


