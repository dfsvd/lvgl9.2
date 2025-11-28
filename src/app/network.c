// src/app/network.c

#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RESPONSE_SIZE (8 * 1024) // 8KB 固定缓冲区

char *network_fetch_data(const char *url) {
  char command[512];
  FILE *fp;

  // 使用绝对路径确保在不同环境下也能找到 curl
  // -s: 静默模式; -k: 跳过证书验证
  // 将 stderr 重定向到 stdout 以便捕获错误信息
  snprintf(command, sizeof(command), "/bin/curl -s -k \"%s\" 2>&1", url);

  fp = popen(command, "r");
  if (fp == NULL) {
    perror("[Network] popen failed to execute curl command");
    return NULL;
  }

  // 【修正】使用固定缓冲区，避免 realloc 错误
  char *result = (char *)malloc(MAX_RESPONSE_SIZE);
  if (result == NULL) {
    perror("[Network] malloc failed");
    pclose(fp);
    return NULL;
  }

  // 读取 curl 的输出
  size_t bytes_read = fread(result, 1, MAX_RESPONSE_SIZE - 1, fp);

  pclose(fp);

  if (bytes_read > 0) {
    result[bytes_read] = '\0'; // 确保空字符结束
    return result;
  } else {
    // 读取失败或响应为空，打印调试信息（包含 stderr）
    if (bytes_read == 0) {
      // 如果有错误输出，会被捕获到 result（空字符串时表示无输出）
      result[0] = '\0';
      fprintf(stderr, "[Network] curl returned no stdout. Captured: '%s'\n",
              result);
    }
    free(result);
    return NULL;
  }
}

int network_download_file(const char *url, const char *local_path) {
  char command[512];
  snprintf(command, sizeof(command), "curl -s -k -o %s \"%s\"", local_path,
           url);
  // system() 返回命令的退出状态
  return system(command);
}