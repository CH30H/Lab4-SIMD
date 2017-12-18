# Lab4-SIMD
The Lab4（SIMD） for Lab on Computer Organization and Architecture
Lab on Computer Organization and Architecture
Lab4.1 		基于SIMD扩展指令的图像处理加速
Author:		Yichun Li(Judy Lee)
No.			1400012782
School:		EECS, Peking University
Date:		12-17-2017



YUV转换代码使用说明

1.Instruction
	该代码读取单帧的YUV文件，将其转换成RGB格式并进行Alpha渐变处理，然后转换回YUV格式并写入文件中，生成多帧渐变的YUV文件。
	最终该代码的可执行程序在命令行输出多种方法生成YUV文件消耗时间，并在demo文件夹中输出多个多帧渐变YUV文件，可用YUV播放器打开播放。

	
------------------------------------------------------------------------------------


2.How to Use

	该代码使用可执行文件yuv_noSIMD、yuv_MMX、yuv_SSE2_AVX获取结果。由于YUV文件较大，没有附上demo文件夹，需手动添加。
	该代码使用可执行文件yuv_noSIMD、yuv_MMX、yuv_SSE2_AVX获取结果。由于YUV文件较大，没有附上demo文件夹，需手动添加。
	首先用makefile获取3个可执行文件：
		命令行输入make(文件包已包括可执行文件)
	
	然后将demo文件夹放于可执行文件同一目录下，demo文件夹应包括两个单帧YUV文件dem1.yuv和dem2.yuv。
	
	最后命令行输入./yuv_noSIMD或./yuv_MMX或./yuv_SSE2_AVX，得到处理时间和多帧渐变YUV图像文件。
		
	
------------------------------------------------------------------------------------


3.The Output

	命令行输出各种处理方式的打开文件时间和总处理时间，demo文件夹下得到各种处理方式处理后的多帧YUV文件。
	
	yuv_noSIMD：
		对dem1.yuv和dem2.yuv进行普通处理，由于YUV与RGB之间的转换公式有浮点型和整型两种，故分别进行测试。
		输出时间依次为
			单幅图像淡入淡出-使用浮点型公式
			两幅图像叠加渐变-使用浮点型公式
			单幅图像淡入淡出-使用整型公式
			两幅图像叠加渐变-使用整型公式
			
		输出文件依次为
			单幅图像淡入淡出-使用浮点型公式	-	test_noSIMD_one.yuv
			两幅图像叠加渐变-使用浮点型公式	-	test_noSIMD_two.yuv
			单幅图像淡入淡出-使用整型公式	-	test_noSIMD_int_one.yuv
			两幅图像叠加渐变-使用整型公式	-	test_noSIMD_int_two.yuv
			
	yuv_MMX:
		对dem1.yuv和dem2.yuv进行MMX优化处理，由于YUV与RGB之间的转换公式有浮点型和整型两种，故分别进行测试。
		输出时间依次为
			单幅图像淡入淡出-使用整型公式
			两幅图像叠加渐变-使用整型公式
			单幅图像淡入淡出-使用浮点型公式
			两幅图像叠加渐变-使用浮点型公式
			
		输出文件依次为
			单幅图像淡入淡出-使用浮点型公式	-	test_mmx_part_one.yuv
			两幅图像叠加渐变-使用浮点型公式	-	test_mmx_part_two.yuv
			单幅图像淡入淡出-使用整型公式	-	test_mmx_all_one.yuv
			两幅图像叠加渐变-使用整型公式	-	test_mmx_all_two.yuv

	yuv_SSE2_AVX:
		对dem1.yuv和dem2.yuv分别进行SSE2和AVX优化处理。
		输出时间依次为
			单幅图像淡入淡出-SSE2
			两幅图像叠加渐变-SSE2
			单幅图像淡入淡出-AVX
			两幅图像叠加渐变-AVX
			
		输出文件依次为
			单幅图像淡入淡出-SSE2	-	test_sse_one.yuv
			两幅图像叠加渐变-SSE2	-	test_sse_two.yuv
			单幅图像淡入淡出-AVX	-	test_avx_one.yuv
			两幅图像叠加渐变-AVX	-	test_avx_two.yuv	
