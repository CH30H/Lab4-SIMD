/*
本模板仅供参考
*/
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <mmintrin.h>
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

__m64 *mR, *mG, *mB, *mY, *mU, *mV;
__m64 num1, num2, num3, num_Y, num_U, num_V, num_C, num_D, num_E, num_A,
	num_R, num_G, num_B, res1, res2, res3, res4;
signed short *pY, *pU, *pV, *pR, *pG, *pB;

int i, j, tem;

int total_time = 0, file_time = 0;
clock_t start_clock, finish_clock, file_clock;

double YuvToRgb[3][3] = {1, 0, 1.140,
                         1, -0.394, -0.581,
                         1, 2.032, 0};
double RgbToYuv[3][3] = {0.299, 0.587, 0.114,
                         -0.147, -0.289, 0.436,
                         0.615, -0.515, -0.100};

int process_with_all_mmx(bool pics);
int process_with_part_mmx(bool pics);

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
	for(i = 0;i < x;i++)
		for(j = 0;j < y;j++)
		{
			//R= Y + 1.140 * V
			tem = yuv_y[index][i * y + j] + (yuv_v[index][(i / 2) * (y / 2) + j / 2] - 128) * YuvToRgb[0][2];
			rgb_r[index][i * y + j] = bound(tem);

			//G= Y - 0.394 * U - 0.581 * V
			tem = yuv_y[index][i * y + j] + (yuv_u[index][(i / 2) * (y / 2) + j / 2] - 128) * YuvToRgb[1][1]
									+ (yuv_v[index][(i / 2) * (y / 2) + j / 2] - 128) * YuvToRgb[1][2];
			rgb_g[index][i * y + j] = bound(tem);

			//B= Y + 2.032 * U
			tem = yuv_y[index][i * y + j] + (yuv_u[index][(i / 2) * (y / 2) + j / 2] - 128) * YuvToRgb[2][1];
			rgb_b[index][i * y + j] = bound(tem);
		}
}

void yuvToRgb_mmx(int index)
{
	int rownum = 0, rownumhalf = 0;

	//point
	pR = rgb_r[index];
	pG = rgb_g[index];
	pB = rgb_b[index];

	for(i = 0;i < x;i++)
		for(j = 0;j < y;j += 4)
		{
			mR = (__m64*)pR;
			mG = (__m64*)pG;
			mB = (__m64*)pB;

			rownum = i * y + j;
			rownumhalf = (i / 2) * (y / 2) + j / 2;

			num_Y = _mm_set_pi16((short)yuv_y[index][rownum + 3], (short)yuv_y[index][rownum + 2], (short)yuv_y[index][rownum + 1], (short)yuv_y[index][rownum]);
			num_U = _mm_set_pi16((short)yuv_u[index][rownumhalf + 1], (short)yuv_u[index][rownumhalf + 1], (short)yuv_u[index][rownumhalf], (short)yuv_u[index][rownumhalf]);
			num_V = _mm_set_pi16((short)yuv_v[index][rownumhalf + 1], (short)yuv_v[index][rownumhalf + 1], (short)yuv_v[index][rownumhalf], (short)yuv_v[index][rownumhalf]);
			
			num1 = _mm_set1_pi16((short)16);
			num2 = _mm_set1_pi16((short)128);

			num_C = _m_psubsw(num_Y, num1);	// C = Y - 16
			num_D = _m_psubsw(num_U, num2);	// D = U - 128
			num_E = _m_psubsw(num_V, num2);	// E = V - 128

			//R = clip(( 298 * C           + 409 * E + 128) >> 8)
			num1 = _mm_set1_pi16((short)298);
			num1 = _m_psllwi(num1, 2);
			num2 = _m_psllwi(num_C, 6);
			res1 = _m_pmulhw(num1, num2);	// use the high 16-bits, means ">> 16"

			num1 = _mm_set1_pi16((short)409);
			num1 = _m_psllwi(num1, 2);
			num2 = _m_psllwi(num_E, 6);
			res2 = _m_pmulhw(num1, num2);	// use the high 16-bits, means ">> 16"

			*mR = _m_paddsw(res1, res2);
			
			//G = clip(( 298 * C - 100 * D - 208 * E + 128) >> 8)
			num1 = _mm_set1_pi16((short)298);
			num1 = _m_psllwi(num1, 2);
			num2 = _m_psllwi(num_C, 6);
			res1 = _m_pmulhw(num1, num2);	// use the high 16-bits, means ">> 16"
			
			num1 = _mm_set1_pi16((short)100);
			num1 = _m_psllwi(num1, 8);
			//num2 = _m_psllwi(num_E, 6);
			res2 = _m_pmulhw(num1, num_D);	// use the high 16-bits, means ">> 16"

			num1 = _mm_set1_pi16((short)208);
			num1 = _m_psllwi(num1, 2);
			num2 = _m_psllwi(num_E, 6);
			res3 = _m_pmulhw(num1, num2);	// use the high 16-bits, means ">> 16"

			res4 = _m_psubsw(res1, res2);
			*mG = _m_psubsw(res4, res3);
			
			//B = clip(( 298 * C + 516 * D           + 128) >> 8)
			num1 = _mm_set1_pi16((short)298);
			num1 = _m_psllwi(num1, 2);
			num2 = _m_psllwi(num_C, 6);
			res1 = _m_pmulhw(num1, num2);	// use the high 16-bits, means ">> 16"
			
			num1 = _mm_set1_pi16((short)516);
			num1 = _m_psllwi(num1, 1);
			num2 = _m_psllwi(num_D, 7);
			res2 = _m_pmulhw(num1, num2);	// use the high 16-bits, means ">> 16"

			*mB = _m_paddsw(res1, res2);

			pR += 4;pG += 4;pB += 4;
		}
}

void alpha_rgb_mmx(int alpha)
{
	int rownum = 0;
	pR = rgb_r_new;
	pG = rgb_g_new;
	pB = rgb_b_new;

	num_A = _mm_set1_pi16((short)alpha);	// < 255
	num2 = _m_psllwi(num_A, 4);

	for(i = 0;i < x;i++)
		for(j = 0;j < y;j += 4)
		{
			mR = (__m64*)pR;
			mG = (__m64*)pG;
			mB = (__m64*)pB;

			rownum = i * y + j;

			//R’= A * R / 256
			num1 = _mm_set_pi16(rgb_r[0][rownum + 3], rgb_r[0][rownum + 2], rgb_r[0][rownum + 1], rgb_r[0][rownum]);
			num1 = _m_psllwi(num1, 4);
			*mR = _m_pmulhw(num1, num2);

			//G’= A * G / 256
			num1 = _mm_set_pi16(rgb_g[0][rownum + 3], rgb_g[0][rownum + 2], rgb_g[0][rownum + 1], rgb_g[0][rownum]);
			num1 = _m_psllwi(num1, 4);
			*mG = _m_pmulhw(num1, num2);

			//B’= A * B / 256
			num1 = _mm_set_pi16(rgb_b[0][rownum + 3], rgb_b[0][rownum + 2], rgb_b[0][rownum + 1], rgb_b[0][rownum]);
			num1 = _m_psllwi(num1, 4);
			*mB = _m_pmulhw(num1, num2);

			pR += 4;pG += 4;pB += 4;
		}
}

void alpha_rgb_double_mmx(int alpha)
{
	int rownum = 0;
	pR = rgb_r_new;
	pG = rgb_g_new;
	pB = rgb_b_new;
	
	num_A = _mm_set1_pi16((short)alpha);	// < 255
	num2 = _m_psllwi(num_A, 4);

	for(i = 0;i < x;i++)
		for(j = 0;j < y;j += 4)
		{
			mR = (__m64*)pR;
			mG = (__m64*)pG;
			mB = (__m64*)pB;

			rownum = i * y + j;
			//R’= (A * R1 + (256-A) * R2) / 256
			num_R = _mm_set_pi16(rgb_r[0][rownum + 3], rgb_r[0][rownum + 2], rgb_r[0][rownum + 1], rgb_r[0][rownum]);
			res1 = _mm_set_pi16(rgb_r[1][rownum + 3], rgb_r[1][rownum + 2], rgb_r[1][rownum + 1], rgb_r[1][rownum]);
			num1 = _m_psubsw(num_R, res1);
			num1 = _m_psllwi(num1, 4);
			res3 = _m_pmulhw(num1, num2);
			*mR = _m_paddsw(res3, res1);

			//G’= (A * G1 + (256-A) * G2) / 256
			num_G = _mm_set_pi16(rgb_g[0][rownum + 3], rgb_g[0][rownum + 2], rgb_g[0][rownum + 1], rgb_g[0][rownum]);
			res1 = _mm_set_pi16(rgb_g[1][rownum + 3], rgb_g[1][rownum + 2], rgb_g[1][rownum + 1], rgb_g[1][rownum]);
			num1 = _m_psubsw(num_G, res1);
			num1 = _m_psllwi(num1, 4);
			res3 = _m_pmulhw(num1, num2);
			*mG = _m_paddsw(res3, res1);

			//B’= (A * B1 + (256-A) * B2) / 256
			num_B = _mm_set_pi16(rgb_b[0][rownum + 3], rgb_b[0][rownum + 2], rgb_b[0][rownum + 1], rgb_b[0][rownum]);
			res1 = _mm_set_pi16(rgb_b[1][rownum + 3], rgb_b[1][rownum + 2], rgb_b[1][rownum + 1], rgb_b[1][rownum]);
			num1 = _m_psubsw(num_B, res1);
			num1 = _m_psllwi(num1, 4);
			res3 = _m_pmulhw(num1, num2);
			*mB = _m_paddsw(res3, res1);
			
			pR += 4;pG += 4;pB += 4;
		}
}

void rgbToYuv()
{
	int point = 0;
	for(i = 0;i < x;i++)
		for(j = 0;j < y;j++)
		{
			//Y= 0.299 * R + 0.587 * G + 0.114 * B
			tem = rgb_r_new[i * y + j] * RgbToYuv[0][0] + rgb_g_new[i * y + j] * RgbToYuv[0][1] + rgb_b_new[i * y + j] * RgbToYuv[0][2];
			yuv_y_new[i * y + j] = bound(tem);

			if(i % 2 == 1 && j % 2 == 1)
			{
				point = (y / 2) * (i / 2) + (j / 2);

				//U= -0.147 * R - 0.289 * G + 0.436 * B = 0.492 * (B- Y)
				tem = rgb_r_new[i * y + j] * RgbToYuv[1][0] + rgb_g_new[i * y + j] * RgbToYuv[1][1] + rgb_b_new[i * y + j] * RgbToYuv[1][2] + 128;
				yuv_u_new[point] = bound(tem);
				
				//V= 0.615 * R - 0.515 * G - 0.100 * B = 0.877 * (R- Y)
				tem = rgb_r_new[i * y + j] * RgbToYuv[2][0] + rgb_g_new[i * y + j] * RgbToYuv[2][1] + rgb_b_new[i * y + j] * RgbToYuv[2][2] + 128;
				yuv_v_new[point] = bound(tem);
			}
		}

}

void rgbToYuv_mmx()
{
	pY = yuv_y_new;
	pU = yuv_u_new;
	pV = yuv_v_new;
	
	int rownum = 0;

	for(i = 0;i < x;i++)
		for(j = 0;j < y;j += 4)
		{
			rownum = i * y + j;
			mY = (__m64*)pY;

			// Y = ( (  66 * R + 129 * G +  25 * B + 128) >> 8) +  16
			num_R = _mm_set_pi16(rgb_r_new[rownum + 3], rgb_r_new[rownum + 2], rgb_r_new[rownum + 1], rgb_r_new[rownum]);
			num1 = _mm_set1_pi16((short)66);
			num1 = _m_psllwi(num1, 8);
			res1 = _m_pmulhw(num1, num_R);

			num_G = _mm_set_pi16(rgb_g_new[rownum + 3], rgb_g_new[rownum + 2], rgb_g_new[rownum + 1], rgb_g_new[rownum]);
			num1 = _mm_set1_pi16((short)129);
			num1 = _m_psllwi(num1, 6);
			num2 = _m_psllwi(num_G, 2);
			res2 = _m_pmulhw(num1, num2);

			num_B = _mm_set_pi16(rgb_b_new[rownum + 3], rgb_b_new[rownum + 2], rgb_b_new[rownum + 1], rgb_b_new[rownum]);
			num1 = _mm_set1_pi16((short)25);
			num1 = _m_psllwi(num1, 8);
			res3 = _m_pmulhw(num1, num_B);

			res4 = _mm_set1_pi16((short)16);

			num1 = _m_paddsw(res1, res2);
			num2 = _m_paddsw(res3, res4);
			*mY = _m_paddsw(num1, num2);

			if(i % 2 == 0 && j % 8 == 0)
			{
				mU = (__m64*)pU;
				mV = (__m64*)pV;

				num_R = _mm_set_pi16(rgb_r_new[rownum + 6], rgb_r_new[rownum + 4], rgb_r_new[rownum + 2], rgb_r_new[rownum]);
				num_G = _mm_set_pi16(rgb_g_new[rownum + 6], rgb_g_new[rownum + 4], rgb_g_new[rownum + 2], rgb_g_new[rownum]);
				num_B = _mm_set_pi16(rgb_b_new[rownum + 6], rgb_b_new[rownum + 4], rgb_b_new[rownum + 2], rgb_b_new[rownum]);
				
				// U = ( ( -38 * R -  74 * G + 112 * B + 128) >> 8) + 128
				num1 = _mm_set1_pi16((short)38);
				num1 = _m_psllwi(num1, 8);
				res1 = _m_pmulhw(num1, num_R);

				num1 = _mm_set1_pi16((short)74);
				num1 = _m_psllwi(num1, 8);
				res2 = _m_pmulhw(num1, num_G);

				num1 = _mm_set1_pi16((short)112);
				num1 = _m_psllwi(num1, 8);
				res3 = _m_pmulhw(num1, num_B);
				
				res4 = _mm_set1_pi16((short)128);

				num1 = _m_paddsw(res1, res2);
				num2 = _m_paddsw(res3, res4);
				*mU = _m_psubsw(num2, num1);

				// V = ( ( 112 * R -  94 * G -  18 * B + 128) >> 8) + 128
				num1 = _mm_set1_pi16((short)112);
				num1 = _m_psllwi(num1, 8);
				res1 = _m_pmulhw(num1, num_R);

				num1 = _mm_set1_pi16((short)94);
				num1 = _m_psllwi(num1, 8);
				res2 = _m_pmulhw(num1, num_G);

				num1 = _mm_set1_pi16((short)18);
				num1 = _m_psllwi(num1, 8);
				res3 = _m_pmulhw(num1, num_B);
				
				res4 = _mm_set1_pi16((short)128);

				num1 = _m_paddsw(res1, res4);
				num2 = _m_paddsw(res2, res3);
				*mV = _m_psubsw(num1, num2);

				pU+=4;pV += 4;
			}
			pY += 4;
		}
}

int main(){	
	readyuv((char*)"demo/dem1.yuv", (char*)"demo/dem2.yuv");
	int time_single_all = process_with_all_mmx(false);
	int time_double_all = process_with_all_mmx(true);
	int time_single_part = process_with_part_mmx(false);
	int time_double_part = process_with_part_mmx(true);
	return 0;
}

//下面的4个函数应该统计出图像处理的时间;
//函数参数和返回值可以需要自己定.
int process_with_all_mmx(bool pics){
	start_clock = clock();
	/*处理过程*/
	FILE *fw;
	if(!pics)	// one picture
	{
		if((fw = fopen("demo/test_mmx_all_one.yuv", "wb")) == NULL)
		{
			printf("Open write file1(mmx, one picture) failed.\n");
			return 0;
		}
		file_clock = clock();
		yuvToRgb_mmx(0);
		for(int a = 1;a < 255;a += 3)
		{
			alpha_rgb_mmx(a);
			rgbToYuv_mmx();
			writeyuv(fw);
		}
	}
	else			// two pictures blending
	{
		if((fw = fopen("demo/test_mmx_all_two.yuv", "wb")) == NULL)
		{
			printf("Open write file2(mmx, two pictures) failed.\n");
			return 0;
		}
		file_clock = clock();
		yuvToRgb_mmx(0);
		yuvToRgb_mmx(1);
		for(int a = 1;a < 255;a += 3)
		{
			alpha_rgb_double_mmx(a);
			rgbToYuv_mmx();
			writeyuv(fw);
		}
	}
	fclose(fw);
	finish_clock = clock();

	file_time = file_clock - start_clock;
	total_time = finish_clock - start_clock;
	if(!pics)	// one picture
	{
		cout << "TEST: mmx, one picture, only integer" << endl;
	}
	else
	{
		cout << "TEST: mmx, two picture, only integer" << endl;
	}
	cout << "The time costs on open file is " << file_time << "/" << CLOCKS_PER_SEC << "(s)" << endl;
	cout << "The time costs on total process is " << total_time << "/" << CLOCKS_PER_SEC << "(s)" << endl << endl;
	return total_time;
}

int process_with_part_mmx(bool pics)
{
	start_clock = clock();
	/*处理过程*/
	FILE *fw;
	if(!pics)	// one picture
	{
		if((fw = fopen("demo/test_mmx_part_one.yuv", "wb")) == NULL)
		{
			printf("Open write file1(mmx, one picture) failed.\n");
			return 0;
		}
		file_clock = clock();
		yuvToRgb(0);
		for(int a = 1;a < 255;a += 3)
		{
			alpha_rgb_mmx(a);
			rgbToYuv();
			writeyuv(fw);
		}
	}
	else			// two pictures blending
	{
		if((fw = fopen("demo/test_mmx_part_two.yuv", "wb")) == NULL)
		{
			printf("Open write file2(mmx, two pictures) failed.\n");
			return 0;
		}
		file_clock = clock();
		yuvToRgb(0);
		yuvToRgb(1);
		for(int a = 1;a < 255;a += 3)
		{
			alpha_rgb_double_mmx(a);
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
		cout << "TEST: mmx, one picture, both float and integer" << endl;
	}
	else
	{
		cout << "TEST: mmx, two picture, both float and integer" << endl;
	}
	cout << "The time costs on open file is " << file_time << "/" << CLOCKS_PER_SEC << "(s)" << endl;
	cout << "The time costs on total process is " << total_time << "/" << CLOCKS_PER_SEC << "(s)" << endl << endl;
	return total_time;
}