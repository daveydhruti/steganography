#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


typedef struct LinkedList {
    char *comment_node;
    int position;
    struct LinkedList *next_node;
} LinkedList;

typedef struct {
    int r, g, b;
} Pixel;

typedef struct {
    char format;
    int width;
    int height;
    int max;
    Pixel **pixels;
    // LinkedList *comments;
} PPM;


int check_file_format(FILE *f, char *format) {
    // Checks the "magic number" for file format    
    if (fgetc(f) != 'P') {
        return 1; // input output error
    }

    *format = fgetc(f);

    return 0;
}


// add comment to linked list 
LinkedList *add_comment(LinkedList *head, const char *comment, int position) {
    LinkedList *new_node = malloc(sizeof(LinkedList));
    if (new_node == NULL) {
        return head;
    }
    
    new_node->comment_node = malloc(strlen(comment) + 1);
    if (new_node->comment_node == NULL) {
        free(new_node);
        return head;
    }
    
    strcpy(new_node->comment_node, comment);
    new_node->position = position;
    new_node->next_node = head;
    
    return new_node;
}

void check_for_comments(FILE *f){
    char c;
    while (1) {
        c = fgetc(f);
        if (c >= '0' && c <= '9') {
            ungetc(c, f);
            break;
        } else if (c == '\n') {
            continue;
        } else if (c == '#') {
            while(c != '\n') {
                int comment_length = 1;
                while (c != '\n' && c != EOF) {
                    c = fgetc(f);
                    comment_length++;
                }
                char buffer[comment_length + 1];
                fseek(f, -comment_length, SEEK_CUR);
                fread(buffer, sizeof(char), comment_length - 1, f); 
                buffer[comment_length - 1] = '\0';
                fseek(f, 1, SEEK_CUR);
                //TODO store these comments and positions
                // printf("%s\n", buffer);
            }
        }
    }    
}

PPM *get_ppm(FILE *f) {

    PPM *data = malloc(sizeof(PPM));
    
    // start of header
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
    // end of header

    data->pixels = malloc(data->width * data->height * sizeof(sizeof(int) * 3));

    for (int i = 0; i < data->width * data->height; i++) {
        data->pixels[i] = malloc(sizeof(int) * 3);
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

PPM *encode(const char *text, const PPM *img) {
    if (!text || !img || strlen(text) == 0) {
        return NULL;
    }

    int img_size = img->width * img->height;

    if (strlen(text) > img_size) {
        fprintf(stderr, "Error: The message is too long to be encoded. Try with less than %d characters.\n", img_size);
        return NULL;
    }

    PPM *encoded_img = malloc(sizeof(PPM));

    // copy PPM data
    encoded_img->format = img->format;
    encoded_img->width = img->width;
    encoded_img->height = img->height;
    encoded_img->max = img->max;
    encoded_img->pixels = img->pixels;

    int rand_int = rand() % ((encoded_img->width * encoded_img->height) / strlen(text));
    
    
    for (int i = 0; i < strlen(text); i++) {
        while (encoded_img->pixels[rand_int]->r == text[i]) {
            rand_int++;
            printf("same char");
        }
        
        encoded_img->pixels[rand_int]->r = text[i];

        rand_int += rand() % ((encoded_img->width * encoded_img->height) / strlen(text)) + 1;

        if (rand_int > img_size) {
            fprintf(stderr, "Error: index out of bounds."); // most likely won't happen
            return NULL;
        }
    }
    return encoded_img;
}


char *decode(const PPM * originalImg, const PPM * encodedImg) {
    if (!originalImg || !encodedImg || originalImg->width != encodedImg->width || originalImg->height != encodedImg->height){
        return NULL;
    }

    char *text = malloc(1000); // TODO: fix size

    int k = 0;
    for (int i = 0; i < encodedImg->width * encodedImg->height; i++) {
        if (encodedImg->pixels[i]->r != originalImg->pixels[i]->r) {
            text[k] = encodedImg->pixels[i]->r;
            k++;
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
    printf("      <original.ppm> : Original PPM image file\n");
    printf("      <encoded.ppm>  : PPM image with hidden message\n");
}


int main(int argc, char *argv[]) {

    srand(time(NULL)); // using time as seed

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
        
        PPM *newimg = encode(argv[4], input_img);
        if (newimg == NULL) {
            return 1;
        }

        write_ppm(argv[3], newimg);
        
    } else if (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--decode") == 0) {
        if (argc != 4) {
            printf("Error: Incorrect number of arguments\n");
            return 1;
        }
        
        PPM *original_ppm = read_ppm(argv[2]);
        PPM *encoded_ppm = read_ppm(argv[3]);
        
        char *text = decode(original_ppm, encoded_ppm);
        printf("Secret: %s\n", text);

    } else {
        printf("Error: Invalid option '%s'\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }
}