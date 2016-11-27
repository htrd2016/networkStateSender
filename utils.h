char* run_cmd(const char *cmd, int cmd_length, char out_str[], int out_str_size);
int write_to_send_file(const char *file_name, const char *agent_host_name, char (*item_key)[512], char (*item_data)[64], int item_count);
void send_file(const char *server_ip, int server_port, const char *file_name);
int parse_oids_items_to_array(const char *file_path, char (*snmp_oid_out)[512], char (*item_key_out)[512], int *count_out);

