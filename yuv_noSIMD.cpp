/*
本模板仅供参考
*/
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

#define x 1080
#define y 1920
#define ImgSize x*y
#define bound(tem) (tem < 0 ? 0 : (tem > 255 ? 255 : tem))

unsigned char yuv_y_raw[ImgSize];
unsigned char yuv_u_raw[ImgSize / 4];
unsigned char yuv_v_raw[ImgSize / 4];
signed short yuv_y[2][ImgSize];
signed short yuv_u[2][ImgSize / 4];
signed short yuv_v[2][ImgSize / 4];
signed short yuv_y_new[ImgSize];
signed short yuv_u_new[ImgSize / 4];
signed short yuv_v_new[ImgSize / 4];
unsigned char yuv_y_new_raw[ImgSize];
unsigned char yuv_u_new_raw[ImgSize / 4];
unsigned char yuv_v_new_raw[ImgSize / 4];
signed short rgb_r[2][ImgSize];
signed short rgb_g[2][ImgSize];
signed short rgb_b[2][ImgSize];
signed short rgb_r_new[ImgSize];
signed short rgb_g_new[ImgSize];
signed short rgb_b_new[ImgSize];

int i, j, tem;

int total_time = 0, file_time = 0;
clock_t start_clock, finish_clock, file_clock;

double YuvToRgb[3][3] = {1, 0, 1.140,
                         1, -0.394, -0.581,
                         1, 2.032, 0};
double RgbToYuv[3][3] = {0.299, 0.587, 0.114,
                         -0.147, -0.289, 0.436,
                         0.615, -0.515, -0.100};

int process_without_simd(bool pics);
int process_without_simd_int(bool pics);

void readyuv(char* file1, char* file2)
{
	FILE *fp = NULL;

	int freadSize_y = 0, freadSize_u = 0, freadSize_v = 0;

	if((fp = fopen(file1, "rb")) == NULL)
	{
		printf("Open file1 failed.\n");
		return;
	}
	freadSize_y = fread(yuv_y_raw, 1, ImgSize, fp);
	freadSize_u = fread(yuv_u_raw, 1, ImgSize >> 2, fp);
	freadSize_v = fread(yuv_v_raw, 1, ImgSize >> 2, fp);
	fclose(fp);
	if(freadSize_y < ImgSize || freadSize_u < (ImgSize >> 2) || freadSize_v < (ImgSize >> 2))
	{
		printf("Read file1 failed.\n");
		return;
	}
	for(i = 0;i < ImgSize;i++)
	{
		yuv_y[0][i] = yuv_y_raw[i];
	}
	for(i = 0;i < ImgSize / 4;i++)
	{
		yuv_u[0][i] = yuv_u_raw[i];
		yuv_v[0][i] = yuv_v_raw[i];
	}

	if((fp = fopen(file2, "rb")) == NULL)
	{
		printf("Open file2 failed.\n");
		return;
	}
	freadSize_y = fread(yuv_y_raw, 1, ImgSize, fp);
	freadSize_u = fread(yuv_u_raw, 1, ImgSize >> 2, fp);
	freadSize_v = fread(yuv_v_raw, 1, ImgSize >> 2, fp);
	fclose(fp);
	if(freadSize_y < ImgSize || freadSize_u < (ImgSize >> 2) || freadSize_v < (ImgSize >> 2))
	{
		printf("Read file2 failed.\n");
		return;
	}
	for(i = 0;i < ImgSize;i++)
	{
		yuv_y[1][i] = yuv_y_raw[i];
	}
	for(i = 0;i < ImgSize / 4;i++)
	{
		yuv_u[1][i] = yuv_u_raw[i];
		yuv_v[1][i] = yuv_v_raw[i];
	}
}

void writeyuv(FILE *fw)
{
	for(i = 0;i < ImgSize;i++)
	{
		yuv_y_new_raw[i] = yuv_y_new[i];
	}
	for(i = 0;i < ImgSize / 4;i++)
	{
		yuv_u_new_raw[i] = yuv_u_new[i];
		yuv_v_new_raw[i] = yuv_v_new[i];
	}
	fwrite(yuv_y_new_raw, 1, ImgSize, fw);
	fwrite(yuv_u_new_raw, 1, ImgSize >> 2, fw);
	fwrite(yuv_v_new_raw, 1, ImgSize >> 2, fw);

	//fclose(fp);
	return;
}

void yuvToRgb(int index)
{
	int rownum = 0, rownumhalf = 0;
	for(i = 0;i < x;i++)
		for(j = 0;j < y;j++)
		{
			rownum = i * y + j;
			rownumhalf = (i / 2) * (y / 2) + j / 2;

			//R= Y + 1.140 * V
			tem = yuv_y[index][rownum] + (yuv_v[index][rownumhalf] - 128) * YuvToRgb[0][2];
			rgb_r[index][rownum] = bound(tem);

			//G= Y - 0.394 * U - 0.581 * V
			tem = yuv_y[index][rownum] + (yuv_u[index][rownumhalf] - 128) * YuvToRgb[1][1]
									+ (yuv_v[index][rownumhalf] - 128) * YuvToRgb[1][2];
			rgb_g[index][rownum] = bound(tem);

			//B= Y + 2.032 * U
			tem = yuv_y[index][rownum] + (yuv_u[index][rownumhalf] - 128) * YuvToRgb[2][1];
			rgb_b[index][rownum] = bound(tem);
		}
}

void yuvToRgb_int(int index)	// only use char and integer, no float
{
	int rownum = 0, rownumhalf = 0;
	for(i = 0;i < x;i++)
		for(j = 0;j < y;j++)
		{
			rownum = i * y + j;
			rownumhalf = (i / 2) * (y / 2) + j / 2;

			//R = clip(( 298 * C           + 409 * E + 128) >> 8)
			tem = (298 * (yuv_y[index][rownum] - 16) + 409 * (yuv_v[index][rownumhalf] - 128) + 128) >> 8;
			rgb_r[index][rownum] = bound(tem);

			//G = clip(( 298 * C - 100 * D - 208 * E + 128) >> 8)
			tem = (298 * (yuv_y[index][rownum] - 16) - 100 * (yuv_u[index][rownumhalf] - 128)
									- 208 * (yuv_v[index][rownumhalf] - 128) + 128) >> 8;
			rgb_g[index][rownum] = bound(tem);

			//B = clip(( 298 * C + 516 * D           + 128) >> 8)
			tem = (298 * (yuv_y[index][rownum] - 16) + 516 * (yuv_u[index][rownumhalf] - 128) + 128) >> 8;
			rgb_b[index][rownum] = bound(tem);
		}
}

void alpha_rgb(int alpha)
{
	int rownum = 0;
	for(i = 0;i < x;i++)
		for(j = 0;j < y;j++)
		{
			rownum = i * y + j;

			//R’= A * R / 256
			tem = rgb_r[0][rownum] * alpha / 256;
			rgb_r_new[rownum] = bound(tem);

			//G’= A * G / 256			
			tem = rgb_g[0][rownum] * alpha / 256;
			rgb_g_new[rownum] = bound(tem);
			
			//B’= A * B / 256
			tem = rgb_b[0][rownum] * alpha / 256;
			rgb_b_new[rownum] = bound(tem);
		}
}

void alpha_rgb_double(int alpha)
{
	int rownum = 0;
	for(i = 0;i < x;i++)
		for(j = 0;j < y;j++)
		{
			rownum = i * y + j;

			//R’= (A * R1 + (256-A) * R2) / 256
			tem = (rgb_r[0][rownum] * alpha + rgb_r[1][rownum] * (256 - alpha)) / 256;
			rgb_r_new[rownum] = bound(tem);

			//G’= (A * G1 + (256-A) * G2) / 256
			tem = (rgb_g[0][rownum] * alpha + rgb_g[1][rownum] * (256 - alpha)) / 256;
			rgb_g_new[rownum] = bound(tem);
			
			//B’= (A * B1 + (256-A) * B2) / 256
			tem = (rgb_b[0][rownum] * alpha + rgb_b[1][rownum] * (256 - alpha)) / 256;
			rgb_b_new[rownum] = bound(tem);
		}
}

void rgbToYuv()
{
	int rownum = 0, rownumhalf = 0;
	for(i = 0;i < x;i++)
		for(j = 0;j < y;j++)
		{
			rownum = i * y + j;

			//Y= 0.299 * R + 0.587 * G + 0.114 * B
			tem = rgb_r_new[rownum] * RgbToYuv[0][0] + rgb_g_new[rownum] * RgbToYuv[0][1] + rgb_b_new[rownum] * RgbToYuv[0][2];
			yuv_y_new[rownum] = bound(tem);

			if(i % 2 == 1 && j % 2 == 1)
			{
				rownumhalf = (y / 2) * (i / 2) + (j / 2);

				//U= -0.147 * R - 0.289 * G + 0.436 * B = 0.492 * (B- Y)
				tem = rgb_r_new[rownum] * RgbToYuv[1][0] + rgb_g_new[rownum] * RgbToYuv[1][1] + rgb_b_new[rownum] * RgbToYuv[1][2] + 128;
				yuv_u_new[rownumhalf] = bound(tem);
				
				//V= 0.615 * R - 0.515 * G - 0.100 * B = 0.877 * (R- Y)
				tem = rgb_r_new[rownum] * RgbToYuv[2][0] + rgb_g_new[rownum] * RgbToYuv[2][1] + rgb_b_new[rownum] * RgbToYuv[2][2] + 128;
				yuv_v_new[rownumhalf] = bound(tem);
			}
		}

}

void rgbToYuv_int()	// rgb to yuv using integer
{	
	int rownum = 0, rownumhalf = 0;
	for(i = 0;i < x;i++)
		for(j = 0;j < y;j++)
		{
			rownum = i * y + j;
			// Y = ( (  66 * R + 129 * G +  25 * B + 128) >> 8) +  16
			tem = ((66 * rgb_r_new[rownum] + 129 * rgb_g_new[rownum] + 25 * rgb_b_new[rownum] + 128) >> 8) + 16;
			yuv_y_new[rownum] = bound(tem);

			if(i % 2 == 1 && j % 2 == 1)
			{
				rownumhalf = (y / 2) * (i / 2) + (j / 2);

				//U = ( ( -38 * R -  74 * G + 112 * B + 128) >> 8) + 128
				tem = ((-38 * rgb_r_new[rownum] - 74 * rgb_g_new[rownum] + 112 * rgb_b_new[rownum] + 128) >> 8) + 128;
				yuv_u_new[rownumhalf] = bound(tem);

				//V = ( ( 112 * R -  94 * G -  18 * B + 128) >> 8) + 128
				tem = ((112 * rgb_r_new[rownum] - 94 * rgb_g_new[rownum] - 18 * rgb_b_new[rownum] + 128) >> 8) + 128;
				yuv_v_new[rownumhalf] = bound(tem);
			}
		}
}

int main(){	
	readyuv((char*)"demo/dem1.yuv", (char*)"demo/dem2.yuv");
	int time_single = process_without_simd(false);
	int time_double = process_without_simd(true);
	int time_single_int = process_without_simd_int(false);
	int time_double_int = process_without_simd_int(true);
	return 0;
}



//下面的4个函数应该统计出图像处理的时间;
//函数参数和返回值可以需要自己定.
int process_without_simd(bool pics){
	start_clock = clock();
	/*处理过程*/
	FILE *fw;
	if(!pics)	// one picture
	{
		if((fw = fopen("demo/test_noSIMD_one.yuv", "wb")) == NULL)
		{
			printf("Open write file1(no SIMD, one picture) failed.\n");
			return 0;
		}
		file_clock = clock();
		yuvToRgb(0);
		for(int a = 1;a < 255;a += 3)
		{
			alpha_rgb(a);
			rgbToYuv();
			writeyuv(fw);
		}
	}
	else			// two pictures blending
	{
		if((fw = fopen("demo/test_noSIMD_two.yuv", "wb")) == NULL)
		{
			printf("Open write file2(no SIMD, two pictures) failed.\n");
			return 0;
		}
		file_clock = clock();
		yuvToRgb(0);
		yuvToRgb(1);
		for(int a = 1;a < 255;a += 3)
		{
			alpha_rgb_double(a);
			rgbToYuv();
			writeyuv(fw);
		}
	}
	fclose(fw);
	finish_clock = clock();

	file_time = file_clock - start_clock;
	total_time = finish_clock - start_clock;
	if(!pics)	// one picture
	{
		cout << "TEST: no SIMD, one picture, both float and integer" << endl;
	}
	else
	{
		cout << "TEST: no SIMD, two picture, both float and integer" << endl;
	}
	cout << "The time costs on open file is " << file_time << "/" << CLOCKS_PER_SEC << "(s)" << endl;
	cout << "The time costs on total process is " << total_time << "/" << CLOCKS_PER_SEC << "(s)" << endl << endl;
	return total_time;
}

int process_without_simd_int(bool pics){
	start_clock = clock();
	/*处理过程*/
	FILE *fw;
	if(!pics)	// one picture
	{
		if((fw = fopen("demo/test_noSIMD_int_one.yuv", "wb")) == NULL)
		{
			printf("Open write file3(no SIMD, one picture, only integer) failed.\n");
			return 0;
		}
		file_clock = clock();
		yuvToRgb_int(0);
		for(int a = 1;a < 255;a += 3)
		{
			alpha_rgb(a);
			rgbToYuv_int();
			writeyuv(fw);
		}
	}
	else			// two pictures blending
	{
		if((fw = fopen("demo/test_noSIMD_int_two.yuv", "wb")) == NULL)
		{
			printf("Open write file4(no SIMD, two pictures, only integer) failed.\n");
			return 0;
		}
		file_clock = clock();
		yuvToRgb_int(0);
		yuvToRgb_int(1);
		for(int a = 1;a < 255;a += 3)
		{
			alpha_rgb_double(a);
			rgbToYuv_int();
			writeyuv(fw);
		}
	}
	fclose(fw);
	finish_clock = clock();

	file_time = file_clock - start_clock;
	total_time = finish_clock - start_clock;
	if(!pics)	// one picture
	{
		cout << "TEST: no SIMD, one picture, only integer" << endl;
	}
	else
	{
		cout << "TEST: no SIMD, two picture, only integer" << endl;
	}
	cout << "The time costs on open file is " << file_time << "/" << CLOCKS_PER_SEC << "(s)" << endl;
	cout << "The time costs on total process is " << total_time << "/" << CLOCKS_PER_SEC << "(s)" << endl << endl;
	return total_time;
}