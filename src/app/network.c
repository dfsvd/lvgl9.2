// src/app/network.c

#include "network.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RESPONSE_SIZE (8 * 1024) // 8KB 固定缓冲区

// URL 编码辅助函数
static char *url_encode(const char *str) {
  if (!str)
    return NULL;
  size_t len = strlen(str);
  // 最坏情况每个字符编码为 %XX，需要 3 倍空间
  char *encoded = malloc(len * 3 + 1);
  if (!encoded)
    return NULL;

  char *p = encoded;
  for (const char *s = str; *s; s++) {
    unsigned char c = (unsigned char)*s;
    // 保留字母数字及安全字符
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' ||
        c == '/') {
      *p++ = c;
    } else {
      sprintf(p, "%%%02X", c);
      p += 3;
    }
  }
  *p = '\0';
  return encoded;
}

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
  char *encoded_url = url_encode(url);
  if (!encoded_url)
    return -1;

  char command[2048];
  snprintf(command, sizeof(command), "curl -s -k -o \"%s\" \"%s\"", local_path,
           encoded_url);
  free(encoded_url);

  // system() 返回命令的退出状态
  return system(command);
}

// 后台下载，返回 0 成功启动
int network_download_file_bg(const char *url, const char *local_path) {
  char *encoded_url = url_encode(url);
  if (!encoded_url)
    return -1;

  char command[2048];
  snprintf(command, sizeof(command),
           "nohup curl -s -k -o \"%s\" \"%s\" >/dev/null 2>&1 &", local_path,
           encoded_url);
  free(encoded_url);

  int rc = system(command);
  return (rc == 0) ? 0 : -1;
}