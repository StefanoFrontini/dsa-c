#include "helpers.h"
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

int getMax(WORD a, BYTE b)
{
    //printf("%u - %u\n", a, b);
    if (a > b)
    {
        return b;
    }
    else
    {
        return a;
    }
}

void swap(int width, RGBTRIPLE arr[width])
{
    int j = 0;
    int k = width;
    while(j < k)
    {
        RGBTRIPLE tmp;
        tmp = arr[j];
        arr[j] = arr[k - 1];
        arr[k - 1] = tmp;
        j++;
        k--;
    }
}

// Convert image to grayscale
void grayscale(int height, int width, RGBTRIPLE image[height][width])
{

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            BYTE red = image[i][j].rgbtRed;
            BYTE green = image[i][j].rgbtGreen;
            BYTE blue = image[i][j].rgbtBlue;

            BYTE average = (red + green + blue) / 3;

            image[i][j].rgbtRed = average;
            image[i][j].rgbtGreen = average;
            image[i][j].rgbtBlue = average;
        }
    }

}

// Convert image to sepia
void sepia(int height, int width, RGBTRIPLE image[height][width])
{
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            BYTE red = image[i][j].rgbtRed;
            BYTE green = image[i][j].rgbtGreen;
            BYTE blue = image[i][j].rgbtBlue;

            BYTE max = 255;

            BYTE sepiaRed = getMax(
                floor(0.393 * (float) red + 0.769 * (float) green + 0.189 * (float) blue), max);

            BYTE sepiaGreen = getMax(
                floor(0.349 * (float) red + 0.686 * (float) green + 0.168 * (float) blue), max);

            BYTE sepiaBlue = getMax(
                floor(0.272 * (float) red + 0.534 * (float) green + 0.131 * (float) blue), max);

            image[i][j].rgbtRed = sepiaRed;
            image[i][j].rgbtGreen = sepiaGreen;
            image[i][j].rgbtBlue = sepiaBlue;
        }
    }
}

// Reflect image horizontally
void reflect(int height, int width, RGBTRIPLE image[height][width])
{

    for (int i = 0; i < height; i++)
    {
        swap(width, image[i]);
    }
}

bool isValid(int a, int b, int height, int width)
{
    if((a >= 0 && a < height) && (b >= 0 && b < width))
    {
        return true;
    }
    else
    {
        return false;
    }
}


// Blur image
void blur(int height, int width, RGBTRIPLE image[height][width])
{
  
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {

            int counter = 0;
            RGBTRIPLE arr[9];

            for (int m = -1; m < 2; m++)
            {
                for (int n = -1; n < 2; n++)
                {

                    if(isValid(i + m, j + n, height, width))
                    {
                        arr[counter] = image[i + m][j + n];
                        counter++;
                    }
                }
            }
            int redTotal = 0;
            int greenTotal = 0;
            int blueTotal = 0;


            for (int z = 0; z < counter; z++)
            {
                redTotal += arr[z].rgbtRed;
                greenTotal += arr[z].rgbtGreen;
                blueTotal += arr[z].rgbtBlue;

            }

            if(counter == 0) return;
            image[i][j].rgbtRed = floor((float) redTotal / (float) counter);
            image[i][j].rgbtGreen = floor((float) greenTotal / (float) counter);
            image[i][j].rgbtBlue = floor((float) blueTotal / (float) counter);

        }
    }
}
