#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <pmmintrin.h>

#include "bitmap.h"
#include "arg_parse.h"

int min(int a, int b)
{
	return (a < b) ? a : b;
}

int max(int a, int b)
{
	return (a > b) ? a : b;
}



void print_red(const char* input)
{
	printf("\033[1;31m");
	printf("%s\n",input);
	printf("\033[0m");
}



void brightness_change_sse(bitmap_pixel_hsv_t* pixels, int count, float level)
{
	float level_negative = (level - fabs(level)) / 2;
	float level_positive = (level + fabs(level)) / 2;
	float pixel_max = 255.0f;

	float* ptr_ln = &level_negative;
	float* ptr_lp = &level_positive;
	float* ptr_pm = &pixel_max;

	float* save_pixel = (float*)malloc(sizeof(float) * 4);
	float* load_pixel = (float*)malloc(sizeof(float) * 4);
	
	__m128 level_negative_sse = _mm_load_ps1(ptr_ln);
	__m128 level_positive_sse = _mm_load_ps1(ptr_lp);
	__m128 pixel_max_sse = _mm_load_ps1(ptr_pm);

	

	for(int i = 0; i < count; i++)
	{
		for(int k = 0; k < 4; k++)
		{
			bitmap_pixel_hsv_t *pixel = &pixels[i];
			load_pixel[k] = pixel->v;
		}

		__m128 pixel_sse = _mm_load_ps(load_pixel);
		__m128 result_1_sse = _mm_mul_ps(pixel_sse, level_negative_sse);
		__m128 result_2_sse = _mm_sub_ps(pixel_max_sse, pixel_sse);
		__m128 result_3_sse = _mm_mul_ps(result_2_sse, level_positive_sse);
		__m128 result_4_sse = _mm_add_ps(result_3_sse, result_1_sse);
		__m128 result_5_sse = _mm_add_ps(pixel_sse, result_4_sse);
		_mm_store_ps(save_pixel	, result_5_sse);
		
		

		for (int j = 0; j < 4; j++)
		{
			bitmap_pixel_hsv_t *pixeld = &pixels[i];
			pixeld->v = (bitmap_component_t)save_pixel[j];
			
		}
	}

	free(load_pixel);
	free(save_pixel);


}




// Get value from brightness option and return a valid value
float get_brightness(struct _arguments *arguments)
{
	if(arguments->brightness_adjust != 0x0)
	{
		// Compare string length with position of non numeric value
		if(strlen(arguments->brightness_adjust) != strspn(arguments->brightness_adjust, "0123456789-+."))
		{
			print_red("No valid digit!");
			exit(-1);
		}



		float brightness_value = (float)atof(arguments->brightness_adjust);
		printf("Value: %f\n",brightness_value);

		// Checks the brightness range
		if(brightness_value < -1.0f || brightness_value > 1.0f )
		{
			print_red("Brightness must be -1.0 to 1.0!");
			exit(-1);
		}
		return brightness_value;
	}
}

// Error Handling
void error_check(int error)
{
	switch (error)
	{
		case BITMAP_ERROR_SUCCESS:
			break;

		case BITMAP_ERROR_INVALID_PATH:
			print_red("Invalid Path!");
			exit(-1);

		case BITMAP_ERROR_INVALID_FILE_FORMAT:
			print_red("File is not a bitmap!");
			exit(-1);

		case BITMAP_ERROR_IO:
			exit(-1);

		case BITMAP_ERROR_MEMORY:
			exit(-1);

		case BITMAP_ERROR_FILE_EXISTS:
			exit(-1);
	}
}


int main(int argc, char** argv)
{

	// Holds the input values
	struct _arguments arguments = { 0 };

	 // Parse the argv input
	argp_parse(&argp, argc, argv, 0, 0, &arguments);


	//Read bitmap pixels:
	bitmap_error_t  error;
	bitmap_pixel_hsv_t* pixels;
	int width, height;
	float brightness;


	error = bitmapReadPixels(arguments.input_path, (bitmap_pixel_t**)&pixels, &width, &height, BITMAP_COLOR_SPACE_HSV);
	error_check(error);
	//assert(error == BITMAP_ERROR_SUCCESS);


	brightness = get_brightness(&arguments);

	brightness_change_sse(pixels, width * height, brightness);


	// Checks for output set by user
	if(arguments.output == 0x0)
	{
		arguments.output = strtok(arguments.input_path, ".");
		arguments.output = strcat(arguments.output, "_changed.bmp"); //TODO: dark , light depends -b '+' or -b '-'
	}


	//Write bitmap pixels:
	bitmap_parameters_t parameters =
	{

		.bottomUp = BITMAP_BOOL_TRUE,
		.widthPx = width,
		.heightPx = height,
		.colorDepth = BITMAP_COLOR_DEPTH_24,
		.compression = BITMAP_COMPRESSION_NONE,
		.dibHeaderFormat = BITMAP_DIB_HEADER_INFO,
		.colorSpace = BITMAP_COLOR_SPACE_HSV

	};


	error = bitmapWritePixels(arguments.output,BITMAP_BOOL_TRUE, &parameters, (bitmap_pixel_t*)pixels);
	error_check(error);
	//assert(error == BITMAP_ERROR_SUCCESS);



	//Free the pixel array:
	free(pixels);

	return 0;
}
