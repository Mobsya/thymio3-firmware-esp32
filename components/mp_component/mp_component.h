
#define MAX_MP_SCRIPT_LEN 1024

void init_micropython(void);
void exec_script(uint8_t id);
void mp_exec_script_from_ram(char *script);
void mp_stop_script(void);

// Check if the script mainID.py is present. ID is from 1 to 7.
uint8_t script_is_present(uint8_t id);
