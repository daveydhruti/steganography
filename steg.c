#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    int r, g, b;
} Pixel;

typedef struct {
    char format;
    int width;
    int height;
    int max;
    Pixel **pixels;
} PPM;

int check_file_format(FILE *f, char *format) {
    if (fgetc(f) != 'P') {
        return 1;
    }
    *format = fgetc(f);
    return 0;
}

void check_for_comments(FILE *f) {
    char c;
    while (1) {
        c = fgetc(f);
        if (c >= '0' && c <= '9') {
            ungetc(c, f);
            break;
        } else if (c == '\n') {
            continue;
        } else if (c == '#') {
            while (c != '\n' && c != EOF) {
                c = fgetc(f);
            }
        }
    }
}

PPM *get_ppm(FILE *f) {
    PPM *data = malloc(sizeof(PPM));

    if (check_file_format(f, &data->format) != 0) {
        return NULL;
    }

    fseek(f, 2, SEEK_SET);
    check_for_comments(f);

    fscanf(f, "%d", &data->width);
    check_for_comments(f);
    fscanf(f, "%d", &data->height);
    check_for_comments(f);
    fscanf(f, "%d", &data->max);

    data->pixels = malloc(data->width * data->height * sizeof(Pixel *));
    for (int i = 0; i < data->width * data->height; i++) {
        data->pixels[i] = malloc(sizeof(Pixel));
        fscanf(f, "%d %d %d", &data->pixels[i]->r, &data->pixels[i]->g, &data->pixels[i]->b);
    }

    return data;
}

int write_ppm(const char *filename, const PPM *img) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return 1;
    }

    fprintf(file, "P%c\n", img->format);
    fprintf(file, "%d %d\n", img->width, img->height);
    fprintf(file, "%d\n", img->max);

    for (int i = 0; i < img->width * img->height; i++) {
        fprintf(file, "%d %d %d\n", img->pixels[i]->r, img->pixels[i]->g, img->pixels[i]->b);
    }
    return 0;
}

PPM *read_ppm(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "File %s could not be opened.\n", filename);
        return NULL;
    }

    PPM *input_image = get_ppm(f);
    fclose(f);
    return input_image;
}

void encode_bits(Pixel *pixel, unsigned char ch) {
    pixel->r = (pixel->r & ~0x3) ^ (1 << 2) | (((ch >> 5) & 0x3));  
    pixel->g = (pixel->g & ~0x7) | ((ch >> 2) & 0x7);
    pixel->b = (pixel->b & ~0x3) | (ch & 0x3);
}

int has_marker(Pixel *pixel1, Pixel *pixel2) {
    return !((pixel1->r >> 2) & 1) == ((pixel2->r >> 2) & 1);
}

unsigned char decode_bits(Pixel *pixel) {
    return (((pixel->r) & 0x3) << 5) | ((pixel->g & 0x7) << 2) | (pixel->b & 0x3);
}

PPM *copy_ppm(const PPM *img) {
    PPM *copy = malloc(sizeof(PPM));
    copy->format = img->format;
    copy->width = img->width;
    copy->height = img->height;
    copy->max = img->max;

    copy->pixels = malloc(img->width * img->height * sizeof(Pixel *));
    for (int i = 0; i < img->width * img->height; i++) {
        copy->pixels[i] = malloc(sizeof(Pixel));
        copy->pixels[i]->r = img->pixels[i]->r;
        copy->pixels[i]->g = img->pixels[i]->g;
        copy->pixels[i]->b = img->pixels[i]->b;
    }
    return copy;
}

// Fisher-Yates shuffle
void shuffle_indices(int *indices, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }
}

// Comparison function for qsort
int compare_ints(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

PPM *encode(const char *text, const PPM *img) {
    if (!text || !img || strlen(text) == 0) {
        return NULL;
    }

    size_t img_size = (size_t)img->width * img->height;
    size_t text_len = strlen(text) + 1;  // +1 for null terminator
    if (text_len > img_size) {
        fprintf(stderr, "Error: The message is too long to be encoded. Try with less than %zu characters.\n", img_size);
        return NULL;
    }

    PPM *encoded_img = copy_ppm(img);

    int *indices = malloc(img_size * sizeof(int));
    for (int i = 0; i < img_size; i++) {
        indices[i] = i;
    }
    shuffle_indices(indices, img_size);

    int *selected = malloc(text_len * sizeof(int));
    for (int i = 0; i < text_len; i++) {
        selected[i] = indices[i];
    }
    qsort(selected, text_len, sizeof(int), compare_ints);

    for (int i = 0; i < text_len; i++) {
        encode_bits(encoded_img->pixels[selected[i]], text[i]);
    }

    free(indices);
    free(selected);
    return encoded_img;
}

char *decode(const PPM *original_img, const PPM *encoded_img) {
    if (!original_img || !encoded_img) {
        return NULL;
    }

    if (original_img->width != encoded_img->width ||
        original_img->height != encoded_img->height) {
        fprintf(stderr, "Error: Image dimensions do not match\n");
        return NULL;
    }

    size_t img_size = (size_t)original_img->width * original_img->height;
    char *text = malloc(img_size + 1);
    int k = 0;

    for (int i = 0; i < img_size; i++) {
        if (original_img->pixels[i]->r != encoded_img->pixels[i]->r ||
            original_img->pixels[i]->g != encoded_img->pixels[i]->g ||
            original_img->pixels[i]->b != encoded_img->pixels[i]->b) {

            if (has_marker(encoded_img->pixels[i], original_img->pixels[i])) {
                text[k] = decode_bits(encoded_img->pixels[i]);
                if (text[k] == '\0') {
                    break;
                }
                k++;
            }
        }
    }

    text[k] = '\0';
    return text;
}

void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("OPTIONS:\n");
    printf("  -h, --help\n");
    printf("      Print this menu and exit\n\n");
    printf("  -e, --encode <input.ppm> <output.ppm> <message>\n");
    printf("      Encode a message into a PPM image\n");
    printf("      <input.ppm>   : Original PPM image file\n");
    printf("      <output.ppm>  : Output PPM image with encoded message\n");
    printf("      <message>     : Message to hide in the image\n\n");
    printf("  -d, --decode <original.ppm> <encoded.ppm>\n");
    printf("      Decode a message from a PPM image\n");
    printf("      <original.ppm> : Original PPM image (before encoding)\n");
    printf("      <encoded.ppm>  : PPM image with hidden message\n");
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    } else if (strcmp(argv[1], "-e") == 0 || strcmp(argv[1], "--encode") == 0) {
        if (argc != 5) {
            printf("Error: Incorrect number of arguments\n");
            return 1;
        }

        PPM *input_img = read_ppm(argv[2]);
        if (input_img == NULL) {
            fprintf(stderr, "Error reading file %s\n", argv[2]);
            return 1;
        }

        PPM *encoded_img = encode(argv[4], input_img);
        if (encoded_img == NULL) {
            return 1;
        }

        write_ppm(argv[3], encoded_img);

    } else if (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--decode") == 0) {
        if (argc != 4) {
            printf("Error: Incorrect number of arguments\n");
            return 1;
        }

        PPM *original_img = read_ppm(argv[2]);
        if (original_img == NULL) {
            fprintf(stderr, "Error reading file %s\n", argv[2]);
            return 1;
        }

        PPM *encoded_img = read_ppm(argv[3]);
        if (encoded_img == NULL) {
            fprintf(stderr, "Error reading file %s\n", argv[3]);
            return 1;
        }

        char *text = decode(original_img, encoded_img);
        printf("Secret: %s\n", text);

    } else {
        printf("Error: Invalid option '%s'\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }
}