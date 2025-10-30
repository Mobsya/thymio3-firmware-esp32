
#define MAX_MP_SCRIPT_LEN 51200

void init_micropython(void);
void exec_script(uint8_t id);
void mp_exec_script_from_ram(char *script);
void mp_stop_script(void);
void mp_save_script(char* script, uint8_t id, uint16_t script_len);
void mp_save_file(uint8_t* data, char* filename, uint32_t data_len);
void mp_delete_file(char* filename);
void mp_list_files(void);
void mp_list_files_free_buffer(void);
void mp_mem_info(void);

// Check if the script mainID.py is present. ID is from 1 to 7.
uint8_t script_is_present(uint8_t id);
