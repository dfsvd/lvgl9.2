// include/app/network.h

#ifndef NETWORK_H
#define NETWORK_H

/**
 * @brief 执行一个 cURL GET 请求，并将响应体读取到动态分配的内存中。
 * * 注意：调用者必须负责使用 free() 释放返回的 char* 内存。
 *
 * @param url 完整的请求 URL (包含参数)。
 * @return 成功返回 JSON 字符串指针，失败返回 NULL。
 */
char *network_fetch_data(const char *url);

/**
 * @brief 执行 cURL 下载文件
 */
int network_download_file(const char *url, const char *local_path);

/**
 * @brief 后台执行 cURL 下载文件
 * @param base_url 基础 URL（如 "http://8.217.250.241/music"）
 * @param filename 文件名（如 "追梦人 - 凤飞飞.flac"，会自动 URL 编码）
 * @param local_path 本地保存路径
 */
int network_download_file_bg(const char *base_url, const char *filename,
                             const char *local_path);

#endif // NETWORK_H