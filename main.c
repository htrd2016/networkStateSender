#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "utils.h"

#define MAX_COUNT 20

FILE *files[MAX_COUNT];

int get_network_names(char (*names)[512], int *count)
{
  char *start = 0;
  char *end = 0;
  int index =0;

  const char *ls_cmd = "ls /sys/class/net/";
  char ls_cmd_ret[512] = {0};
  run_cmd(ls_cmd, ls_cmd_ret, sizeof(ls_cmd_ret));
  start = ls_cmd_ret;

  while(NULL!=(end=strchr(start, '\n'))&&index<=MAX_COUNT)
  {
    memset(names[index], 0, 512);
    memcpy(names[index], start, end-start);
    //printf("%s\n", names[index]);
    start = end+1;
    index++;
  }
  *count = index;
  return index;
}

char *read_line(FILE *file, char line[32])
{
  char *tmp = NULL;
  memset(line,0,32);
  if(file == NULL)
  {
    perror("file not opened!!!");
    printf("file == null\n");
    return line;
  }

  fseek(file,0,SEEK_SET);  
//  printf("%d\n", file);  

  if(fgets(line, 32, file))
  {
    if(NULL!=(tmp = strchr(line,'\n')))
    {
      *tmp = '\0';
    }
  }
  return line;
}

int get_network_state(int count, char (*states)[32])
{
  int index = 0;
  for(index=0; index<count; index++)
  {
    if(files[index] == NULL)
    {
       perror("open file failed!!!!\n");
       return -1;
    }
    read_line(files[index], states[index]);
    //printf("[%s]\n", states[index]);
  }
  return 0;
}

void init_files()
{
  int index = 0;
  for(index=0; index<MAX_COUNT;index++)
  {
    files[index] = NULL;
  }
}

void close_files()
{
  int index = 0;
  for(index=0; index<MAX_COUNT;index++)
  {
    if(files[index] != NULL)
    {
      fclose(files[index]);
      files[index] = NULL;
    }
  }
}

int open_files(char (*names)[512],int count)
{
  char file_name[512] = {0};
  int index = 0;
  for(index=0; index<count; index++)
  {
    sprintf(file_name, "/sys/class/net/%s/carrier", names[index]);
    printf("%s %d\n", file_name, count);
    if(files[index] != NULL)
    {
      fclose(files[index]);
    }
    files[index] = fopen(file_name, "r");
    if(files[index] == NULL)
    {
       perror("open file failed!!!!\n");
       printf("open file %s failed!!!\n", file_name);
       return -1;
    }
  }
//printf("------------\n");
  return 0;
}

int send_auto_discovery(const char *server_ip, int server_port, const char *agent_host_name, const char* discovery_item_key, char (*network_names)[512], int count)
{
  char cmd_ret[512] = {0};
  char cmd[2048] = {0};
  char array_items[1024] = {0};
  int index = 0;
  char item_temp[512] = {0};
  for(index=0; index<count; index++)
  {
    if(strlen(array_items) == 0)
    {
      sprintf(array_items, "{\"{#NAME}\":\"%s\"}", network_names[index]);
    }
    else
    {
      sprintf(item_temp, ",{\"{#NAME}\":\"%s\"}", network_names[index]);
      strcat(array_items, item_temp);
      //sprintf(array_items, "%s,{\"{#NAME}\":\"%s\"}", array_items, network_names[index]);
    }
    //printf("%s\n", array_items);
  }
  sprintf(cmd, "zabbix_sender -z %s -p %d -s \"%s\" -k %s -o \'{\"data\":[\%s]}\'", server_ip, server_port, agent_host_name, discovery_item_key, array_items);

  printf("%s\n", cmd);
  run_cmd(cmd, cmd_ret, sizeof(cmd_ret));
  
  if (strstr(cmd_ret, "failed: 0") == NULL)
  {
    return -1;
  }
 
  return 0;
}

int send_network_to_server(const char *server_ip, int server_port, const char *agent_host_name, char *item_key, char (*network_names)[512], char (*item_data)[32], int item_count)
{
   int index = 0;
   char key[512] = {0};
   for(index=0; index<item_count; index++)
   {
     if(strlen(item_data[index]) == 0)
     {
       continue;
     }
     memset(key, 0, sizeof(key));
     sprintf(key, "%s[%s]", item_key, network_names[index]);

     send_one_data(server_ip, server_port, agent_host_name, key, item_data[index]);
   }
   return 0;
}

int main(int argc, char* argv[])
{
  char server_ip[64] = {0};
  int server_port = -1;
  char agent_host_name[512] = {0};
  char item_key[512] = {0};
  int send_interval = -1;
  char discovery_item_key[512] = {0};

  char network_names[MAX_COUNT][512];
  char network_state[MAX_COUNT][32];
  int count = 0;

  int time = 0;

  if(argc<7)
  {
    printf("use<path><server ip><server port><agent host name><auto discovery item key><item key><send data interval>\n");
    return -1;
  }

  strcpy(server_ip, argv[1]);
  server_port = atoi(argv[2]);
  strcpy(agent_host_name, argv[3]);
  strcpy(discovery_item_key, argv[4]);  
  strcpy(item_key, argv[5]);
  send_interval = atoi(argv[6]);

  printf("srever ip:%s,srever port:%d,agent host name:%s,item key:%s,send interval:%d\n", server_ip, server_port, agent_host_name, item_key, send_interval);

  init_files(files);
  while(1)
  { 
    get_network_names(network_names, &count);
  
    if(count>0)
    {
      if(-1 == open_files(network_names, count))
      {
         sleep(30);
         continue;
      }
      send_auto_discovery(server_ip, server_port, agent_host_name, discovery_item_key, network_names, count);
      
      for(time=0; time<5000; time++)
      {
        if(-1 == get_network_state(count, network_state))
        {
           sleep(30);
           break;
        }
        
        send_network_to_server(server_ip, server_port, agent_host_name, item_key, network_names, network_state, count);
        usleep(send_interval*1000);
      }
    }
    else
    {
       perror("get net work names failed!!!\n");
       sleep(30);
    } 
  }
  close_files();
  return 0;
}
