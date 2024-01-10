#define FILE_NAME "hello_world"
#define BUFFER_SIZE 1024
#define PROC_PATH "/proc/"

enum settings_output {
    OUTPUT_DEFAULT = 0,
    OUTPUT_WITHOUT_COLUMN
};

typedef enum settings_output output_settings;