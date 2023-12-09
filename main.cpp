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



unsigned char* pixel = nullptr;//�摜�f�[�^������
int in_image_x = 0, in_image_y = 0, in_image_bpp = 0;
std::array<char, 1024> file_name = {};//���̓t�@�C���������Ă���

int file_no = 1;//�t�@�C���ԍ����蓖�ėp

const int default_bpp = 3;
const size_t limit_size = 1024 * 1024 * 2;




class image_data {//�摜�f�[�^���Ǘ�����N���X
public:
	//���T�C�Y��摜�֌W
	int out_image_x = 0;
	int out_image_y = 0;

private:
	//���T�C�Y��摜�֌W
	unsigned char* out_pixel = nullptr;

	int image_no = 0;//�t�@�C���ԍ�������
	size_t file_size = 0;

	std::array<char, 1024> name_buf = {};//���t�@�C����������

public:

	void resize(void) {
		try {//���T�C�Y��̃��������m��
			out_pixel = new unsigned char[out_image_x * out_image_y * default_bpp * sizeof(unsigned char)];
		}
		catch (std::bad_alloc) {
			ERROR_PRINT("MEM_ERROR", -1)
		}

		stbir_resize_uint8(pixel, in_image_x, in_image_y, 0, out_pixel, out_image_x, out_image_y, 0, default_bpp);
	}

	void image_output(void) {

		name_buf.fill('\0');

		sprintf(&name_buf.front(), "%s%03d", &file_name.front(), image_no);//�����ŏo��

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

	_inline size_t return_filesize(void) {//file_size��ǂ݂�����p�ɂ���֐�
		return file_size;
	}

	_inline int set_fileno(int no) {//file_no����x�����������݂���֐�
		if ((no > 0) && (image_no == 0)) {
			image_no = no;
			return 0;
		} else {
			return -1;
		}
	}

	void image_rename(void) {
		std::array<char, 1024> str_buf = {};//���T�C�Y��t�@�C����������

		str_buf = name_buf;

		str_buf[strlen(&str_buf.front()) + 1 - 7] = '\0';//.pngxxx������
		sprintf(&str_buf.front(), "%s_2MB.png", &str_buf.front());//_2MB.png��t������

		rename(&name_buf.front(), &str_buf.front());
	}

	~image_data() {
		if (out_pixel) {//�܂��������ĂȂ��Ȃ���
			delete[] out_pixel;
		}
		remove(&name_buf.front());//�ꎞ�t�@�C��������
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


	//�h���b�O����Ȃ��������̏���
	if (argc < 2) {
		printf("ver:1.1\n");
		printf("�t�@�C�����h���b�O���ċN�����Ă�������\n");
		Sleep(5000);
		return 0;
	}

	for (int i = 1; i < argc; i++) {

		//�������烁�C������
		size_t in_image_size = get_filesize(argv[i]);
		if (in_image_size < limit_size) {//����2MB�ȉ��̕��̓X�L�b�v
			printf("%d���ڂ�2MB�ȉ��̉摜���X�L�b�v���܂����B\n", i);
			continue;
		}

		strcpy(&file_name.front(), argv[i]);//�t�@�C�������O���[�o���ϐ��ɃR�s�[

		pixel = stbi_load(argv[i], &in_image_x, &in_image_y, &in_image_bpp, 3);

		if (!pixel) {
			printf("%d���ڂ̉摜�̓ǂݍ��݂Ɏ��s���܂����B\n", i);
			continue;
		}
		printf("%d���ڂ̉摜��ǂݍ��݂܂����B\n", i);

		int core_num = std::thread::hardware_concurrency();

		int image_notch = in_image_x / 16;

		file_no = 0;//���t�@�C�����J�E���^�J���X�g�΍�


		std::vector<image_data> image_vec(image_notch);//�T�C�Y���L�^���Ă����z��

		std::vector<std::thread*> thread_list(image_notch);//�X���b�h�����L�^����z��

		int th_count_create = 0;//���������J�E���g(�쐬��)
		int th_count_join = 0;//���������J�E���g(join��)
		int image_vec_num = 0;//�ŏI�I�ɏ���������
		int limit_clear_no = ~0;//��𖞂������z��̔ԍ�

		while (1) {//�ꎟ�T��
			for (int th_j = 0; th_j < core_num; th_j++) {//�v���Z�b�T�Ԃ�X���b�h����
				if (th_count_create < image_notch) {//���T�C�Y�l���}�C�i�X���Ȃ��悤�Ɋm�F
					image_vec[th_count_create].out_image_x = in_image_x - (16 * th_count_create);
					image_vec[th_count_create].out_image_y = in_image_y - (9 * th_count_create);
					try {
						std::thread* p_thread = new std::thread(image_resize, &image_vec[th_count_create]);//�X���b�h����
						thread_list[th_count_create] = p_thread;//�X���b�h�����L�^
						th_count_create = th_count_create + core_num + 1;//�v���Z�b�T�����J���ĒT��
					}
					catch (std::bad_alloc) {
						ERROR_PRINT("MEM_ERROR", -1)
					}
				}
			}

			for (int th_j = 0; th_j < core_num; th_j++) {//�X���b�h�������I���̂�҂�
				if (th_count_join < image_notch) {

					printf("\033[1K\033[0G�t�@�C�����o�͂��Ă����܂��ɒT�����B�i��%d%%�B", (int)(th_count_join * 100) / image_notch);
					fflush(stdout);

					(*thread_list[th_count_join]).join();

					if ((image_vec[th_count_join].return_filesize() < limit_size) && (limit_clear_no == ~0)) {
						if (th_count_join == 0) {
							limit_clear_no = th_count_join;//������������𖞂������Ƃ��̏���
						} else {
							limit_clear_no = th_count_join - (core_num + 1);//�����𖞂������O�̏ꏊ���L�^
						}
					}

					th_count_join = th_count_join + core_num + 1;
				}
			}

			if (limit_clear_no < 0xffffffff) {
				break;
			}
		}

		printf("\033[1K\033[0G�t�@�C�����o�͂��Ă����܂��ɒT�����B�i��%d%%�B\n", 100);
		fflush(stdout);

		th_count_create = limit_clear_no+1;//���T���G���A���w��
		th_count_join = limit_clear_no+1;

		//��������񎟒T��
		for (int th_j = 0; th_j < core_num; th_j++) {//�v���Z�b�T�Ԃ�X���b�h����
			if (th_count_create < image_notch) {//���T�C�Y�l���}�C�i�X���Ȃ��悤�Ɋm�F
				image_vec[th_count_create].out_image_x = in_image_x - (16 * th_count_create);
				image_vec[th_count_create].out_image_y = in_image_y - (9 * th_count_create);
				try {
					std::thread* p_thread = new std::thread(image_resize, &image_vec[th_count_create]);//�X���b�h����
					thread_list[th_count_create] = p_thread;//�X���b�h�����L�^
					th_count_create++;
				}
				catch (std::bad_alloc) {
					ERROR_PRINT("MEM_ERROR", -1)
				}
			}
		}

		for (int th_j = 0; th_j < core_num; th_j++) {//�X���b�h�������I���̂�҂�
			if (th_count_join < image_notch) {

				printf("\033[1K\033[0G�t�@�C�����o�͂��ďڍׂɒT�����B�i��%d%%�B", (int)(th_count_join * 100) / image_notch);
				fflush(stdout);

				(*thread_list[th_count_join]).join();
				th_count_join++;
			}
		}


		printf("\033[1K\033[0G�t�@�C�����o�͂��ďڍׂɒT�����B�i��%d%%�B\n", 100);
		fflush(stdout);

		stbi_image_free(pixel);


		for (int j = limit_clear_no; j < limit_clear_no + core_num + 1; j++) {//�������ł����΂�傫�����̂�I�����ă��l�[��
			printf("\033[1K\033[0G�����ɍ����摜��T�����B�i��%d%%�B", (int)(j * 100) / image_notch);
			fflush(stdout);
			if (image_vec[j].return_filesize() < limit_size) {
				image_vec[j].image_rename();
				printf("\033[1K\033[0G�����ɍ����摜��T�����B�i��%d%%�B\n", 100);
				fflush(stdout);
				printf("%d���ڂ̉摜��%3.2fMB�ŏ����o���܂����B\n", i, (double)image_vec[j].return_filesize() * 2 / (limit_size));
				fflush(stdout);
				break;
			}
		}


	}
	printf("�������܂����B�����p���肪�Ƃ��������܂����B\n");
	fflush(stdout);
	Sleep(5000);
}


