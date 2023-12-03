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
	pthread_t* handle;//�h���b�O����Ȃ��������̏���
	point_t* p;//����n�����߂̍\����



	//�h���b�O����Ȃ��������̏���
	if (argc < 2) {
		printf("ver:0.3\n");
		printf("�t�@�C�����h���b�O���ċN�����Ă�������\n");
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
		printf("�i��%4lf%%\n", (double)100 * (i) / (argc - 1));
		fflush(stdout);
	}
	printf("�������܂����B�����p���肪�Ƃ��������܂��B\n");
	Sleep(2000);

}

int* image_processing(void* point) {
	point_t* data = (point_t*)point;

	unsigned char* pixel = NULL;//�摜�f�[�^�i�[
	unsigned char* pixel_re = NULL;//�摜�f�[�^�i�[
	int width = 0, height = 0, bpp = 0;//�t�@�C���ǂݍ��ݗp
	int re_width = 1280, re_height = 720;
	int idx = 0;

	pixel = stbi_load(data->argv, &width, &height, &bpp, 3);

	//NULL����
	if (pixel == NULL) {
		printf("%d���ڂ̉摜�̓ǂݍ��݂Ɏ��s���܂����B\n", data->count);
		Sleep(5000);
		return -1;
	} else {
		printf("%d���ڂ̉摜��ǂݍ��݂܂����B\n", data->count);
	}
	//������̃f�[�^���i�[���郁����
	if ((size_t)width * height > 1280 * 720) {//�摜��1280x720��菬�����ƃI�[�o�[��������΍�
		pixel_re = (unsigned char*)malloc((size_t)width * height * 3 * sizeof(unsigned char));
	} else {
		pixel_re = (unsigned char*)malloc((size_t)1280 * 720 * 3 * sizeof(unsigned char));
	}


	if (pixel_re == NULL) {
		return -3;
	}

	/*�������烁�C������*/
	//�摜����
	stbir_resize_uint8(pixel, width, height, 0, pixel_re, re_width, re_height, 0, 3);

	printf("%d���ڂ̉摜��%dx%d�Ƀ��T�C�Y���܂����B\n", data->count, re_width, re_height);

	/*�����܂Ń��C������*/

	//�o��
	char filename_buf[1024];
	strcpy(filename_buf, data->argv);
	filename_buf[strlen(filename_buf) + 1 - 4];
	sprintf(filename_buf, "%s_2MB.png", filename_buf);


	stbi_write_png(filename_buf, re_width, re_height, 3, pixel_re, 0);

	printf("%d���ڂ̉摜��%dx%d�ŏ����o���܂����B\n", data->count, re_width, re_height);

	//�t�@�C���T�C�Y�m�F
	FILE* fp = NULL;
	size_t image_size = 0;
	while (1) {//�t�@�C���T�C�Y���擾
		fp = fopen(filename_buf, "r");
		if (!fp) {
			return -3;
		}
		image_size = _filelengthi64(_fileno(fp));

		fclose(fp);

		printf("%d���ڂ̉摜�̌��݂̃T�C�Y%3.2fMB\n", data->count, (double)image_size / (1024 * 1024));

		if (image_size < 1024 * 1024 * 2) {//2MB��菬����������
			break;
		}

		re_width -= 16;//���T�C�Y��̃T�C�Y��ύX
		re_height -= 9;

		//���T�C�Y��蒼��
		stbir_resize_uint8(pixel, width, height, 0, pixel_re, re_width, re_height, 0, 3);
		printf("%d���ڂ̉摜��%dx%d�Ƀ��T�C�Y���܂����B\n", data->count, re_width, re_height);

		//�o�͂��Ă݂�
		stbi_write_png(filename_buf, re_width, re_height, 3, pixel_re, 0);
		printf("%d���ڂ̉摜��%dx%d�ŏ����o�������܂����B\n", data->count, re_width, re_height);
	}
	//���
	stbi_image_free(pixel);
	free(pixel_re);
}


