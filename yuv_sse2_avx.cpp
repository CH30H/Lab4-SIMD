/*
本模板仅供参考
*/
#include <iostream>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>
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
signed short yuv_y_new[ImgSize] = {0};
signed short yuv_u_new[ImgSize / 4] = {0};
signed short yuv_v_new[ImgSize / 4] = {0};
unsigned char yuv_y_new_raw[ImgSize];
unsigned char yuv_u_new_raw[ImgSize / 4];
unsigned char yuv_v_new_raw[ImgSize / 4];
signed short rgb_r[2][ImgSize];
signed short rgb_g[2][ImgSize];
signed short rgb_b[2][ImgSize];
signed short rgb_r_new[ImgSize];
signed short rgb_g_new[ImgSize];
signed short rgb_b_new[ImgSize];

__m128i *mR, *mG, *mB, *mY, *mU, *mV;
__m128i num1, num2, num3, num_Y, num_U, num_V, num_C, num_D, num_E, num_A, num_nA,
	num_R, num_G, num_B, res1, res2, res3, res4;
__m256i *mRa, *mGa, *mBa, *mYa, *mUa, *mVa;
__m256i num1a, num2a, num3a, num_Ya, num_Ua, num_Va, num_Ca, num_Da, num_Ea, num_Aa, num_nAa,
	num_Ra, num_Ga, num_Ba, res1a, res2a, res3a, res4a;
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
int process_with_sse(bool pics);
int process_with_avx(bool pics);

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

void yuvToRgb_sse(int index)
{
	int rownum = 0, rownumhalf = 0;

	//point
	pR = rgb_r[index];
	pG = rgb_g[index];
	pB = rgb_b[index];

	for(i = 0;i < x;i++)
		for(j = 0;j < y;j += 8)
		{
			mR = (__m128i*)pR;
			mG = (__m128i*)pG;
			mB = (__m128i*)pB;

			rownum = i * y + j;
			rownumhalf = (i / 2) * (y / 2) + j / 2;

			num_Y = _mm_set_epi16((short)yuv_y[index][rownum + 7], (short)yuv_y[index][rownum + 6], (short)yuv_y[index][rownum + 5], (short)yuv_y[index][rownum + 4],
								(short)yuv_y[index][rownum + 3], (short)yuv_y[index][rownum + 2], (short)yuv_y[index][rownum + 1], (short)yuv_y[index][rownum]);
			num_U = _mm_set_epi16((short)yuv_u[index][rownumhalf + 3], (short)yuv_u[index][rownumhalf + 3], (short)yuv_u[index][rownumhalf + 2], (short)yuv_u[index][rownumhalf + 2],
								(short)yuv_u[index][rownumhalf + 1], (short)yuv_u[index][rownumhalf + 1], (short)yuv_u[index][rownumhalf], (short)yuv_u[index][rownumhalf]);
			num_V = _mm_set_epi16((short)yuv_v[index][rownumhalf + 3], (short)yuv_v[index][rownumhalf + 3], (short)yuv_v[index][rownumhalf + 2], (short)yuv_v[index][rownumhalf + 2],
								(short)yuv_v[index][rownumhalf + 1], (short)yuv_v[index][rownumhalf + 1], (short)yuv_v[index][rownumhalf], (short)yuv_v[index][rownumhalf]);

			num1 = _mm_set1_epi16(128);
			num_D = _mm_sub_epi16(num_U, num1);// U - 128
			num_E = _mm_sub_epi16(num_V, num1);// V - 128

			//R= Y + 1.140 * V
			num1 = _mm_set1_epi16((1 << 16) * 0.140);
			res1 = _mm_mulhi_epi16(num_E, num1);
			res1 = _mm_add_epi16(res1, num_E);
			num_R = _mm_add_epi16(num_Y, res1);

			//G= Y - 0.394 * U - 0.581 * V
			num1 = _mm_set1_epi16((1 << 16) * 0.394);
			res1 = _mm_mulhi_epi16(num_D, num1);
			num2 = _mm_set1_epi16((1 << 16) * 0.081);
			res2 = _mm_mulhi_epi16(num_E, num2);
			num2 = _mm_srai_epi16(num_E, 1);
			res2 = _mm_add_epi16(res2, num2);
			res3 = _mm_sub_epi16(num_Y, res1);
			num_G = _mm_sub_epi16(res3, res2);

			//B= Y + 2.032 * U
			num1 = _mm_set1_epi16((1 << 16) * 0.032);
			res1 = _mm_mulhi_epi16(num_D, num1);
			res1 = _mm_add_epi16(res1, num_D);
			res1 = _mm_add_epi16(res1, num_D);
			num_B = _mm_add_epi16(num_Y, res1);

			// above 255
			num1 = _mm_set1_epi16(255);
			num2 = _mm_sub_epi16(num1, num_R);
			num1 = _mm_cmplt_epi16(num1, num_R);// 255 < R ? 0xffff : 0x0
			num1 = _mm_and_si128(num1, num2);	// if R < 255, 0; else 255 - R
			num_R = _mm_add_epi16(num_R, num1);	// R < 255, R; R > 255, 255

			num1 = _mm_set1_epi16(255);
			num2 = _mm_sub_epi16(num1, num_G);
			num1 = _mm_cmplt_epi16(num1, num_G);// 255 < G ? 0xffff : 0x0
			num1 = _mm_and_si128(num1, num2);	// if G < 255, 0; else 255 - G
			num_G = _mm_add_epi16(num_G, num1);	// G < 255, G; G > 255, 255		
			
			num1 = _mm_set1_epi16(255);
			num2 = _mm_sub_epi16(num1, num_B);
			num1 = _mm_cmplt_epi16(num1, num_B);// 255 < B ? 0xffff : 0x0
			num1 = _mm_and_si128(num1, num2);	// if B < 255, 0; else 255 - B
			num_B = _mm_add_epi16(num_B, num1);	// B < 255, B; B > 255, 255

			// under 0
			num1 = _mm_set1_epi16(0);
			num2 = _mm_sub_epi16(num1, num_R);
			num1 = _mm_cmplt_epi16(num_R, num1);// R < 0 ? 0xffff : 0x0
			num1 = _mm_and_si128(num1, num2);	// if R < 0, -R; else 0
			num_R = _mm_add_epi16(num_R, num1);	// R < 0, 0; R > 0, R

			num1 = _mm_set1_epi16(0);
			num2 = _mm_sub_epi16(num1, num_G);
			num1 = _mm_cmplt_epi16(num_G, num1);// G < 0 ? 0xffff : 0x0
			num1 = _mm_and_si128(num1, num2);	// if G < 0, -G; else 0
			num_G = _mm_add_epi16(num_G, num1);	// G < 0, 0; G > 0, G	

			num1 = _mm_set1_epi16(0);
			num2 = _mm_sub_epi16(num1, num_B);
			num1 = _mm_cmplt_epi16(num_B, num1);// B < 0 ? 0xffff : 0x0
			num1 = _mm_and_si128(num1, num2);	// if B < 0, -B; else 0
			num_B = _mm_add_epi16(num_B, num1);	// B < 0, 0; B > 0, B

			*mR = num_R;
			*mG = num_G;
			*mB = num_B;
			pR += 8;pG += 8;pB += 8;
		}
}

void yuvToRgb_avx(int index)
{
	int rownum = 0, rownumhalf = 0;

	//point
	pR = rgb_r[index];
	pG = rgb_g[index];
	pB = rgb_b[index];

	for(i = 0;i < x;i++)
		for(j = 0;j < y;j += 16)
		{
			mRa = (__m256i*)pR;
			mGa = (__m256i*)pG;
			mBa = (__m256i*)pB;

			rownum = i * y + j;
			rownumhalf = (i / 2) * (y / 2) + j / 2;

			num_Ya = _mm256_set_epi16((short)yuv_y[index][rownum + 15], (short)yuv_y[index][rownum + 14], (short)yuv_y[index][rownum + 13], (short)yuv_y[index][rownum + 12],
								(short)yuv_y[index][rownum + 11], (short)yuv_y[index][rownum + 10], (short)yuv_y[index][rownum + 9], (short)yuv_y[index][rownum + 8],
								(short)yuv_y[index][rownum + 7], (short)yuv_y[index][rownum + 6], (short)yuv_y[index][rownum + 5], (short)yuv_y[index][rownum + 4],
								(short)yuv_y[index][rownum + 3], (short)yuv_y[index][rownum + 2], (short)yuv_y[index][rownum + 1], (short)yuv_y[index][rownum]);
			num_Ua = _mm256_set_epi16((short)yuv_u[index][rownumhalf + 7], (short)yuv_u[index][rownumhalf + 7], (short)yuv_u[index][rownumhalf + 6], (short)yuv_u[index][rownumhalf + 6],
								(short)yuv_u[index][rownumhalf + 5], (short)yuv_u[index][rownumhalf + 5], (short)yuv_u[index][rownumhalf + 4], (short)yuv_u[index][rownumhalf + 4],
								(short)yuv_u[index][rownumhalf + 3], (short)yuv_u[index][rownumhalf + 3], (short)yuv_u[index][rownumhalf + 2], (short)yuv_u[index][rownumhalf + 2],
								(short)yuv_u[index][rownumhalf + 1], (short)yuv_u[index][rownumhalf + 1], (short)yuv_u[index][rownumhalf], (short)yuv_u[index][rownumhalf]);
			num_Va = _mm256_set_epi16((short)yuv_v[index][rownumhalf + 7], (short)yuv_v[index][rownumhalf + 7], (short)yuv_v[index][rownumhalf + 6], (short)yuv_v[index][rownumhalf + 6],
								(short)yuv_v[index][rownumhalf + 5], (short)yuv_v[index][rownumhalf + 5], (short)yuv_v[index][rownumhalf + 4], (short)yuv_v[index][rownumhalf + 4],
								(short)yuv_v[index][rownumhalf + 3], (short)yuv_v[index][rownumhalf + 3], (short)yuv_v[index][rownumhalf + 2], (short)yuv_v[index][rownumhalf + 2],
								(short)yuv_v[index][rownumhalf + 1], (short)yuv_v[index][rownumhalf + 1], (short)yuv_v[index][rownumhalf], (short)yuv_v[index][rownumhalf]);

			num1a = _mm256_set1_epi16(128);
			num_Da = _mm256_sub_epi16(num_Ua, num1a);// U - 128
			num_Ea = _mm256_sub_epi16(num_Va, num1a);// V - 128

			//R= Y + 1.140 * V
			num1a = _mm256_set1_epi16((1 << 16) * 0.140);
			res1a = _mm256_mulhi_epi16(num_Ea, num1a);
			res1a = _mm256_add_epi16(res1a, num_Ea);
			num_Ra = _mm256_add_epi16(num_Ya, res1a);

			//G= Y - 0.394 * U - 0.581 * V
			num1a = _mm256_set1_epi16((1 << 16) * 0.394);
			res1a = _mm256_mulhi_epi16(num_Da, num1a);
			num2a = _mm256_set1_epi16((1 << 16) * 0.081);
			res2a = _mm256_mulhi_epi16(num_Ea, num2a);
			num2a = _mm256_srai_epi16(num_Ea, 1);
			res2a = _mm256_add_epi16(num2a, res2a);
			res3a = _mm256_sub_epi16(num_Ya, res1a);
			num_Ga = _mm256_sub_epi16(res3a, res2a);

			//B= Y + 2.032 * U
			num1a = _mm256_set1_epi16((1 << 16) * 0.032);
			res1a = _mm256_mulhi_epi16(num_Da, num1a);
			res1a = _mm256_add_epi16(res1a, num_Da);
			res1a = _mm256_add_epi16(res1a, num_Da);
			num_Ba = _mm256_add_epi16(num_Ya, res1a);

			// above 255
			num1a = _mm256_set1_epi16(255);
			num2a = _mm256_sub_epi16(num1a, num_Ra);
			num1a = _mm256_cmpgt_epi16(num_Ra, num1a);	// 255 < R ? 0xffff : 0x0
			num1a = _mm256_and_si256(num1a, num2a);		// if R < 255, 0; else 255 - R
			num_Ra = _mm256_add_epi16(num_Ra, num1a);	// R < 255, R; R > 255, 255

			num1a = _mm256_set1_epi16(255);
			num2a = _mm256_sub_epi16(num1a, num_Ga);
			num1a = _mm256_cmpgt_epi16(num_Ga, num1a);// 255 < G ? 0xffff : 0x0
			num1a = _mm256_and_si256(num1a, num2a);	// if G < 255, 0; else 255 - G
			num_Ga = _mm256_add_epi16(num_Ga, num1a);	// G < 255, G; G > 255, 255		
			
			num1a = _mm256_set1_epi16(255);
			num2a = _mm256_sub_epi16(num1a, num_Ba);
			num1a = _mm256_cmpgt_epi16(num_Ba, num1a);	// 255 < B ? 0xffff : 0x0
			num1a = _mm256_and_si256(num1a, num2a);		// if B < 255, 0; else 255 - B
			num_Ba = _mm256_add_epi16(num_Ba, num1a);	// B < 255, B; B > 255, 255

			// under 0
			num1a = _mm256_set1_epi16(0);
			num2a = _mm256_sub_epi16(num1a, num_Ra);
			num1a = _mm256_cmpgt_epi16(num1a, num_Ra);	// R < 0 ? 0xffff : 0x0
			num1a = _mm256_and_si256(num1a, num2a);		// if R < 0, -R; else 0
			num_Ra = _mm256_add_epi16(num_Ra, num1a);	// R < 0, 0; R > 0, R

			num1a = _mm256_set1_epi16(0);
			num2a = _mm256_sub_epi16(num1a, num_Ga);
			num1a = _mm256_cmpgt_epi16(num1a, num_Ga);	// G < 0 ? 0xffff : 0x0
			num1a = _mm256_and_si256(num1a, num2a);		// if G < 0, -G; else 0
			num_Ga = _mm256_add_epi16(num_Ga, num1a);	// G < 0, 0; G > 0, G	

			num1a = _mm256_set1_epi16(0);
			num2a = _mm256_sub_epi16(num1a, num_Ba);
			num1a = _mm256_cmpgt_epi16(num1a, num_Ba);	// B < 0 ? 0xffff : 0x0
			num1a = _mm256_and_si256(num1a, num2a);		// if B < 0, -B; else 0
			num_Ba = _mm256_add_epi16(num_Ba, num1a);	// B < 0, 0; B > 0, B

			*mRa = num_Ra;
			*mGa = num_Ga;
			*mBa = num_Ba;
			pR += 16;pG += 16;pB += 16;
		}
}

void alpha_rgb_sse(int alpha)
{
	int rownum = 0;
	pR = rgb_r_new;
	pG = rgb_g_new;
	pB = rgb_b_new;

	num_A = _mm_set1_epi16((short)alpha);	// < 255

	for(i = 0;i < x;i++)
		for(j = 0;j < y;j += 8)
		{
			mR = (__m128i*)pR;
			mG = (__m128i*)pG;
			mB = (__m128i*)pB;

			rownum = i * y + j;

			//R’= A * R / 256
			num_R = _mm_set_epi16(rgb_r[0][rownum + 7], rgb_r[0][rownum + 6], rgb_r[0][rownum + 5], rgb_r[0][rownum + 4],
								rgb_r[0][rownum + 3], rgb_r[0][rownum + 2], rgb_r[0][rownum + 1], rgb_r[0][rownum]);
			num_R = _mm_mullo_epi16(num_R, num_A);
			num_R = _mm_srli_epi16(num_R, 8);
			*mR = num_R;

			//G’= A * G / 256
			num_G = _mm_set_epi16(rgb_g[0][rownum + 7], rgb_g[0][rownum + 6], rgb_g[0][rownum + 5], rgb_g[0][rownum + 4],
								rgb_g[0][rownum + 3], rgb_g[0][rownum + 2], rgb_g[0][rownum + 1], rgb_g[0][rownum]);
			num_G = _mm_mullo_epi16(num_G, num_A);
			*mG = _mm_srli_epi16(num_G, 8);

			//B’= A * B / 256
			num_B = _mm_set_epi16(rgb_b[0][rownum + 7], rgb_b[0][rownum + 6], rgb_b[0][rownum + 5], rgb_b[0][rownum + 4],
								rgb_b[0][rownum + 3], rgb_b[0][rownum + 2], rgb_b[0][rownum + 1], rgb_b[0][rownum]);
			num_B = _mm_mullo_epi16(num_B, num_A);
			*mB = _mm_srli_epi16(num_B, 8);

			pR += 8;pG += 8;pB += 8;
		}
}

void alpha_rgb_avx(int alpha)
{
	int rownum = 0;
	pR = rgb_r_new;
	pG = rgb_g_new;
	pB = rgb_b_new;

	num_Aa = _mm256_set1_epi16((short)alpha);	// < 255

	for(i = 0;i < x;i++)
		for(j = 0;j < y;j += 16)
		{			
			mRa = (__m256i*)pR;
			mGa = (__m256i*)pG;
			mBa = (__m256i*)pB;

			rownum = i * y + j;

			//R’= A * R / 256
			num_Ra = _mm256_set_epi16(rgb_r[0][rownum + 15], rgb_r[0][rownum + 14], rgb_r[0][rownum + 13], rgb_r[0][rownum + 12],
								rgb_r[0][rownum + 11], rgb_r[0][rownum + 10], rgb_r[0][rownum + 9], rgb_r[0][rownum + 8],
								rgb_r[0][rownum + 7], rgb_r[0][rownum + 6], rgb_r[0][rownum + 5], rgb_r[0][rownum + 4],
								rgb_r[0][rownum + 3], rgb_r[0][rownum + 2], rgb_r[0][rownum + 1], rgb_r[0][rownum]);
			num_Ra = _mm256_mullo_epi16(num_Ra, num_Aa);
			*mRa = _mm256_srli_epi16(num_Ra, 8);

			//G’= A * G / 256
			num_Ga = _mm256_set_epi16(rgb_g[0][rownum + 15], rgb_g[0][rownum + 14], rgb_g[0][rownum + 13], rgb_g[0][rownum + 12],
								rgb_g[0][rownum + 11], rgb_g[0][rownum + 10], rgb_g[0][rownum + 9], rgb_g[0][rownum + 8],
								rgb_g[0][rownum + 7], rgb_g[0][rownum + 6], rgb_g[0][rownum + 5], rgb_g[0][rownum + 4],
								rgb_g[0][rownum + 3], rgb_g[0][rownum + 2], rgb_g[0][rownum + 1], rgb_g[0][rownum]);
			num_Ga = _mm256_mullo_epi16(num_Ga, num_Aa);
			*mGa = _mm256_srli_epi16(num_Ga, 8);

			//B’= A * B / 256
			num_Ba = _mm256_set_epi16(rgb_b[0][rownum + 15], rgb_b[0][rownum + 14], rgb_b[0][rownum + 13], rgb_b[0][rownum + 12],
								rgb_b[0][rownum + 11], rgb_b[0][rownum + 10], rgb_b[0][rownum + 9], rgb_b[0][rownum + 8],
								rgb_b[0][rownum + 7], rgb_b[0][rownum + 6], rgb_b[0][rownum + 5], rgb_b[0][rownum + 4],
								rgb_b[0][rownum + 3], rgb_b[0][rownum + 2], rgb_b[0][rownum + 1], rgb_b[0][rownum]);
			num_Ba = _mm256_mullo_epi16(num_Ba, num_Aa);
			*mBa = _mm256_srli_epi16(num_Ba, 8);

			pR += 16;pG += 16;pB += 16;
		}
}

void alpha_rgb_double_sse(int alpha)
{
	int rownum = 0;
	pR = rgb_r_new;
	pG = rgb_g_new;
	pB = rgb_b_new;
	
	num_A = _mm_set1_epi16((short)alpha);	// < 255
	num_nA = _mm_set1_epi16((short)256 - alpha);

	for(i = 0;i < x;i++)
		for(j = 0;j < y;j += 8)
		{
			mR = (__m128i*)pR;
			mG = (__m128i*)pG;
			mB = (__m128i*)pB;

			rownum = i * y + j;

			//R’= (A * R1 + (256-A) * R2) / 256
			num_R = _mm_set_epi16(rgb_r[0][rownum + 7], rgb_r[0][rownum + 6], rgb_r[0][rownum + 5], rgb_r[0][rownum + 4],
								rgb_r[0][rownum + 3], rgb_r[0][rownum + 2], rgb_r[0][rownum + 1], rgb_r[0][rownum]);
			num1 = _mm_set_epi16(rgb_r[1][rownum + 7], rgb_r[1][rownum + 6], rgb_r[1][rownum + 5], rgb_r[1][rownum + 4],
								rgb_r[1][rownum + 3], rgb_r[1][rownum + 2], rgb_r[1][rownum + 1], rgb_r[1][rownum]);
			res1 = _mm_mullo_epi16(num_R, num_A);
			res2 = _mm_mullo_epi16(num1, num_nA);
			res3 = _mm_add_epi16(res1, res2);
			*mR = _mm_srli_epi16(res3, 8);

			//G’= (A * G1 + (256-A) * G2) / 256
			num_G = _mm_set_epi16(rgb_g[0][rownum + 7], rgb_g[0][rownum + 6], rgb_g[0][rownum + 5], rgb_g[0][rownum + 4],
								rgb_g[0][rownum + 3], rgb_g[0][rownum + 2], rgb_g[0][rownum + 1], rgb_g[0][rownum]);
			num1 = _mm_set_epi16(rgb_g[1][rownum + 7], rgb_g[1][rownum + 6], rgb_g[1][rownum + 5], rgb_g[1][rownum + 4],
								rgb_g[1][rownum + 3], rgb_g[1][rownum + 2], rgb_g[1][rownum + 1], rgb_g[1][rownum]);
			res1 = _mm_mullo_epi16(num_G, num_A);
			res2 = _mm_mullo_epi16(num1, num_nA);
			res3 = _mm_add_epi16(res1, res2);
			*mG = _mm_srli_epi16(res3, 8);

			//B’= (A * B1 + (256-A) * B2) / 256
			num_B = _mm_set_epi16(rgb_b[0][rownum + 7], rgb_b[0][rownum + 6], rgb_b[0][rownum + 5], rgb_b[0][rownum + 4],
								rgb_b[0][rownum + 3], rgb_b[0][rownum + 2], rgb_b[0][rownum + 1], rgb_b[0][rownum]);
			num1 = _mm_set_epi16(rgb_b[1][rownum + 7], rgb_b[1][rownum + 6], rgb_b[1][rownum + 5], rgb_b[1][rownum + 4],
								rgb_b[1][rownum + 3], rgb_b[1][rownum + 2], rgb_b[1][rownum + 1], rgb_b[1][rownum]);
			res1 = _mm_mullo_epi16(num_B, num_A);
			res2 = _mm_mullo_epi16(num1, num_nA);
			res3 = _mm_add_epi16(res1, res2);
			*mB = _mm_srli_epi16(res3, 8);

			pR += 8;pG += 8;pB += 8;
		}
}

void alpha_rgb_double_avx(int alpha)
{	
	int rownum = 0;
	pR = rgb_r_new;
	pG = rgb_g_new;
	pB = rgb_b_new;
	
	num_Aa = _mm256_set1_epi16((short)alpha);	// < 255
	num_nAa = _mm256_set1_epi16((short)256 - alpha);

	for(i = 0;i < x;i++)
		for(j = 0;j < y;j += 16)
		{			
			mRa = (__m256i*)pR;
			mGa = (__m256i*)pG;
			mBa = (__m256i*)pB;

			rownum = i * y + j;

			//R’= (A * R1 + (256-A) * R2) / 256
			num_Ra = _mm256_set_epi16(rgb_r[0][rownum + 15], rgb_r[0][rownum + 14], rgb_r[0][rownum + 13], rgb_r[0][rownum + 12],
								rgb_r[0][rownum + 11], rgb_r[0][rownum + 10], rgb_r[0][rownum + 9], rgb_r[0][rownum + 8],
								rgb_r[0][rownum + 7], rgb_r[0][rownum + 6], rgb_r[0][rownum + 5], rgb_r[0][rownum + 4],
								rgb_r[0][rownum + 3], rgb_r[0][rownum + 2], rgb_r[0][rownum + 1], rgb_r[0][rownum]);
			num1a = _mm256_set_epi16(rgb_r[1][rownum + 15], rgb_r[1][rownum + 14], rgb_r[1][rownum + 13], rgb_r[1][rownum + 12],
								rgb_r[1][rownum + 11], rgb_r[1][rownum + 10], rgb_r[1][rownum + 9], rgb_r[1][rownum + 8],
								rgb_r[1][rownum + 7], rgb_r[1][rownum + 6], rgb_r[1][rownum + 5], rgb_r[1][rownum + 4],
								rgb_r[1][rownum + 3], rgb_r[1][rownum + 2], rgb_r[1][rownum + 1], rgb_r[1][rownum]);
			res1a = _mm256_mullo_epi16(num_Ra, num_Aa);
			res2a = _mm256_mullo_epi16(num1a, num_nAa);
			res3a = _mm256_add_epi16(res1a, res2a);
			*mRa = _mm256_srli_epi16(res3a, 8);

			//G’= (A * G1 + (256-A) * G2) / 256
			num_Ga = _mm256_set_epi16(rgb_g[0][rownum + 15], rgb_g[0][rownum + 14], rgb_g[0][rownum + 13], rgb_g[0][rownum + 12],
								rgb_g[0][rownum + 11], rgb_g[0][rownum + 10], rgb_g[0][rownum + 9], rgb_g[0][rownum + 8],
								rgb_g[0][rownum + 7], rgb_g[0][rownum + 6], rgb_g[0][rownum + 5], rgb_g[0][rownum + 4],
								rgb_g[0][rownum + 3], rgb_g[0][rownum + 2], rgb_g[0][rownum + 1], rgb_g[0][rownum]);
			num1a = _mm256_set_epi16(rgb_g[1][rownum + 15], rgb_g[1][rownum + 14], rgb_g[1][rownum + 13], rgb_g[1][rownum + 12],
								rgb_g[1][rownum + 11], rgb_g[1][rownum + 10], rgb_g[1][rownum + 9], rgb_g[1][rownum + 8],
								rgb_g[1][rownum + 7], rgb_g[1][rownum + 6], rgb_g[1][rownum + 5], rgb_g[1][rownum + 4],
								rgb_g[1][rownum + 3], rgb_g[1][rownum + 2], rgb_g[1][rownum + 1], rgb_g[1][rownum]);
			res1a = _mm256_mullo_epi16(num_Ga, num_Aa);
			res2a = _mm256_mullo_epi16(num1a, num_nAa);
			res3a = _mm256_add_epi16(res1a, res2a);
			*mGa = _mm256_srli_epi16(res3a, 8);

			//B’= (A * B1 + (256-A) * B2) / 256
			num_Ba = _mm256_set_epi16(rgb_b[0][rownum + 15], rgb_b[0][rownum + 14], rgb_b[0][rownum + 13], rgb_b[0][rownum + 12],
								rgb_b[0][rownum + 11], rgb_b[0][rownum + 10], rgb_b[0][rownum + 9], rgb_b[0][rownum + 8],
								rgb_b[0][rownum + 7], rgb_b[0][rownum + 6], rgb_b[0][rownum + 5], rgb_b[0][rownum + 4],
								rgb_b[0][rownum + 3], rgb_b[0][rownum + 2], rgb_b[0][rownum + 1], rgb_b[0][rownum]);
			num1a = _mm256_set_epi16(rgb_b[1][rownum + 15], rgb_b[1][rownum + 14], rgb_b[1][rownum + 13], rgb_b[1][rownum + 12],
								rgb_b[1][rownum + 11], rgb_b[1][rownum + 10], rgb_b[1][rownum + 9], rgb_b[1][rownum + 8],
								rgb_b[1][rownum + 7], rgb_b[1][rownum + 6], rgb_b[1][rownum + 5], rgb_b[1][rownum + 4],
								rgb_b[1][rownum + 3], rgb_b[1][rownum + 2], rgb_b[1][rownum + 1], rgb_b[1][rownum]);
			res1a = _mm256_mullo_epi16(num_Ba, num_Aa);
			res2a = _mm256_mullo_epi16(num1a, num_nAa);
			res3a = _mm256_add_epi16(res1a, res2a);
			*mBa = _mm256_srli_epi16(res3a, 8);

			pR += 16;pG += 16;pB += 16;
		}
}

void rgbToYuv_sse()
{
	pY = yuv_y_new;
	pU = yuv_u_new;
	pV = yuv_v_new;
	
	int rownum = 0;

	for(i = 0;i < x;i++)
		for(j = 0;j < y;j += 8)
		{
			rownum = i * y + j;
			mY = (__m128i*)pY;
			
			//Y= 0.299 * R + 0.587 * G + 0.114 * B
			num_R = _mm_set_epi16(rgb_r_new[rownum + 7], rgb_r_new[rownum + 6], rgb_r_new[rownum + 5], rgb_r_new[rownum + 4],
								rgb_r_new[rownum + 3], rgb_r_new[rownum + 2], rgb_r_new[rownum + 1], rgb_r_new[rownum]);
			num_G = _mm_set_epi16(rgb_g_new[rownum + 7], rgb_g_new[rownum + 6], rgb_g_new[rownum + 5], rgb_g_new[rownum + 4],
								rgb_g_new[rownum + 3], rgb_g_new[rownum + 2], rgb_g_new[rownum + 1], rgb_g_new[rownum]);
			num_B = _mm_set_epi16(rgb_b_new[rownum + 7], rgb_b_new[rownum + 6], rgb_b_new[rownum + 5], rgb_b_new[rownum + 4],
								rgb_b_new[rownum + 3], rgb_b_new[rownum + 2], rgb_b_new[rownum + 1], rgb_b_new[rownum]);

			num1 = _mm_set1_epi16((1 << 16) * 0.299);
			res1 = _mm_mulhi_epi16(num1, num_R);

			num1 = _mm_set1_epi16((1 << 16) * 0.087);
			num2 = _mm_srai_epi16(num_G, 1);
			res2 = _mm_mulhi_epi16(num1, num_G);
			res2 = _mm_add_epi16(res2, num2);

			num1 = _mm_set1_epi16((1 << 16) * 0.114);
			res3 = _mm_mulhi_epi16(num1, num_B);

			num_Y = _mm_add_epi16(res1, res2);
			num_Y = _mm_add_epi16(num_Y, res3);
			*mY = num_Y;

			if(i % 2 == 0 && j % 16 == 0)
			{
				mU = (__m128i*)pU;
				mV = (__m128i*)pV;

				num_R = _mm_set_epi16(rgb_r_new[rownum + 14], rgb_r_new[rownum + 12], rgb_r_new[rownum + 10], rgb_r_new[rownum + 8],
									rgb_r_new[rownum + 6], rgb_r_new[rownum + 4], rgb_r_new[rownum + 2], rgb_r_new[rownum]);
				num_G = _mm_set_epi16(rgb_g_new[rownum + 14], rgb_g_new[rownum + 12], rgb_g_new[rownum + 10], rgb_g_new[rownum + 8],
									rgb_g_new[rownum + 6], rgb_g_new[rownum + 4], rgb_g_new[rownum + 2], rgb_g_new[rownum]);
				num_B = _mm_set_epi16(rgb_b_new[rownum + 14], rgb_b_new[rownum + 12], rgb_b_new[rownum + 10], rgb_b_new[rownum + 8],
									rgb_b_new[rownum + 6], rgb_b_new[rownum + 4], rgb_b_new[rownum + 2], rgb_b_new[rownum]);

				res4 = _mm_set1_epi16(128);

				// U= -0.147 * R - 0.289 * G + 0.436 * B = 0.492 * (B- Y)
				num1 = _mm_set1_epi16((1 << 16) * 0.147);
				res1 = _mm_mulhi_epi16(num1, num_R);

				num1 = _mm_set1_epi16((1 << 16) * 0.289);
				res2 = _mm_mulhi_epi16(num1, num_G);

				num1 = _mm_set1_epi16((1 << 16) * 0.436);
				res3 = _mm_mulhi_epi16(num1, num_B);

				num_U = _mm_sub_epi16(res3, res2);
				num_U = _mm_sub_epi16(num_U, res1);
				*mU = _mm_add_epi16(num_U, res4);

				// V= 0.615 * R - 0.515 * G - 0.100 * B = 0.877 * (R- Y)
				num1 = _mm_set1_epi16((1 << 16) * 0.115);
				num2 = _mm_srai_epi16(num_R, 1);
				res1 = _mm_mulhi_epi16(num1, num_R);
				res1 = _mm_add_epi16(res1, num2);

				num1 = _mm_set1_epi16((1 << 16) * 0.015);
				num2 = _mm_srai_epi16(num_G, 1);
				res2 = _mm_mulhi_epi16(num1, num_G);
				res2 = _mm_add_epi16(res2, num2);

				num1 = _mm_set1_epi16((1 << 16) * 0.100);
				res3 = _mm_mulhi_epi16(num1, num_B);
				num_V = _mm_sub_epi16(res1, res2);
				num_V = _mm_sub_epi16(num_V, res3);
				*mV = _mm_add_epi16(num_V, res4);

				pU += 8;pV += 8;
			}
			pY += 8;
		}
}

void rgbToYuv_avx()
{
	pY = yuv_y_new;
	pU = yuv_u_new;
	pV = yuv_v_new;
	
	int rownum = 0;

	for(i = 0;i < x;i++)
		for(j = 0;j < y;j += 16)
		{
			rownum = i * y + j;
			mYa = (__m256i*)pY;
			
			//Y= 0.299 * R + 0.587 * G + 0.114 * B
			num_Ra = _mm256_set_epi16(rgb_r_new[rownum + 15], rgb_r_new[rownum + 14], rgb_r_new[rownum + 13], rgb_r_new[rownum + 12],
								rgb_r_new[rownum + 11], rgb_r_new[rownum + 10], rgb_r_new[rownum + 9], rgb_r_new[rownum + 8],
								rgb_r_new[rownum + 7], rgb_r_new[rownum + 6], rgb_r_new[rownum + 5], rgb_r_new[rownum + 4],
								rgb_r_new[rownum + 3], rgb_r_new[rownum + 2], rgb_r_new[rownum + 1], rgb_r_new[rownum]);
			num_Ga = _mm256_set_epi16(rgb_g_new[rownum + 15], rgb_g_new[rownum + 14], rgb_g_new[rownum + 13], rgb_g_new[rownum + 12],
								rgb_g_new[rownum + 11], rgb_g_new[rownum + 10], rgb_g_new[rownum + 9], rgb_g_new[rownum + 8],
								rgb_g_new[rownum + 7], rgb_g_new[rownum + 6], rgb_g_new[rownum + 5], rgb_g_new[rownum + 4],
								rgb_g_new[rownum + 3], rgb_g_new[rownum + 2], rgb_g_new[rownum + 1], rgb_g_new[rownum]);
			num_Ba = _mm256_set_epi16(rgb_b_new[rownum + 15], rgb_b_new[rownum + 14], rgb_b_new[rownum + 13], rgb_b_new[rownum + 12],
								rgb_b_new[rownum + 11], rgb_b_new[rownum + 10], rgb_b_new[rownum + 9], rgb_b_new[rownum + 8],
								rgb_b_new[rownum + 7], rgb_b_new[rownum + 6], rgb_b_new[rownum + 5], rgb_b_new[rownum + 4],
								rgb_b_new[rownum + 3], rgb_b_new[rownum + 2], rgb_b_new[rownum + 1], rgb_b_new[rownum]);

			num1a = _mm256_set1_epi16((1 << 16) * 0.299);
			res1a = _mm256_mulhi_epi16(num1a, num_Ra);

			num1a = _mm256_set1_epi16((1 << 16) * 0.087);
			num2a = _mm256_srai_epi16(num_Ga, 1);
			res2a = _mm256_mulhi_epi16(num1a, num_Ga);
			res2a = _mm256_add_epi16(res2a, num2a);

			num1a = _mm256_set1_epi16((1 << 16) * 0.114);
			res3a = _mm256_mulhi_epi16(num1a, num_Ba);

			num_Ya = _mm256_add_epi16(res1a, res2a);
			num_Ya = _mm256_add_epi16(num_Ya, res3a);
			*mYa = num_Ya;

			if(i % 2 == 0 && j % 32 == 0)
			{
				mUa = (__m256i*)pU;
				mVa = (__m256i*)pV;

				num_Ra = _mm256_set_epi16(rgb_r_new[rownum + 30], rgb_r_new[rownum + 28], rgb_r_new[rownum + 26], rgb_r_new[rownum + 24],
									rgb_r_new[rownum + 22], rgb_r_new[rownum + 20], rgb_r_new[rownum + 18], rgb_r_new[rownum + 16],
									rgb_r_new[rownum + 14], rgb_r_new[rownum + 12], rgb_r_new[rownum + 10], rgb_r_new[rownum + 8],
									rgb_r_new[rownum + 6], rgb_r_new[rownum + 4], rgb_r_new[rownum + 2], rgb_r_new[rownum]);
				num_Ga = _mm256_set_epi16(rgb_g_new[rownum + 30], rgb_g_new[rownum + 28], rgb_g_new[rownum + 26], rgb_g_new[rownum + 24],
									rgb_g_new[rownum + 22], rgb_g_new[rownum + 20], rgb_g_new[rownum + 18], rgb_g_new[rownum + 16],
									rgb_g_new[rownum + 14], rgb_g_new[rownum + 12], rgb_g_new[rownum + 10], rgb_g_new[rownum + 8],
									rgb_g_new[rownum + 6], rgb_g_new[rownum + 4], rgb_g_new[rownum + 2], rgb_g_new[rownum]);
				num_Ba = _mm256_set_epi16(rgb_b_new[rownum + 30], rgb_b_new[rownum + 28], rgb_b_new[rownum + 26], rgb_b_new[rownum + 24],
									rgb_b_new[rownum + 22], rgb_b_new[rownum + 20], rgb_b_new[rownum + 18], rgb_b_new[rownum + 16],
									rgb_b_new[rownum + 14], rgb_b_new[rownum + 12], rgb_b_new[rownum + 10], rgb_b_new[rownum + 8],
									rgb_b_new[rownum + 6], rgb_b_new[rownum + 4], rgb_b_new[rownum + 2], rgb_b_new[rownum]);

				res4a = _mm256_set1_epi16(128);

				// U= -0.147 * R - 0.289 * G + 0.436 * B = 0.492 * (B- Y)
				num1a = _mm256_set1_epi16((1 << 16) * 0.147);
				res1a = _mm256_mulhi_epi16(num1a, num_Ra);

				num1a = _mm256_set1_epi16((1 << 16) * 0.289);
				res2a = _mm256_mulhi_epi16(num1a, num_Ga);

				num1a = _mm256_set1_epi16((1 << 16) * 0.436);
				res3a = _mm256_mulhi_epi16(num1a, num_Ba);

				num_Ua = _mm256_sub_epi16(res3a, res2a);
				num_Ua = _mm256_sub_epi16(num_Ua, res1a);
				*mUa = _mm256_add_epi16(num_Ua, res4a);

				// V= 0.615 * R - 0.515 * G - 0.100 * B = 0.877 * (R- Y)
				num1a = _mm256_set1_epi16((1 << 16) * 0.115);
				num2a = _mm256_srai_epi16(num_Ra, 1);
				res1a = _mm256_mulhi_epi16(num1a, num_Ra);
				res1a = _mm256_add_epi16(res1a, num2a);

				num1a = _mm256_set1_epi16((1 << 16) * 0.015);
				num2a = _mm256_srai_epi16(num_Ga, 1);
				res2a = _mm256_mulhi_epi16(num1a, num_Ga);
				res2a = _mm256_add_epi16(res2a, num2a);

				num1a = _mm256_set1_epi16((1 << 16) * 0.100);
				res3a = _mm256_mulhi_epi16(num1a, num_Ba);
				num_Va = _mm256_sub_epi16(res1a, res2a);
				num_Va = _mm256_sub_epi16(num_Va, res3a);
				*mVa = _mm256_add_epi16(num_Va, res4a);

				pU += 16;pV += 16;
			}
			pY += 16;
		}
}

int main(){	
	readyuv((char*)"demo/dem1.yuv", (char*)"demo/dem2.yuv");
	int time_single_sse2 = process_with_sse(false);
	int time_double_sse2 = process_with_sse(true);
	int time_single_avx = process_with_avx(false);
	int time_double_avx = process_with_avx(true);
	return 0;
}

//下面的4个函数应该统计出图像处理的时间;
//函数参数和返回值可以需要自己定.
int process_with_sse(bool pics){
	start_clock = clock();
	/*处理过程*/
	FILE *fw;
	if(!pics)	// one picture
	{
		if((fw = fopen("demo/test_sse_one.yuv", "wb")) == NULL)
		{
			printf("Open write file1(sse, one picture) failed.\n");
			return 0;
		}
		file_clock = clock();
		yuvToRgb_sse(0);
		for(int a = 1;a < 255;a += 3)
		{
			alpha_rgb_sse(a);
			rgbToYuv_sse();
			writeyuv(fw);
		}
	}
	else			// two pictures blending
	{
		if((fw = fopen("demo/test_sse_two.yuv", "wb")) == NULL)
		{
			printf("Open write file2(sse, two pictures) failed.\n");
			return 0;
		}
		file_clock = clock();
		yuvToRgb_sse(0);
		yuvToRgb_sse(1);
		for(int a = 1;a < 255;a += 3)
		{
			alpha_rgb_double_sse(255 - a);
			rgbToYuv_sse();
			writeyuv(fw);
		}
	}
	fclose(fw);
	finish_clock = clock();

	file_time = file_clock - start_clock;
	total_time = finish_clock - start_clock;
	if(!pics)	// one picture
	{
		cout << "TEST: sse2, one picture" << endl;
	}
	else
	{
		cout << "TEST: sse2, two picture" << endl;
	}
	cout << "The time costs on open file is " << file_time << "/" << CLOCKS_PER_SEC << "(s)" << endl;
	cout << "The time costs on total process is " << total_time << "/" << CLOCKS_PER_SEC << "(s)" << endl << endl;
	return total_time;
}

int process_with_avx(bool pics){
	start_clock = clock();
	/*处理过程*/
	FILE *fw;
	if(!pics)	// one picture
	{
		if((fw = fopen("demo/test_avx_one.yuv", "wb")) == NULL)
		{
			printf("Open write file1(avx, one picture) failed.\n");
			return 0;
		}
		file_clock = clock();
		yuvToRgb_avx(0);
		for(int a = 1;a < 255;a += 3)
		{
			alpha_rgb_avx(a);
			rgbToYuv_avx();
			writeyuv(fw);
		}
	}
	else			// two pictures blending
	{
		if((fw = fopen("demo/test_avx_two.yuv", "wb")) == NULL)
		{
			printf("Open write file2(avx, two pictures) failed.\n");
			return 0;
		}
		file_clock = clock();
		yuvToRgb_avx(0);
		yuvToRgb_avx(1);
		for(int a = 1;a < 255;a += 3)
		{
			alpha_rgb_double_avx(a);
			rgbToYuv_avx();
			writeyuv(fw);
		}
	}
	fclose(fw);
	finish_clock = clock();

	file_time = file_clock - start_clock;
	total_time = finish_clock - start_clock;
	if(!pics)	// one picture
	{
		cout << "TEST: avx, one picture" << endl;
	}
	else
	{
		cout << "TEST: avx, two picture" << endl;
	}
	cout << "The time costs on open file is " << file_time << "/" << CLOCKS_PER_SEC << "(s)" << endl;
	cout << "The time costs on total process is " << total_time << "/" << CLOCKS_PER_SEC << "(s)" << endl << endl;
	return total_time;
}