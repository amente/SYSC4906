#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main(int argc, char *argv[])
{
    uint16_t sample;
    uint32_t i = 0;
    size_t filesize;
    FILE *in, *out;
    char out_name[50];
    if (argc < 2)
    	return -1;
    strcpy(out_name, argv[1]);
    strcat(out_name, ".csv");
	in = fopen(argv[1], "rb");
    out = fopen(out_name, "w");
    if (in == NULL || out == NULL)
    {
    	fclose(in);
    	fclose(out);
    	return -1;
    }
	while (fread(&sample, 2, 1, in) > 0)
    {
        fprintf(out, "%u\r", sample);
        ++i;
    }
    fclose(out);
    fclose(in);
    printf("OK! Read %u samples.\n", i);
    printf("Press any key to exit.");
    getch();
    return 0;
}

