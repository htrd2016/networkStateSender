#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

char* run_cmd(const char *cmd,  char out_str[], int out_str_size)
{
    FILE *fp = 0;
    int index = 0;

    memset(out_str, 0, out_str_size);

    if ((fp = popen(cmd, "r")) == NULL)
    {
        strcpy(out_str, "error:popen failed");
        perror("popen failed");
        return out_str;
    }

    while (fgets(out_str+index, out_str_size-index-1, fp) != NULL)
    {
        index = strlen(out_str);
    }

    if (pclose(fp) == -1)
    {
        strcpy(out_str, "error:pclose failed");
        perror("pclose failed");
        return out_str;
    }
    return out_str;
}

int send_one_data(const char *server_ip, int server_port, const char *agent_host_name, const char *key_to_send, const char *data)
{
  char sender_cmd[1024] = {0};
  char ret[1024] = {0};
  sprintf(sender_cmd, "zabbix_sender -z %s -p %d -s \"%s\" -k %s -o %s\n", server_ip, server_port, agent_host_name, key_to_send, data);
  printf("%s", sender_cmd);
  run_cmd(sender_cmd, ret, 1024);
  printf("send ret:%s\n", ret);
  if (strstr(ret, "failed: 1") != NULL)
  {
    perror(ret);
    printf("error\n");
    return -1;
  }
  return 0;
}


int write_to_send_file(const char *file_name, const char *agent_host_name, char (*item_key)[512], char (*item_data)[64], int item_count)
{
   int index = 0;
   FILE *fp = fopen(file_name, "w");
   if(fp == 0)
   {
     perror("write file failed!!!");
     printf("open file %s failed\n", file_name);
     return -1;
   }
   for(index=0; index<item_count; index++)
   {
     fwrite("\"", 1, 1, fp);
     fwrite(agent_host_name, strlen(agent_host_name), 1, fp);
     fwrite("\" ", 2, 1, fp);
     fwrite(item_key[index], strlen(item_key[index]), 1, fp);
     fwrite(" ", 1, 1, fp);
     fwrite(item_data[index], strlen(item_data[index]), 1, fp);
     fwrite("\n", 1, 1, fp);
   }
   fclose(fp);
   return 0;
}

void send_file(const char *server_ip, int server_port, const char *file_name)
{
  char sender_cmd[1024] = {0};
  char ret[1024] = {0};
  sprintf(sender_cmd, "zabbix_sender -z %s -p %d -i %s\n", server_ip, server_port, file_name);
  printf("%s", sender_cmd);
  run_cmd(sender_cmd, ret, sizeof(ret));
  printf("send ret:%s\n", ret);
  if (strstr(ret, "failed: 1") != NULL)
  {
    perror(ret);
    printf("error\n");
    sleep(10);
  }
}

int parse_oids_items_to_array(const char *file_path, char (*snmp_oid_out)[512], char (*item_key_out)[512], int *count_out)
{
  char line[1024];
  char snmp_oid[512] = {0};
  char item_key[512] = {0};

  int index = 0;
  *count_out = 0;

  FILE *fp = fopen(file_path, "r");
  if (fp==NULL)
  {
    perror("can not open config file\n");
    return -1;
  }

  while(1)
  {
    memset(line, 0, sizeof(line));
    if (NULL == fgets(line,sizeof(line),fp))
    {
      break;
    }
    if(line[0] == '#')
    {
      continue;
    }

    if(-1 == sscanf(line, "%s %s", snmp_oid, item_key))
    {
      break;
    }

    printf("snmp_key:%s, item_key:%s\n", snmp_oid, item_key);
    strcpy(snmp_oid_out[index], snmp_oid);
    strcpy(item_key_out[index], item_key);
    index++;
  }
  *count_out = index;

  printf("count:%d\n", index);
  fclose(fp);
  return index;
}
