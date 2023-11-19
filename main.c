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
#include<pthread.h>
#include<io.h>