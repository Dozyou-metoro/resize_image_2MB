#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
#define HAVE_STRUCT_TIMESPEC

#include<stb_image_write.h>
#include<stb_image.h>
#include<stb_image_resize.h>
#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include<io.h>
#include<pthread.h>

typedef struct {
	int count;
	int argc;
	char* argv;
}point_t;

int* image_processing(void* point);

int main(int argc, char* argv[]) {
	pthread_t* handle;//ドラッグされなかった時の処理
	point_t* p;//情報を渡すための構造体



	//ドラッグされなかった時の処理
	if (argc < 2) {
		printf("ファイルをドラッグして起動してください\n");
		Sleep(5000);
		return 0;
	}


	handle = (pthread_t*)malloc((argc - 1) * sizeof(pthread_t));
	p = (point_t*)malloc((argc - 1) * sizeof(point_t));

	if (handle == NULL) {
		return -1;
	}
	if (p == NULL) {
		return -2;
	}

	for (int i = 1; i < argc; i++) {
		p[i - 1].count = i;
		p[i - 1].argc = argc;
		p[i - 1].argv = argv[i];
		pthread_create(&handle[i - 1], NULL, image_processing, &p[i - 1]);
	}

	for (int i = 1; i < argc; i++) {
		pthread_join(handle[i - 1], NULL);
		printf("進捗%4lf%%\n", (double)100 * (i) / (argc - 1));
		fflush(stdout);
	}
	printf("完了\n");
	Sleep(2000);

}

int* image_processing(void* point) {
	point_t* data = (point_t*)point;

	unsigned char* pixel = NULL;//画像データ格納
	unsigned char* pixel_re = NULL;//画像データ格納
	int width = 0, height = 0, bpp = 0;//ファイル読み込み用
	int re_width = 1280, re_height = 720;
	int idx = 0;

	pixel = stbi_load(data->argv, &width, &height, &bpp, 4);

	//NULL処理
	if (pixel == NULL) {
		printf("%d枚目の画像の読み込みに失敗しました。\n", data->count);
		Sleep(5000);
		return -1;
	} else {
		printf("%d枚目の画像を読み込みました。\n", data->count);
	}
	//処理後のデータを格納するメモリ
	pixel_re = (unsigned char*)malloc((size_t)width * height * 4 * sizeof(unsigned char));

	if (pixel_re == NULL) {
		return -3;
	}

	/*ここからメイン処理*/
	//画像処理
	stbir_resize_uint8(pixel, width, height, 0, pixel_re, re_width, re_height, 0, 4);

	printf("%d枚目の画像を%dx%dにリサイズしました。\n", data->count,re_width,re_height);

	/*ここまでメイン処理*/

	//出力
	stbi_write_png(data->argv, re_width, re_height, 4, pixel_re, 0);

	printf("%d枚目の画像を%dx%dで書き出しました。\n", data->count, re_width, re_height);

	//ファイルサイズ確認
	FILE* fp = NULL;
	size_t image_size = 0;
	while (1) {//ファイルサイズを取得
		fp = fopen(data->argv, "r");
		if (!fp) {
			return -3;
		}
		image_size = _filelengthi64(_fileno(fp));

		fclose(fp);

		printf("%d枚目の画像の現在のサイズ%fMB\n", data->count,(double)image_size/(1024*1024));

		if (image_size < 1024 * 1024 * 2) {//2MBより小さいか判定
			break;
		}

		re_width -= 16;//リサイズ先のサイズを変更
		re_height -= 9;

		//リサイズやり直し
		stbir_resize_uint8(pixel, width, height, 0, pixel_re, re_width, re_height, 0, 4);
		printf("%d枚目の画像を%dx%dにリサイズしました。\n", data->count, re_width, re_height);

		//出力してみる
		stbi_write_png(data->argv, re_width, re_height, 4, pixel_re, 0);
		printf("%d枚目の画像を%dx%dで書き出し直しました。\n", data->count, re_width, re_height);
	}
	//解放
	stbi_image_free(pixel);
	free(pixel_re);
}


